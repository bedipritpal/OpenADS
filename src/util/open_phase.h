#pragma once
#include <cstdint>
#include <chrono>
#include <string>
namespace openads::util::open_phase {
enum class Part { Engine, Handle, Share, Header, Fields, Reply, Submit };
// Independent admin opt-in. Never enables general diagnostic logging.
void configure(const std::string& file) noexcept;
bool enabled() noexcept;
struct Sample {
    std::uint64_t session = 0, request = 0;
    std::uint64_t us[7]{};
    unsigned drivers = 0, opcode = 0;
};
class Request {
public:
    explicit Request(std::uint64_t session, bool is_open) noexcept;
    ~Request() noexcept;
    void result(unsigned opcode) noexcept;
private:
    Sample value_{};
    Sample* previous_ = nullptr;
    bool active_ = false;
    std::chrono::steady_clock::time_point start_{};
};
class Phase {
public:
    explicit Phase(Part part) noexcept;
    ~Phase() noexcept;
    void stop() noexcept;
private:
    Sample* sample_ = nullptr;
    Part part_;
    std::chrono::steady_clock::time_point start_{};
};
void driver_open() noexcept;
} // namespace
