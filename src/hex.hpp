#pragma once
#include <cstdlib>
#include <string>
#include <utility>
#include <boost/algorithm/hex.hpp>

template<typename Input>
inline std::string to_hex(Input first, Input last)
{
    using namespace std;
    string hex;
    boost::algorithm::hex(first, last, back_inserter(hex));
    return std::move(hex);
}

template<typename Range>
inline std::string to_hex(const Range& range)
{
    using namespace std;
    string hex;
    boost::algorithm::hex(begin(range), end(range), back_inserter(hex));
    return std::move(hex);
}

template<typename Input>
inline std::string to_hex_n(Input first, std::size_t size)
{
    using namespace std;
    string hex;
    boost::algorithm::hex(first, first + size, back_inserter(hex));
    return std::move(hex);
}
