#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace llt {

template <typename T, std::size_t Capacity>
class SpscRing {
    static_assert(Capacity >= 2, "Capacity must be at least 2");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
    SpscRing() = default;
    SpscRing(const SpscRing&) = delete;
    SpscRing& operator=(const SpscRing&) = delete;

    bool push(const T& item) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto next = increment(head);
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& out) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        const auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        out = buffer_[tail];
        tail_.store(increment(tail), std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empty() const noexcept {
        return tail_.load(std::memory_order_acquire) == head_.load(std::memory_order_acquire);
    }

private:
    static constexpr std::size_t increment(std::size_t idx) noexcept {
        return (idx + 1) & (Capacity - 1);
    }

    alignas(64) std::array<T, Capacity> buffer_{};
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

} // namespace llt
