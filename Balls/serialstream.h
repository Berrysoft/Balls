#pragma once
#include <fstream>

class serialstream
{
private:
    std::fstream base_stream;

public:
    serialstream(const std::wstring& name = {}, std::ios_base::openmode mode = std::ios::in | std::ios::out) : base_stream(name, mode | std::ios::binary) {}
    serialstream(const serialstream&) = delete;
    serialstream(serialstream&& stream) : base_stream(std::move(stream.base_stream)) {}

    serialstream& operator=(const serialstream&) = delete;
    serialstream& operator=(serialstream&& stream)
    {
        base_stream = std::move(stream.base_stream);
        return *this;
    }

    ~serialstream() { base_stream.close(); }

    template <typename T, typename>
    friend serialstream& operator<<(serialstream& stream, T&& obj);
    template <typename T, typename>
    friend serialstream& operator>>(serialstream& stream, T&& obj);
};

template <typename T, typename = std::enable_if_t<std::is_trivial<std::remove_reference_t<T>>::value>>
inline serialstream& operator<<(serialstream& stream, T&& obj)
{
    T value = std::forward<T>(obj);
    stream.base_stream.write((const char*)&value, sizeof(value));
    return stream;
}

template <typename T, typename = std::enable_if_t<std::is_trivial<std::remove_reference_t<T>>::value>>
inline serialstream& operator>>(serialstream& stream, T&& obj)
{
    stream.base_stream.read((char*)&obj, sizeof(std::remove_reference_t<T>));
    return stream;
}
