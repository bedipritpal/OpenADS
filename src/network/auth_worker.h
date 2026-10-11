#pragma once
#include "network/auth_attempt.h"
#include "network/auth_result.h"
#include <memory>
#include <string>
namespace openads::network {
class AuthWorker {
public:
    AuthWorker(std::string executable, std::string module);
    bool probe();
    bool authenticate(const AuthAttempt::Challenge&, const std::string& blob,
                      const std::string& context, AuthResult&);
private:
    bool run(const std::string& input, std::string& output);
    std::string executable_,module_;
};
}
