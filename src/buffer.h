#pragma once
#include <vector>
#include <cstdlib>

class buffer_t
{
public:
    using value_type = char;
    buffer_t() = default;
    ~buffer_t() = default;

    std::size_t size();
    char* data();
    std::size_t& offset();
    void resize(std::size_t new_size);
    void clear();
    char operator[](std::size_t index);
    void push_back(char c);

private:
    std::vector<char> _data;
    std::size_t _offset = 0;
};
