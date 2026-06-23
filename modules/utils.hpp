#ifndef MY_UTILS
#define MY_UTILS

// Reconstructed common-utilities header.
//
// The original `modules/utils.hpp` was not part of the published source drop
// (the README notes the GUI and "some other minor parts" were omitted). This
// file reconstructs the helper API that randomizer.cpp / itemrando.cpp depend
// on, inferred from their call sites. Behaviour of the `random::` helpers and
// the exact whitespace handling of `parse::` are best-effort reconstructions;
// they compile and run, but the RNG sequence is not guaranteed to be
// bit-identical to the original binary.

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cctype>
#include <charconv>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <tuple>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <random>
#include <type_traits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <ctime>

// libstdc++ does not expose the float-suffixed math overloads in namespace std
// (MSVC, the original build platform, does). The C++ standard requires them, so
// pull the C-library versions in to keep randomizer.cpp portable.
#if defined(__GLIBCXX__) || defined(__GLIBC__)
namespace std { using ::powf; using ::ceilf; }
#endif

// ---------------------------------------------------------------------------
// Hashing
// ---------------------------------------------------------------------------

// Deterministic 32-bit string hash (FNV-1a). Used to salt the per-map seed so
// each map randomizes independently. The exact constants only matter for
// reproducing a given seed's output, not for correctness.
inline uint32_t hash_str_uint32(std::string_view str){
    uint32_t hash = 2166136261u;        // FNV offset basis
    for(unsigned char c : str){
        hash ^= c;
        hash *= 16777619u;              // FNV prime
    }
    return hash;
}

// ---------------------------------------------------------------------------
// Vector helpers
// ---------------------------------------------------------------------------

// True if `value` compares equal to any element of container `c`.
template<typename Container, typename Value>
bool vector_contains(const Container& c, const Value& value){
    return std::find(std::begin(c), std::end(c), value) != std::end(c);
}

// Find the first element equal to `value`, remove it by swapping with the last
// element and popping (O(1), does not preserve order). Returns whether an
// element was removed.
template<typename T, typename Value>
bool vector_find_swap_pop(std::vector<T>& v, const Value& value){
    for(size_t i = 0; i < v.size(); ++i){
        if(v[i] == value){
            v[i] = v.back();
            v.pop_back();
            return true;
        }
    }
    return false;
}

// Remove duplicate elements, preserving the order of first occurrence.
template<typename T>
void vector_remove_duplicates(std::vector<T>& v){
    std::vector<T> out;
    out.reserve(v.size());
    for(const auto& x : v){
        if(!vector_contains(out, x)) out.push_back(x);
    }
    v = std::move(out);
}

// ---------------------------------------------------------------------------
// File helpers
// ---------------------------------------------------------------------------

