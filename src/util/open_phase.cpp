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
        if (std::fseek(sink, 0, SEEK_END) != 0 || std::ftell(sink) < 0 || std::ftell(sink) > cap - 4096) {
            on.store(false, std::memory_order_relaxed); return;
        }
        // Numeric columns only. Driver phases are nested within engine/reply.
        char row[4096];
        const int rc = std::snprintf(row, sizeof(row),
            "OPEN80 session=%llu request=%llu drivers=%u reply_opcode=%u total_us=%llu engine_us=%llu handle_us=%llu share_us=%llu header_us=%llu fields_us=%llu reply_build_us=%llu reply_submit_us=%llu",
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
        if (rc < 0 || static_cast<unsigned>(rc) >= sizeof(row)) {
            on.store(false, std::memory_order_relaxed); return;
        }
        std::size_t used = static_cast<std::size_t>(rc);
        static const char* coarse[] = {"book", "discover", "warm", "twin", "explicit", "metadata", "reset"};
        static const char* detail[] = {"case", "dir_pass", "dir_entry", "tag_list", "tag_open", "bag_wait", "bind_wait", "bind_work", "heal", "reindex"};
        static const char* lanes[] = {"other", "auto", "refresh"};
        for (unsigned i = 0; i < fine_count; ++i) {
            const char* lane = i < coarse_count ? "stage" : lanes[(i - coarse_count) / detail_count];
            const char* label = i < coarse_count ? coarse[i] : detail[(i - coarse_count) % detail_count];
            const int n = std::snprintf(row + used, sizeof(row) - used,
                " %s_%s_us=%llu %s_%s_n=%llu", lane, label,
                static_cast<unsigned long long>(value_.fine_us[i]), lane, label,
                static_cast<unsigned long long>(value_.fine_n[i]));
            if (n < 0 || static_cast<std::size_t>(n) >= sizeof(row) - used - 1) {
                on.store(false, std::memory_order_relaxed); return;
            }
            used += static_cast<std::size_t>(n);
        }
        row[used++] = '\n';
        if (std::fwrite(row, 1, used, sink) != used || std::fflush(sink) != 0)
            on.store(false, std::memory_order_relaxed);
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
namespace {
bool slot_for(Sample* s, Metric m, unsigned& slot) noexcept {
    if (!s || s->lane == Lane::None) return false;
    const unsigned v = static_cast<unsigned>(m);
    if (v >= static_cast<unsigned>(Metric::Count)) return false;
    slot = v < coarse_count ? v : coarse_count +
        static_cast<unsigned>(s->lane) * detail_count + v - coarse_count;
    return true;
}
}
Context::Context(Lane lane) noexcept : sample_(current) {
    if (sample_ && (lane == Lane::Other || sample_->lane != Lane::None)) {
        previous_ = sample_->lane; sample_->lane = lane;
    } else sample_ = nullptr;
}
Context::~Context() noexcept { if (sample_) sample_->lane = previous_; }
Detail::Detail(Metric metric) noexcept : sample_(current) {
    if (!slot_for(sample_, metric, slot_)) { sample_ = nullptr; return; }
    ++sample_->fine_n[slot_];
    start_ = std::chrono::steady_clock::now();
}
Detail::~Detail() noexcept { stop(); }
void Detail::stop() noexcept {
    if (sample_) { sample_->fine_us[slot_] += elapsed(start_); sample_ = nullptr; }
}
void count(Metric metric) noexcept {
    unsigned slot;
    if (slot_for(current, metric, slot)) ++current->fine_n[slot];
}
void driver_open() noexcept { if (current) ++current->drivers; }
} // namespace
