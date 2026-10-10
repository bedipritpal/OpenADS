#include "util/log.h"

#include "platform/time.h"
#include "util/client_config.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <map>
#include <ostream>
#include <string>
#include <unordered_set>

namespace openads::util {

namespace {

const char* level_name(LogLevel l) {
    switch (l) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "?";
}

std::string lower(std::string_view s) {
    std::string out{s};
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    return out;
}

constexpr char kB36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

std::ostream* g_console = nullptr;          // nullptr = stderr
std::ostream* g_file    = nullptr;          // nullptr = OPENADS_LOG_FILE
enum class Details { Auto, On, Off };
Details g_details = Details::Auto;
bool    g_file_sink_set = false;            // true when set_audit_file() used
std::mutex g_audit_mu;
std::atomic<std::uint32_t> g_next_conn{0};
std::atomic<std::uint32_t> g_next_seq{1};
std::atomic<bool> g_seeded{false};
std::uint32_t g_conn_seed = 0;
std::unordered_set<std::string> g_remote_asked;
std::uint64_t g_remote_asked_epoch = 0;

std::string norm_audit_path(std::string_view p) {
    std::string s{p};
    for (char& ch : s) {
        if (ch == '\\') ch = '/';
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

void ensure_seed() {
    bool expected = false;
    if (g_seeded.compare_exchange_strong(expected, true)) {
        auto us = openads::platform::utc_unix_micros();
        g_conn_seed = static_cast<std::uint32_t>(us ^ (us >> 32));
    }
}

void write_console_line(const std::string& line) {
    if (g_console != nullptr) {
        (*g_console) << line;
        g_console->flush();
        return;
    }
    // No explicit sink: the DLL is embedded in someone else's process
    // (php, Apache, Harbour apps, the parity harness). SAP's ace64.dll
    // never writes to the host's streams, so the stderr echo is opt-in —
    // OPENADS_LOG / OPENADS_RESOLVE_VERBOSE (env or openads.ini keys
    // `log` / `resolve_verbose`), or set_audit_console().
    // OPENADS_LOG_FILE keeps working regardless via write_file_line.
    if (!client_setting_truthy("OPENADS_RESOLVE_VERBOSE",
                               "resolve_verbose") &&
        client_setting("OPENADS_LOG", "log").empty()) {
        return;
    }
    std::fputs(line.c_str(), stderr);
    std::fflush(stderr);
}

void write_file_line(const std::string& line) {
    if (g_file != nullptr) {
        (*g_file) << line;
        g_file->flush();
        return;
    }
    if (g_file_sink_set) return;  // tests explicitly disabled the file
    const std::string path = client_setting("OPENADS_LOG_FILE", "log_file");
    if (path.empty()) return;
    std::FILE* f = std::fopen(path.c_str(), "a");
    if (f == nullptr) return;
    std::fputs(line.c_str(), f);
    std::fflush(f);
    std::fclose(f);
}

} // namespace

void Log::write(LogLevel level, std::string_view message) noexcept {
    DiagnosticGuard guard;
    if (!guard || sink_ == nullptr) return;
    if (static_cast<int>(level) < static_cast<int>(threshold_)) return;
    (*sink_) << level_name(level) << ' ' << message << '\n';
}

LogLevel log_level_from_string(std::string_view s) noexcept {
    const std::string norm = lower(s);
    if (norm == "trace") return LogLevel::Trace;
    if (norm == "debug") return LogLevel::Debug;
    if (norm == "info")  return LogLevel::Info;
    if (norm == "warn")  return LogLevel::Warn;
    if (norm == "error") return LogLevel::Error;
    return LogLevel::Info;
}

std::string format_connection_serial(std::uint32_t n) {
    char out[7];
    out[6] = '\0';
    for (int i = 5; i >= 0; --i) {
        out[i] = kB36[n % 36u];
        n /= 36u;
    }
    return std::string(out, 6);
}

std::string format_entry_serial(std::uint32_t n) {
    char buf[9];
    std::snprintf(buf, sizeof(buf), "%08u",
                  static_cast<unsigned>(n % 100000000u));
    return std::string(buf, 8);
}

std::string format_alias_field(std::string_view alias) {
    constexpr std::size_t kAliasWidth = 10;
    if (alias.size() >= kAliasWidth) {
        return std::string(alias.data(), kAliasWidth);
    }
    std::string out(alias.data(), alias.size());
    out.append(kAliasWidth - alias.size(), ' ');
    return out;
}

std::string format_log_timestamp() {
    auto w = openads::platform::now_local();
    int ms = static_cast<int>(w.ms_of_day % 1000);
    if (ms < 0) ms = 0;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s %s.%03d",
                  w.date.c_str(), w.time.c_str(), ms);
    return buf;
}

std::string format_log_prefix(std::string_view conn_serial,
                              std::uint32_t    entry_serial,
                              std::uint32_t    seq,
                              std::string_view timestamp) {
    std::string ts{timestamp};
    if (ts.empty()) ts = format_log_timestamp();
    std::string conn;
    if (conn_serial.size() >= 6) {
        conn.assign(conn_serial.data(), 6);
    } else {
        conn.assign(6 - conn_serial.size(), '0');
        conn.append(conn_serial);
    }
    return conn + ' ' + format_entry_serial(entry_serial) + ' ' +
           format_entry_serial(seq) + ' ' + ts;
}

std::uint32_t next_audit_seq() {
    return g_next_seq.fetch_add(1);
}

std::string make_connection_serial() {
    ensure_seed();
    std::uint32_t n = g_conn_seed + g_next_conn.fetch_add(1);
    return format_connection_serial(n);
}

bool audit_details_enabled() {
    if (g_details == Details::On)  return true;
    if (g_details == Details::Off) return false;
    if (client_setting_truthy("OPENADS_RESOLVE_VERBOSE",
                              "resolve_verbose")) return true;
    const std::string n = lower(client_setting("OPENADS_LOG", "log"));
    return n == "debug" || n == "trace";
}

void set_audit_console(std::ostream* sink) { g_console = sink; }

void set_audit_file(std::ostream* sink) {
    g_file = sink;
    g_file_sink_set = true;
}

void set_audit_details_enabled(bool on) {
    g_details = on ? Details::On : Details::Off;
}

namespace {
std::atomic<bool> g_logging_on{false};
std::recursive_mutex g_diagnostic_mu;
std::atomic<std::uint64_t> g_diagnostic_epoch{0};
} // namespace

DiagnosticGuard::DiagnosticGuard()
    : lock_(g_diagnostic_mu), enabled_(g_logging_on.load()) {}
void set_logging_enabled(bool on) {
    std::lock_guard<std::recursive_mutex> lock(g_diagnostic_mu);
    if (g_logging_on.exchange(on) != on) ++g_diagnostic_epoch;
}
bool logging_enabled() { return g_logging_on.load(); }
std::uint64_t diagnostic_epoch() { return g_diagnostic_epoch.load(); }

std::string diagnostic_label(std::string_view value, std::string_view scope) {
    if (value.empty()) return "-";
    if (!logging_enabled()) return "<masked>";
    // Serialize only label allocation, never a database/network operation.
    static std::mutex mu;
    static std::uint64_t epoch = 0;
    static unsigned long long serial = 0;
    static std::map<std::pair<std::string, std::string>, std::string> names;
    std::lock_guard<std::mutex> lock(mu);
    const auto now = diagnostic_epoch();
    if (epoch != now) { names.clear(); serial = 0; epoch = now; }
    std::string normalized(value);
    for (auto& ch : normalized) {
        if (ch == '\\') ch = '/';
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    auto key = std::make_pair(std::string(scope), normalized);
    auto it = names.find(key);
    if (it != names.end()) return it->second;
    if (names.size() >= 16384) return "TBL_OVERFLOW";
    return names.emplace(std::move(key), "TBL_" + std::to_string(++serial)).first->second;
}

void reset_audit_config() {
    g_console = nullptr;
    g_file = nullptr;
    g_file_sink_set = false;
    g_details = Details::Auto;
    g_remote_asked.clear();
    g_next_seq.store(1);
}

void write_audit(AuditKind       kind,
                 std::string_view conn_serial,
                 std::uint32_t    entry_serial,
                 std::uint32_t    seq,
                 std::string_view message,
                 std::string_view timestamp,
                 std::string_view alias) {
    DiagnosticGuard guard;
    if (!guard) return;
    if (kind == AuditKind::Detail && !audit_details_enabled()) return;
    if (seq == 0) seq = next_audit_seq();
    std::string line =
        format_log_prefix(conn_serial, entry_serial, seq, timestamp);
    if (!alias.empty()) {
        line.push_back(' ');
        line.append(format_alias_field(alias));
    }
    line.push_back(' ');
    line.append(message.data(), message.size());
    line.push_back('\n');
    std::lock_guard<std::mutex> lk(g_audit_mu);
    write_console_line(line);
    if (kind != AuditKind::Detail) write_file_line(line);
}

void write_remote_open_audit(std::string_view asked_name,
                            std::string_view alias) {
    DiagnosticGuard guard;
    if (!guard) return;
    static std::string id = make_connection_serial();
    static std::atomic<std::uint32_t> n{1};
    std::string key = norm_audit_path(asked_name);
    {
        std::lock_guard<std::mutex> lk(g_audit_mu);
        const auto epoch = diagnostic_epoch();
        if (g_remote_asked_epoch != epoch) {
            g_remote_asked.clear();
            g_remote_asked_epoch = epoch;
        }
        if (g_remote_asked.size() < 16384 &&
            !g_remote_asked.insert(std::move(key)).second) return;
    }
    std::string msg = "RESOLVED=\"(remote)\" ASKED=\"";
    msg += diagnostic_label(asked_name, id);
    msg += "\" VIA=REMOTE";
    write_audit(AuditKind::Resolved, id, n.fetch_add(1),
                next_audit_seq(), msg, {}, alias);
}

void write_local_access_audit(std::string_view op, std::string_view path) {
    static std::string id = make_connection_serial();
    static std::atomic<std::uint32_t> n{1};
    std::string msg = "LOCALACCESS=\"";
    msg.append(op.data(), op.size());
    msg += "\" ASKED=\"";
    msg += diagnostic_label(path, id);
    msg += "\" VIA=LOCAL MODE=LOG";
    write_audit(AuditKind::Resolved, id, n.fetch_add(1),
                next_audit_seq(), msg);
}

void write_connected_audit(std::string_view data_dir, bool remote) {
    static std::string id = make_connection_serial();
    static std::atomic<std::uint32_t> n{1};
    std::string msg = "CONNECTED=\"";
    msg += diagnostic_label(data_dir, id);
    msg += remote ? "\" VIA=REMOTE" : "\" VIA=LOCAL";
    write_audit(AuditKind::Connected, id, n.fetch_add(1),
                next_audit_seq(), msg);
}

} // namespace openads::util

// C-linkage accessors so the C TUs (ace_stdcall_x86.c) honor the same
// master switch without including util/log.h.
extern "C" int  oads_logging_enabled(void) {
    return openads::util::logging_enabled() ? 1 : 0;
}
extern "C" int oads_diagnostic_begin(void) {
    openads::util::g_diagnostic_mu.lock();
    if (openads::util::logging_enabled()) return 1;
    openads::util::g_diagnostic_mu.unlock();
    return 0;
}
extern "C" void oads_diagnostic_end(void) {
    openads::util::g_diagnostic_mu.unlock();
}
extern "C" void oads_set_logging(int on) {
    openads::util::set_logging_enabled(on != 0);
}