// Read an entire file into a string (binary). Returns "" on failure; callers
// rely on read_param_file() treating an empty buffer as a no-op.
inline std::string get_file_contents_binary(const std::filesystem::path& path){
    std::ifstream file(path, std::ios::binary);
    if(!file){
        std::cout << "Failed to open file: " << path << "\n";
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Write a string to a file (binary), creating parent directories as needed.
inline bool write_to_file_binary(const std::filesystem::path& path, const std::string& data){
    if(path.has_parent_path()){
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    std::ofstream file(path, std::ios::binary);
    if(!file){
        std::cout << "Failed to write file: " << path << "\n";
        return false;
    }
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    return true;
}

// Open an input file stream. Returns whether the file was opened.
inline bool open_file(std::ifstream& file, const std::filesystem::path& path){
    file.open(path);
    return file.is_open();
}

// ---------------------------------------------------------------------------
// Time helpers
// ---------------------------------------------------------------------------

// A filename-safe timestamp, e.g. "_20260623_141530".
inline std::string time_string_now(){
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "_%Y%m%d_%H%M%S", &tm);
    return std::string(buf);
}

// Simple elapsed-time helper. passed() returns milliseconds since construction.
struct Stopwatch{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    long long passed() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    }
    void reset(){ start = std::chrono::steady_clock::now(); }
};

// ---------------------------------------------------------------------------
// Parsing helpers
// ---------------------------------------------------------------------------

namespace parse{
    // Strip leading/trailing ASCII whitespace, returning a view into `s`.
    inline std::string_view trim(std::string_view s){
        size_t b = 0, e = s.size();
        while(b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
        while(e > b && std::isspace(static_cast<unsigned char>(s[e-1]))) --e;
        return s.substr(b, e - b);
    }

    // Split `s` on every occurrence of `delim`. Always returns at least one
    // token (the whole string when no delimiter is present). The returned
    // string_views point into `s`, so `s` must outlive them; callers can mutate
    // tokens with remove_prefix/remove_suffix to trim brackets etc.
    inline std::vector<std::string_view> split(std::string_view s, char delim){
        std::vector<std::string_view> out;
        size_t start = 0;
        while(true){
            size_t pos = s.find(delim, start);
            if(pos == std::string_view::npos){
                out.push_back(s.substr(start));
                break;
            }
            out.push_back(s.substr(start, pos - start));
            start = pos + 1;
        }
        return out;
    }

    // Parse a number out of `s` into `out`, ignoring surrounding whitespace and
    // any trailing characters (lenient prefix parse, like the data files use).
    // Supports integral, floating-point and enum targets. Returns success.
    template<typename T>
    bool read_var(std::string_view s, T& out){
        size_t b = 0, e = s.size();
        while(b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
        while(e > b && std::isspace(static_cast<unsigned char>(s[e-1]))) --e;
        const char* first = s.data() + b;
        const char* last  = s.data() + e;
        if(first >= last) return false;

        if constexpr (std::is_floating_point_v<T>){
            T v{};
            auto r = std::from_chars(first, last, v);
            if(r.ec != std::errc{}) return false;
            out = v;
            return true;
        } else if constexpr (std::is_enum_v<T>){
            std::underlying_type_t<T> v{};
            auto r = std::from_chars(first, last, v);
            if(r.ec != std::errc{}) return false;
            out = static_cast<T>(v);
            return true;
        } else if constexpr (std::is_signed_v<T>){
            long long v{};
            auto r = std::from_chars(first, last, v);
            if(r.ec != std::errc{}) return false;
            out = static_cast<T>(v);
            return true;
        } else {
            unsigned long long v{};
            auto r = std::from_chars(first, last, v);
            if(r.ec != std::errc{}) return false;
            out = static_cast<T>(v);
            return true;
        }
    }
}

// ---------------------------------------------------------------------------
// Randomness helpers (all take a std::mt19937_64 engine by reference)
// ---------------------------------------------------------------------------

// NOTE: named `rng`, not `random`. On Linux/libstdc++ the POSIX `::random()`
// from <cstdlib> is forced into the global namespace (via _GNU_SOURCE in
// libstdc++'s os_defines.h), which collides with a global `namespace random`.
// The original MSVC build has no such symbol. The .cpp files use `rng::`.
namespace rng{
    // Default global engine (used for seed generation).
    inline std::mt19937_64 m_gen{std::random_device{}()};

    // Uniform integer in [min, max].
    template<typename T = int>
    T integer(T min, T max, std::mt19937_64& gen){
        std::uniform_int_distribution<T> dist(min, max);
        return dist(gen);
    }

    // Uniform real in [min, max).
    template<typename T = float>
    T real(T min, T max, std::mt19937_64& gen){
        std::uniform_real_distribution<T> dist(min, max);
        return dist(gen);
    }

    // True with probability chance/out_of.
    inline bool roll(int chance, int out_of, std::mt19937_64& gen){
        if(out_of <= 0) return false;
        std::uniform_int_distribution<int> dist(1, out_of);
        return dist(gen) <= chance;
    }

    // A random valid index into a non-empty container.
    template<typename Container>
    size_t vindex(const Container& c, std::mt19937_64& gen){
        std::uniform_int_distribution<size_t> dist(0, c.size() - 1);
        return dist(gen);
    }

    // A reference to a random element of a non-empty container.
    template<typename Container>
    auto& element(Container& c, std::mt19937_64& gen){
        std::uniform_int_distribution<size_t> dist(0, c.size() - 1);
        return c[dist(gen)];
    }

    // Choose n elements from src. When allow_repeats is false the result holds
    // distinct elements (capped at src.size()); otherwise elements may repeat.
    template<typename T>
    std::vector<T> choose_n_elements(const std::vector<T>& src, size_t n, bool allow_repeats, std::mt19937_64& gen){
        std::vector<T> out;
        if(src.empty()) return out;
        if(allow_repeats){
            out.reserve(n);
            std::uniform_int_distribution<size_t> dist(0, src.size() - 1);
            for(size_t i = 0; i < n; ++i) out.push_back(src[dist(gen)]);
        } else {
            std::vector<T> pool = src;
            n = std::min(n, pool.size());
            out.reserve(n);
            for(size_t i = 0; i < n; ++i){
                std::uniform_int_distribution<size_t> dist(0, pool.size() - 1);
                size_t k = dist(gen);
                out.push_back(pool[k]);
                pool[k] = pool.back();
                pool.pop_back();
            }
        }
        return out;
    }
}

#endif
