#include <cstdlib>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/system/error_code.hpp>
#include <spdlog_easy.hpp>
#include "session.h"
#include "buffer.h"
#include "hex.hpp"

namespace asio = boost::asio;
using io_context_t = asio::io_context;
using error_code_t = boost::system::error_code;
using acceptor_t = asio::ip::tcp::acceptor;
using socket_t = asio::ip::tcp::socket;
using yield_context_t = asio::yield_context;
using endpoint_t = asio::ip::tcp::endpoint;
using address_t = asio::ip::address;

using namespace std;

void on_go(shared_ptr<session_t> session, yield_context_t yield)
{
    error_code_t ec;
    auto& read_buffer = session->read_buffer();
    auto& write_buffer = session->write_buffer();
    while (true)
    {
        while (true)
        {
            async_read(*session, 1, ec, yield);
            if (ec)
            {
                LOG(info, ec.message());
                return;
            }
            if (read_buffer[read_buffer.offset() - 1] == '\n')
            {
                auto hex = to_hex_n(read_buffer.data(), read_buffer.offset());
                LOG(info, hex);
                break;
            }
        }

        write_buffer.resize(read_buffer.offset());
        copy_n(read_buffer.data(), read_buffer.offset(), write_buffer.data());
        //copy_n(read_buffer.data(), read_buffer.offset(), back_inserter(write_buffer));
        read_buffer.clear();
        while (true)
        {
            async_write(*session, 1, ec, yield);
            if (ec)
            {
                LOG(info, ec.message());
                return;
            }
            if (write_buffer.size() == write_buffer.offset())
            {
                auto hex = to_hex_n(write_buffer.data(), write_buffer.offset());
                LOG(info, hex);
                break;
            }
        }
        write_buffer.clear();
    }
}

int main()
{
    auto& log_config = spdlog::easy::get_config();
    log_config.file_size = 5;
    log_config.func_size = 10;
    spdlog::easy::init();
    io_context_t io_context;
    acceptor_t acceptor(io_context,
        endpoint_t(address_t::from_string("0.0.0.0"), 12345));

    asio::spawn(io_context,
        [&](yield_context_t yield)
    {
        while (true)
        {
            try
            {
                auto session = make_shared<session_t>(io_context);
                auto& socket = session->socket();
                error_code_t ec;
                acceptor.async_accept(socket, yield[ec]);
                if (!ec)
                {
                    auto remote_endpoint = socket.remote_endpoint();
                    auto address = remote_endpoint.address().to_string();
                    auto port = remote_endpoint.port();
                    LOG(info, "address:{0} port:{1}", address, port);
                    go(session, on_go);
                }
            }
            catch (const std::exception& e)
            {
                LOG(info, e.what());
            }
        }
    });

    io_context.run();
    return EXIT_SUCCESS;
}
