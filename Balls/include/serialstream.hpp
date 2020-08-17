#ifndef SERIALSTREAM_HPP
#define SERIALSTREAM_HPP

#include <nowide/filesystem.hpp>
#include <nowide/fstream.hpp>

class serialstream
{
private:
    nowide::fstream base_stream;

public:
    serialstream(const nowide::filesystem::path& name = {}, std::ios_base::openmode mode = std::ios::in | std::ios::out) : base_stream(name, mode | std::ios::binary) {}
    serialstream(const serialstream&) = delete;
    serialstream(serialstream&& stream) : base_stream(std::move(stream.base_stream)) {}

    serialstream& operator=(const serialstream&) = delete;
    serialstream& operator=(serialstream&& stream)
    {
        base_stream = std::move(stream.base_stream);
        return *this;
    }

    ~serialstream() { base_stream.close(); }

    serialstream& write(const void* data, std::size_t size)
    {
        base_stream.write((const char*)data, size);
        return *this;
    }
    serialstream& real(void* data, std::size_t size)
    {
        base_stream.read((char*)data, size);
        return *this;
    }

    template <typename T, typename = std::enable_if_t<std::is_trivial<std::remove_reference_t<T>>::value>>
    inline serialstream& operator<<(T&& obj)
    {
        base_stream.write((const char*)&obj, sizeof(std::remove_reference_t<T>));
        return *this;
    }

    template <typename T, typename = std::enable_if_t<std::is_trivial<std::remove_reference_t<T>>::value>>
    inline serialstream& operator>>(T&& obj)
    {
        base_stream.read((char*)&obj, sizeof(std::remove_reference_t<T>));
        return *this;
    }
};

#endif // !SERIALSTREAM_HPP
