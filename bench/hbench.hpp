#include <chrono>
#include <iostream>
#include <string_view>
#include <format>

#include <cassert>
#include <syncstream>

namespace _hbench_internal {
    using s_clock = std::chrono::steady_clock;
    static constexpr char CGREEN[] = "\x1b[94m";
    static constexpr char CGREENB[] = "\x1b[32;1m";
    static constexpr char CRESET[] = "\x1b[0m";

    struct _bench_entry {
        std::chrono::time_point<s_clock> _start, _end;
        std::string_view name;
        bool working;

        _bench_entry() = default;
        _bench_entry(const _bench_entry&) = default;
        _bench_entry& operator=(const _bench_entry&) = default;
    };

    std::chrono::time_point<s_clock> _start, _end;
    size_t _entry_count;
    _bench_entry *_entries;

    void _allocate_slots(int count) {
        _entries = (_bench_entry*)malloc(count * sizeof(struct _bench_entry));
    }

    std::string _format_time() {
        size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start).count();
        if (ms < 1000) {
            return std::format("{}ms", ms);
        } else if (ms >= 1000 && ms < 1000 * 60) {
            size_t s = ms / 1000;
            size_t m_ms = ms % 1000;
            return std::format("{}s {}ms", s, m_ms);
        } else if (ms >= 1000 * 60 && ms < 1000 * 60 * 60) {
            size_t m = ms / (1000 * 60);
            size_t s = (ms % (1000 * 60)) / 1000;
            size_t m_ms = ms - m * 60000 - s * 1000;
            return std::format("{}m {}s {}ms", m, s, m_ms);
        }
        return {};
    }

    std::string _format_time(std::chrono::time_point<s_clock>& _s, std::chrono::time_point<s_clock>& _e) {
        size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(_e - _s).count();
        if (ms < 1000) {
            return std::format("{}ms", ms);
        } else if (ms >= 1000 && ms < 1000 * 60) {
            size_t s = ms / 1000;
            size_t m_ms = ms % 1000;
            return std::format("{}s {}ms", s, m_ms);
        } else if (ms >= 1000 * 60 && ms < 1000 * 60 * 60) {
            size_t m = ms / (1000 * 60);
            size_t s = (ms % (1000 * 60)) / 1000;
            size_t m_ms = ms - m * 60000 - s * 1000;
            return std::format("{}m {}s {}ms", m, s, m_ms);
        }
        return {};
    }

    void _report_start(std::string_view statement) {
        std::osyncstream(std::cout) << CGREEN
                  << "[ START ] "
                  << CRESET
                  << "Starting benchmark of statement [ "
                  << statement
                  << " ]"
                  << std::endl;
    }

    void _report_start(int slot) {
        std::osyncstream(std::cout) << CGREEN
                  << "[ START ] "
                  << CRESET
                  << "Starting benchmark "
                  << _entries[slot].name
                  << std::endl;
    }

    void _print_time(std::string_view statement) {
        std::osyncstream(std::cout) << CGREEN
                  << "[  END  ] "
                  << CRESET
                  << "Finished benchmark of statement [ "
                  << statement 
                  << " ]\n"
                  << CGREENB
                  << "[ TOTAL ] "
                  << _format_time()
                  << CRESET
                  << std::endl;
    }

    void _print_time(int slot) {
        std::osyncstream(std::cout) << CGREEN
                  << "[  END  ] "
                  << CRESET
                  << "Finished benchmark "
                  << _entries[slot].name
                  << "\n"
                  << CGREENB
                  << "[ TOTAL ] "
                  << _format_time()
                  << CRESET
                  << std::endl;
    }
    void _print_time(std::string_view statement, std::chrono::time_point<s_clock>& _s, std::chrono::time_point<s_clock>& _e) {
        std::osyncstream(std::cout) << CGREEN
                  << "[  END  ] "
                  << CRESET
                  << "Finished benchmark of statement [ "
                  << statement 
                  << " ]\n"
                  << CGREENB
                  << "[ TOTAL ] "
                  << _format_time(_s, _e)
                  << CRESET
                  << std::endl;
    }

    void _print_time(int slot, std::chrono::time_point<s_clock>& _s, std::chrono::time_point<s_clock>& _e) {
        std::osyncstream(std::cout) << CGREEN
                  << "[  END  ] "
                  << CRESET
                  << "Finished benchmark "
                  << _entries[slot].name
                  << "\n"
                  << CGREENB
                  << "[ TOTAL ] "
                  << _format_time(_s, _e)
                  << CRESET
                  << std::endl;
    }
}

#define ALLOCATE_SLOTS(num) \
        static_assert(std::is_integral_v<decltype(num)>);               \
        _hbench_internal::_allocate_slots(num);

#define BENCH(statement) \
        _hbench_internal::_report_start(#statement);                    \
        _hbench_internal::_start = _hbench_internal::s_clock::now();    \
        statement;                                                      \
        _hbench_internal::_end = _hbench_internal::s_clock::now();      \
        _hbench_internal::_print_time(#statement);

#define BENCHN(statement, name) \
        _hbench_internal::_report_start(#statement);                    \
        _hbench_internal::_start = _hbench_internal::s_clock::now();    \
        statement;                                                      \
        _hbench_internal::_end = _hbench_internal::s_clock::now();      \
        _hbench_internal::_print_time(#statement);

#define SET_SLOT_NAME(slot, namestr) \
        static_assert(std::is_integral_v<decltype(slot)>);              \
        _hbench_internal::_entries[slot - 1].name = namestr;

#define BENCH_SLOT(slot, statement) { \
        static_assert(std::is_integral_v<decltype(slot)>);              \
        assert(!_hbench_internal::_entries[slot - 1].working);          \
        std::chrono::time_point<_hbench_internal::s_clock> _start, _end;\
        _hbench_internal::_entries[slot - 1].working = true;            \
        _hbench_internal::_report_start(slot - 1);                      \
        _start = _hbench_internal::s_clock::now();                      \
        statement;                                                      \
        _end = _hbench_internal::s_clock::now();                        \
        _hbench_internal::_print_time(slot - 1, _start, _end);          \
        _hbench_internal::_entries[slot - 1].working = false;   }