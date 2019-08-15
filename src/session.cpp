#include "session.h"
#include <cstdlib>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/system/error_code.hpp>
#include <spdlog_easy.hpp>

namespace asio = boost::asio;
using io_context_t = asio::io_context;
using error_code_t = boost::system::error_code;
using acceptor_t = asio::ip::tcp::acceptor;
using socket_t = asio::ip::tcp::socket;
using yield_context_t = asio::yield_context;
using endpoint_t = asio::ip::tcp::endpoint;
using address_t = asio::ip::address;
using strand_t = asio::strand<boost::asio::io_context::executor_type>;

using namespace std;

session_t::session_t(io_context_t& io_context)
    :_socket(io_context), _strand(io_context.get_executor())
{
}

session_t::~session_t()
{
    auto remote_endpoint = _socket.remote_endpoint();
    auto address = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    LOG(info, "address:{0} port:{1}", address, port);
}

socket_t& session_t::socket()
{
    return _socket;
}

buffer_t& session_t::read_buffer()
{
    return _read_buffer;
}

buffer_t& session_t::write_buffer()
{
    return _write_buffer;
}

strand_t& session_t::strand()
{
    return _strand;
}

std::size_t async_read(
    session_t& session,
    std::size_t size,
    boost::system::error_code& ec,
    boost::asio::yield_context& yield)
{
    auto& socket = session.socket();
    auto& read_buffer = session.read_buffer();
    auto& read_offset = session.read_buffer().offset();
    if (read_offset + size > read_buffer.size())
    {
        read_buffer.resize(read_offset + size);
    }
    auto read_size = socket.async_read_some(asio::buffer(read_buffer.data() + read_offset, size), yield[ec]);
    if (!ec)
    {
        read_offset += read_size;
    }
    return read_size;
}

std::size_t async_write(
    session_t& session,
    std::size_t size,
    boost::system::error_code& ec,
    boost::asio::yield_context& yield)
{
    auto& socket = session.socket();
    auto& write_buffer = session.write_buffer();
    auto& write_offset = session.write_buffer().offset();
    if (write_offset + size > write_buffer.size())
    {
        write_buffer.resize(write_offset + size);
    }
    auto write_size = socket.async_write_some(asio::buffer(write_buffer.data() + write_offset, size), yield[ec]);
    if (!ec)
    {
        write_offset += write_size;
    }
    return write_size;
}
