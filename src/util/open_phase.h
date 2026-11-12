#pragma once
#include <cstdint>
#include <chrono>
#include <string>
namespace openads::util::open_phase {
enum class Part { Engine, Handle, Share, Header, Fields, Reply, Submit };
// Independent admin opt-in. Never enables general diagnostic logging.
void configure(const std::string& file) noexcept;
bool enabled() noexcept;
// Coarse stages are disjoint where placed. Detail metrics are nested within
// the owning stage and lane: other reply work, twin auto-open, explicit refresh.
enum class Metric { Book, Discover, Warm, Twin, Explicit, Metadata, Reset,
                    Case, DirPass, DirEntry, TagList, TagOpen, BagWait,
                    BindWait, BindWork, Heal, Reindex, Count };
constexpr unsigned coarse_count = 7;
constexpr unsigned detail_count = static_cast<unsigned>(Metric::Count) - coarse_count;
constexpr unsigned fine_count = coarse_count + 3 * detail_count;
enum class Lane { Other = 0, Auto = 1, Explicit = 2, None = 3 };
struct Sample {
    std::uint64_t session = 0, request = 0;
    std::uint64_t us[7]{};
    unsigned drivers = 0, opcode = 0;
    std::uint64_t fine_us[fine_count]{}, fine_n[fine_count]{};
    Lane lane = Lane::None;
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
class Context {
public:
    explicit Context(Lane lane) noexcept;
    ~Context() noexcept;
private:
    Sample* sample_ = nullptr;
    Lane previous_ = Lane::None;
};
class Detail {
public:
    explicit Detail(Metric metric) noexcept;
    ~Detail() noexcept;
    void stop() noexcept;
private:
    Sample* sample_ = nullptr;
    unsigned slot_ = 0;
    std::chrono::steady_clock::time_point start_{};
};
void count(Metric metric) noexcept;
void driver_open() noexcept;
} // namespace
