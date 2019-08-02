#pragma once
#include <event.h>

template <typename V>
class observable
{
private:
    V value;

public:
    observable() = default;
    template <typename U>
    observable(U&& u) : value(std::forward<U>(u))
    {
    }
    observable(const observable&) = delete;
    observable(observable&&) = default;
    ~observable() {}

    EVENT_SENDER_E(changed, observable<V>&, const V&)

public:
    template <typename U>
    observable& operator=(U&& u)
    {
        value = std::forward<U>(u);
        on_changed(*this, value);
        return *this;
    }
    observable& operator=(const observable&) = delete;
    observable& operator=(observable&&) = default;

    constexpr operator V() const { return value; }

    friend constexpr bool operator==(observable& o1, observable& o2) { return o1.value == o2.value; }
    friend constexpr bool operator!=(observable& o1, observable& o2) { return o1.value != o2.value; }

    //区分前置与后置++/--
    observable& operator++()
    {
        on_changed(*this, ++value);
        return *this;
    }
    observable& operator++(int)
    {
        on_changed(*this, ++value);
        return *this;
    }
    observable& operator--()
    {
        on_changed(*this, ++value);
        return *this;
    }
    observable& operator--(int)
    {
        on_changed(*this, ++value);
        return *this;
    }
    template <typename U>
    observable& operator+=(U&& u)
    {
        on_changed(*this, value += std::forward<U>(u));
        return *this;
    }
    template <typename U>
    observable& operator-=(U&& u)
    {
        on_changed(*this, value -= std::forward<U>(u));
        return *this;
    }
};
