#include "buffer.h"

std::size_t buffer_t::size()
{
    return _data.size();
}

char* buffer_t::data()
{
    return _data.data();
}

std::size_t& buffer_t::offset()
{
    return _offset;
}

void buffer_t::resize(std::size_t new_size)
{
    _data.resize(new_size);
}

void buffer_t::clear()
{
    _offset = 0;
    _data.clear();
    _data.shrink_to_fit();
}

char buffer_t::operator[](std::size_t index)
{
    return _data[index];
}

void buffer_t::push_back(char c)
{
    if (_offset + 1 > _data.size())
    {
        _data.resize(_offset + 1);
    }
    _data[_offset] = c;
    _offset++;
}
