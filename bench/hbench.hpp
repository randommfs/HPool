#include <chrono>
#include <format>
#include <iostream>
#include <numeric>
#include <ranges>
#include <string_view>

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
  _bench_entry(const _bench_entry &) = default;
  _bench_entry &operator=(const _bench_entry &) = default;
};

std::chrono::time_point<s_clock> _start, _end;
size_t _entry_count;
_bench_entry *_entries;

void _allocate_slots(int count) {
  _entries = (_bench_entry *)malloc(count * sizeof(struct _bench_entry));
}

std::string _format_time(size_t ms) {
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

std::string _format_time() {
  return _format_time(
      std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start)
          .count());
}

std::string _format_time(std::chrono::time_point<s_clock> &_s,
                         std::chrono::time_point<s_clock> &_e) {
  return _format_time(
      std::chrono::duration_cast<std::chrono::milliseconds>(_e - _s).count());
}

void _report_start(std::string_view statement) {
  std::cout
      << CGREEN << "[ START ] " << CRESET
      << "Starting benchmark of statement [ " << statement << " ]" << std::endl;
}

void _report_start_n(std::string_view name) {
  std::cout << CGREEN << "[ START ] " << CRESET
                              << "Starting benchmark " << name << std::endl;
}

void _report_start(int slot) { _report_start_n(_entries[slot].name); }

void _print_time(std::string_view statement, std::string time) {
  std::cout
      << CGREEN << "[  END  ] " << CRESET
      << "Finished benchmark of statement [ " << statement << " ]\n"
      << CGREENB << "[ TOTAL ] " << time << CRESET << std::endl;
}

void _print_time_n(std::string_view name, std::string time) {
  std::cout
      << CGREEN << "[  END  ] " << CRESET << "Finished benchmark " << name
      << "\n"
      << CGREENB << "[ TOTAL ] " << time << CRESET << std::endl;
}

void _print_time(std::string_view statement) {
  _print_time(statement, _format_time());
}

void _print_time(std::string_view statement, size_t ms) {
  _print_time(statement, _format_time(ms));
}

void _print_time(int slot) {
  _print_time_n(_entries[slot].name, _format_time());
}
void _print_time(std::string_view statement,
                 std::chrono::time_point<s_clock> &_s,
                 std::chrono::time_point<s_clock> &_e) {
  _print_time(
      statement,
      _format_time(
          std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start)
              .count()));
}

void _print_time(int slot, std::chrono::time_point<s_clock> &_s,
                 std::chrono::time_point<s_clock> &_e) {
  _print_time_n(
      _entries[slot].name,
      _format_time(
          std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start)
              .count()));
}
} // namespace _hbench_internal

#define ALLOCATE_SLOTS(num)                                                    \
  static_assert(std::is_integral_v<decltype(num)>);                            \
  _hbench_internal::_allocate_slots(num);

#define BENCH(statement)                                                       \
  _hbench_internal::_report_start(#statement);                                 \
  _hbench_internal::_start = _hbench_internal::s_clock::now();                 \
  statement;                                                                   \
  _hbench_internal::_end = _hbench_internal::s_clock::now();                   \
  _hbench_internal::_print_time(#statement);

#define BENCH_AVG(statement, iterations)                                       \
  {                                                                            \
    std::chrono::time_point<_hbench_internal::s_clock> _starts[iterations],    \
        _ends[iterations];                                                     \
    _hbench_internal::_report_start(#statement);                               \
    for (size_t i = 0; i < iterations; ++i) {                                  \
      _starts[i] = _hbench_internal::s_clock::now();                           \
      statement;                                                               \
      _ends[i] = _hbench_internal::s_clock::now();                             \
    }                                                                          \
    size_t avg[iterations];                                                    \
    for (size_t i = 0; i < iterations; ++i) {                                  \
      avg[i] = std::chrono::duration_cast<std::chrono::milliseconds>(          \
                   _ends[i] - _starts[i])                                      \
                   .count();                                                   \
    }                                                                          \
    size_t avg_time =                                                          \
        std::accumulate(std::begin(avg), std::end(avg), 1) / iterations;       \
    _hbench_internal::_print_time(#statement, avg_time);                       \
  }

#define BENCHN(statement, name)                                                \
  _hbench_internal::_report_start_n(name);                                     \
  _hbench_internal::_start = _hbench_internal::s_clock::now();                 \
  statement;                                                                   \
  _hbench_internal::_end = _hbench_internal::s_clock::now();                   \
  _hbench_internal::_print_time_n(name, _hbench_internal::_format_time());

#define BENCHN_AVG(statement, name, iterations)                                \
  {                                                                            \
    std::chrono::time_point<_hbench_internal::s_clock> _starts[iterations],    \
        _ends[iterations];                                                     \
    _hbench_internal::_report_start_n(name);                                   \
    for (size_t i = 0; i < iterations; ++i) {                                  \
      _starts[i] = _hbench_internal::s_clock::now();                           \
      statement;                                                               \
      _ends[i] = _hbench_internal::s_clock::now();                             \
    }                                                                          \
    size_t avg[iterations];                                                    \
    for (size_t i = 0; i < iterations; ++i) {                                  \
      avg[i] = std::chrono::duration_cast<std::chrono::milliseconds>(          \
                   _ends[i] - _starts[i])                                      \
                   .count();                                                   \
    }                                                                          \
    size_t avg_time =                                                          \
        std::accumulate(std::begin(avg), std::end(avg), 1) / iterations;       \
    _hbench_internal::_print_time_n(name,                                      \
                                    _hbench_internal::_format_time(avg_time)); \
  }

#define SET_SLOT_NAME(slot, namestr)                                           \
  static_assert(std::is_integral_v<decltype(slot)>);                           \
  _hbench_internal::_entries[slot - 1].name = namestr;

#define BENCH_SLOT(slot, statement)                                            \
  {                                                                            \
    static_assert(std::is_integral_v<decltype(slot)>);                         \
    assert(!_hbench_internal::_entries[slot - 1].working);                     \
    std::chrono::time_point<_hbench_internal::s_clock> _start, _end;           \
    _hbench_internal::_entries[slot - 1].working = true;                       \
    _hbench_internal::_report_start(slot - 1);                                 \
    _start = _hbench_internal::s_clock::now();                                 \
    statement;                                                                 \
    _end = _hbench_internal::s_clock::now();                                   \
    _hbench_internal::_print_time(slot - 1, _start, _end);                     \
    _hbench_internal::_entries[slot - 1].working = false;                      \
  }
