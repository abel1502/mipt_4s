#pragma once
#include "general.h"
#include <vector>
#include <map>
#include <type_traits>
#include <cstdarg>



namespace helpers {


template <typename K, typename V>
class CounterMap {
    static_assert(std::is_arithmetic_v<V>);

public:
    CounterMap() = default;

    inline V get(const K &key) {
        add(key);

        return buf[key];
    }

    inline void add(const K &key) {
        if (!buf.contains(key)) {
            buf[key] = next++;
        }
    }

    inline void reset() {
        buf.clear();
        next = {};
    }

protected:
    std::map<K, V> buf{};
    V next{};

};


class FlagMap {
public:
    FlagMap() = default;

    inline bool get(size_t idx) const {
        if (idx >= buf.size()) {
            return false;
        }

        return buf[idx];
    }

    inline void set(size_t idx, bool value = true) {
        ensure(idx);

        buf[idx] = value;
    }

    inline void reset() {
        buf.clear();
    }

protected:
    std::vector<bool> buf{};


    inline void ensure(size_t idx) {
        if (idx >= buf.size()) {
            buf.resize(idx + 1);
        }
    }

};


std::string vsprintfxx(const char *fmt, va_list args);

inline std::string sprintfxx(const char *fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    std::string result = vsprintfxx(fmt, args);
    va_end(args);

    return result;
}


std::string htmlEncode(const std::string_view &data);


}
