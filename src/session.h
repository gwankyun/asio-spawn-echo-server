#pragma once
#include <cstdlib>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include "buffer.h"

class session_t : public std::enable_shared_from_this<session_t>
{
    using io_context_t = boost::asio::io_context;
    using socket_t = boost::asio::ip::tcp::socket;
    using strand_t = boost::asio::strand<boost::asio::io_context::executor_type>;
public:
    session_t(io_context_t& io_context);
    ~session_t();

    socket_t& socket();
    buffer_t& read_buffer();
    buffer_t& write_buffer();
    strand_t& strand();

private:
    socket_t _socket;
    strand_t _strand;
    buffer_t _read_buffer;
    buffer_t _write_buffer;
};

std::size_t async_read(
    session_t& session,
    std::size_t size,
    boost::system::error_code& ec,
    boost::asio::yield_context& yield);

std::size_t async_write(
    session_t& session,
    std::size_t size,
    boost::system::error_code& ec,
    boost::asio::yield_context& yield);

template<typename F>
inline void go(std::shared_ptr<session_t> session, F f)
{
    using yield_context_t = boost::asio::yield_context;
    boost::asio::spawn(session->strand(), 
        [session, f](yield_context_t yield)
    {
        f(session, yield);
    });
}
