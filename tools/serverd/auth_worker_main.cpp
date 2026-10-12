#include "engine/hrb_harbour.h"
#include <iostream>
#include "hbapigt.h"
#include "hbapierr.h"
#include <algorithm>
#include <iterator>
#include <string>
#include <filesystem>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <aclapi.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
// Worker-only RTL surface requested by Pritpal Bedi. Referencing these
// symbols pulls Harbour's coupled hbtoken object from the static RTL.
HB_FUNC_EXTERN( HB_ATOKENS );
HB_FUNC_EXTERN( HB_TOKENCOUNT );
HB_FUNC_EXTERN( HB_TOKENGET );
HB_FUNC_EXTERN( HB_TOKENPTR );

// Same byte-reverse semantics as the native OpenADS expression builtin.
// Preserve embedded NULs; this is not Unicode grapheme reversal.
HB_FUNC( REVERSE ) {
    if (!HB_ISCHAR(1)) {
        hb_errRT_BASE_SubstR(EG_ARG, 3012, nullptr, "REVERSE", HB_ERR_ARGS_BASEPARAMS);
        return;
    }
    std::string value(hb_parc(1), static_cast<std::size_t>(hb_parclen(1)));
    std::reverse(value.begin(), value.end());
    hb_retclen(value.data(), static_cast<HB_SIZE>(value.size()));
}
HB_INIT_SYMBOLS_BEGIN( openads_auth_worker_rtl__InitSymbols )
{ "HB_ATOKENS",    { HB_FS_PUBLIC }, { HB_FUNCNAME( HB_ATOKENS ) },    nullptr },
{ "HB_TOKENCOUNT", { HB_FS_PUBLIC }, { HB_FUNCNAME( HB_TOKENCOUNT ) }, nullptr },
{ "HB_TOKENGET",   { HB_FS_PUBLIC }, { HB_FUNCNAME( HB_TOKENGET ) },   nullptr },
{ "HB_TOKENPTR",   { HB_FS_PUBLIC }, { HB_FUNCNAME( HB_TOKENPTR ) },   nullptr },
{ "REVERSE",       { HB_FS_PUBLIC }, { HB_FUNCNAME( REVERSE ) },       nullptr }
HB_INIT_SYMBOLS_END( openads_auth_worker_rtl__InitSymbols )

