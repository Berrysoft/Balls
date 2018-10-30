#pragma once
#include <limits>

template <typename T>
class loopvar
{
private:
    T value;
    T min;
    T max;

public:
    loopvar(T value = T(0), T min = T(0), T max = std::numeric_limits<T>::max())
        : value(value), min(min), max(max) {}

    constexpr operator T() const { return value; }
    constexpr operator bool() const { return value; }

    loopvar& operator++()
    {
        if (++value >= max)
        {
            value = min;
        }
        return *this;
    }
    loopvar operator++(int)
    {
        loopvar t = *this;
        operator++();
        return t;
    }
    loopvar& operator--()
    {
        if (--value <= min)
        {
            value = max;
        }
        return *this;
    }
    loopvar operator--(int)
    {
        loopvar t = *this;
        operator--();
        return t;
    }
};
