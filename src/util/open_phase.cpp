#include "util/open_phase.h"
#include <atomic>
#include <cstdio>
#include <mutex>
namespace openads::util::open_phase {
namespace {
std::atomic<bool> on{false};
std::atomic<std::uint64_t> serial{0};
std::mutex sink_mutex;
std::FILE* sink = nullptr;
constexpr long cap = 8L * 1024L * 1024L;
thread_local Sample* current = nullptr;
std::uint64_t elapsed(std::chrono::steady_clock::time_point start) noexcept {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count());
}
}
void configure(const std::string& file) noexcept {
    try {
        std::lock_guard<std::mutex> lock(sink_mutex);
        on.store(false, std::memory_order_relaxed);
        if (sink) { std::fclose(sink); sink = nullptr; }
        if (file.empty()) return;
        sink = std::fopen(file.c_str(), "ab");
        if (!sink) { std::fprintf(stderr, "Open phase diagnostic unavailable; disabled.\n"); return; }
        on.store(true, std::memory_order_relaxed);
    } catch (...) { on.store(false, std::memory_order_relaxed); }
}
bool enabled() noexcept { return on.load(std::memory_order_relaxed); }
Request::Request(std::uint64_t session, bool is_open) noexcept {
    if (!is_open || !enabled()) return;
    active_ = true; previous_ = current; current = &value_;
    value_.session = session;
    value_.request = serial.fetch_add(1, std::memory_order_relaxed) + 1;
    start_ = std::chrono::steady_clock::now();
}
void Request::result(unsigned opcode) noexcept { if (active_) value_.opcode = opcode; }
Request::~Request() noexcept {
    if (!active_) return;
    const auto total = elapsed(start_);
    current = previous_;
    try {
        std::lock_guard<std::mutex> lock(sink_mutex);
        if (!sink || !enabled()) return;
        if (std::fseek(sink, 0, SEEK_END) != 0 || std::ftell(sink) < 0 || std::ftell(sink) > cap - 600) {
            on.store(false, std::memory_order_relaxed); return;
        }
        // Numeric columns only. Driver phases are nested within engine/reply.
        const int rc = std::fprintf(sink,
            "OPEN79 session=%llu request=%llu drivers=%u reply_opcode=%u total_us=%llu engine_us=%llu handle_us=%llu share_us=%llu header_us=%llu fields_us=%llu reply_build_us=%llu reply_submit_us=%llu\n",
            static_cast<unsigned long long>(value_.session),
            static_cast<unsigned long long>(value_.request), value_.drivers, value_.opcode,
            static_cast<unsigned long long>(total),
            static_cast<unsigned long long>(value_.us[0]),
            static_cast<unsigned long long>(value_.us[1]),
            static_cast<unsigned long long>(value_.us[2]),
            static_cast<unsigned long long>(value_.us[3]),
            static_cast<unsigned long long>(value_.us[4]),
            static_cast<unsigned long long>(value_.us[5]),
            static_cast<unsigned long long>(value_.us[6]));
        if (rc < 0 || std::fflush(sink) != 0) on.store(false, std::memory_order_relaxed);
    } catch (...) { on.store(false, std::memory_order_relaxed); }
}
Phase::Phase(Part part) noexcept : sample_(current), part_(part) {
    if (sample_) start_ = std::chrono::steady_clock::now();
}
Phase::~Phase() noexcept { stop(); }
void Phase::stop() noexcept {
    if (sample_) {
        sample_->us[static_cast<unsigned>(part_)] += elapsed(start_);
        sample_ = nullptr;
    }
}
void driver_open() noexcept { if (current) ++current->drivers; }
} // namespace