namespace {
#ifdef _WIN32
bool read_auth_module(const std::string& path, std::string& contents) {
    const auto wide = std::filesystem::path(path).wstring();
    HANDLE file = CreateFileW(wide.c_str(), GENERIC_READ | READ_CONTROL, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) { std::fprintf(stderr, "Auth module protected open failed (%lu).\n", GetLastError()); return false; }
    BY_HANDLE_FILE_INFORMATION fi{};
    LARGE_INTEGER size{};
    if (!GetFileInformationByHandle(file, &fi) ||
        (fi.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ||
        !GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 1024 * 1024) {
        std::fprintf(stderr, "Auth module file metadata rejected (%lu).\n", GetLastError()); CloseHandle(file); return false;
    }
    PSECURITY_DESCRIPTOR sd = nullptr; PACL acl = nullptr;
    if (GetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
        nullptr, nullptr, &acl, nullptr, &sd) != ERROR_SUCCESS) { std::fprintf(stderr, "Auth module security descriptor read failed.\n"); CloseHandle(file); return false; }
    HANDLE token = nullptr; DWORD bytes = 0; bool ok = acl != nullptr;
    std::vector<unsigned char> info;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) { std::fprintf(stderr, "Auth module token read failed.\n"); ok = false; }
    if (ok) {
        GetTokenInformation(token, TokenUser, nullptr, 0, &bytes);
        info.resize(bytes);
        ok = bytes != 0 && GetTokenInformation(token, TokenUser, info.data(), bytes, &bytes);
    }
    for (DWORD i = 0; ok && i < acl->AceCount; ++i) {
        void* ptr = nullptr; if (!GetAce(acl, i, &ptr)) { ok = false; break; }
        const auto* header = static_cast<ACE_HEADER*>(ptr);
        if (header->AceType == ACCESS_DENIED_ACE_TYPE) continue;
        if (header->AceType != ACCESS_ALLOWED_ACE_TYPE) { std::fprintf(stderr, "Auth module unexpected ACE type %u.\n", static_cast<unsigned>(header->AceType)); ok = false; break; }
        auto* ace = static_cast<ACCESS_ALLOWED_ACE*>(ptr);
        PSID sid = &ace->SidStart;
        if (!IsValidSid(sid) || (!EqualSid(sid, reinterpret_cast<TOKEN_USER*>(info.data())->User.Sid) &&
            !IsWellKnownSid(sid, WinLocalSystemSid) && !IsWellKnownSid(sid, WinBuiltinAdministratorsSid))) { std::fprintf(stderr, "Auth module ACL has an unapproved principal.\n"); ok = false; }
    }
    if (token) CloseHandle(token);
    if (sd) LocalFree(sd);
    if (ok) {
        contents.resize(static_cast<std::size_t>(size.QuadPart));
        DWORD received = 0;
        ok = ReadFile(file, &contents[0], static_cast<DWORD>(contents.size()), &received, nullptr) && received == contents.size();
    }
    CloseHandle(file);
    if (!ok) { std::fprintf(stderr, "Auth module protected ACL/read validation failed.\n"); std::fill(contents.begin(), contents.end(), '\0'); contents.clear(); }
    return ok;
}
#else
bool read_auth_module(const std::string& path,std::string& bytes) {
    int fd=open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC);struct stat st{};
    if(fd<0)return false;
    if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_uid!=geteuid() ||
       (st.st_mode&(S_IRWXG|S_IRWXO))!=0 || st.st_size<=0 || st.st_size>1024*1024){close(fd);return false;}
    bytes.resize(static_cast<std::size_t>(st.st_size));std::size_t off=0;
    while(off<bytes.size()){auto n=read(fd,&bytes[off],bytes.size()-off);if(n<=0)break;off+=static_cast<std::size_t>(n);}
    close(fd);return off==bytes.size();
}
#endif
}
int main(int argc,char** argv){
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
    if(argc!=2)return 1;
    hb_gtSetDefault("NUL");
    // Isolated process owns its own backend. Never register with index UDFs.
    openads::engine::hrb_udf::HarbourBackend backend;std::string err;
    std::string module;
    if(!read_auth_module(argv[1],module))return 1;
    // HB_HRBLOAD accepts in-memory HRB bytes: no check/reopen race.
    if(!backend.load(module,err)||!backend.has("OADS_AUTH_V1"))return 1;
    std::string input;char b[1024];
    while(std::cin.read(b,sizeof(b))||std::cin.gcount()){input.append(b,static_cast<std::size_t>(std::cin.gcount()));if(input.size()>16384)return 1;}
    if(input=="P"){std::cout<<"READY";return 0;}
    if(input.size()<41||input[0]!='A')return 1;
    using Scalar=openads::engine::hrb_udf::Scalar;Scalar args[3],out;
    args[0].kind=Scalar::Kind::String;args[0].s=input.substr(1,32);
    std::size_t pos=33;
    for(unsigned i=1;i<3;++i){if(pos+4>input.size())return 1;std::size_t n=0;for(unsigned j=0;j<4;++j)n|=std::size_t(static_cast<unsigned char>(input[pos++]))<<(8*j);if(n>input.size()-pos)return 1;args[i].kind=Scalar::Kind::String;args[i].s=input.substr(pos,n);pos+=n;}
    if(pos!=input.size())return 1;
    if(!backend.call("OADS_AUTH_V1",args,3,out,err)||out.kind!=Scalar::Kind::String||out.s.size()>4363)return 1;
    std::cout.write(out.s.data(),static_cast<std::streamsize>(out.s.size()));std::cout.flush();
    // Process exits immediately; no reusable Harbour heap holding credentials.
    return std::cout.good()?0:1;
}
