// asio-spawn-echo-server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <memory>
#include <utility>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <functional>

#include <cmath>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog_easy.hpp>

using namespace std;

namespace asio = boost::asio;

using io_context_t = asio::io_context;
using tcp_t = asio::ip::tcp;
using socket_t = tcp_t::socket;
using address_t = asio::ip::address;
using endpoint_t = tcp_t::endpoint;
using acceptor_t = tcp_t::acceptor;
using resolver_t = tcp_t::resolver;
using error_code_t = boost::system::error_code;
using yield_context_t = asio::yield_context;

#define NOMINMAX

class session_t : public enable_shared_from_this<session_t>
{
public:
	session_t() = default;
	session_t(shared_ptr<socket_t> socket_);
	session_t(io_context_t &io_context);
	~session_t();

	void run();
	string address();
	uint16_t port();

	shared_ptr<socket_t> socket;

private:

};

string session_t::address()
{
	return socket->remote_endpoint().address().to_string();
}

uint16_t session_t::port()
{
	return socket->remote_endpoint().port();
}

session_t::~session_t()
{
	LOG_INFO("log", "address:{0} port:{1}", address(), port());
}

session_t::session_t(shared_ptr<socket_t> socket_)
{
	socket = socket_;
}

session_t::session_t(io_context_t &io_context)
{
	socket = make_shared<socket_t>(io_context);
}

template<typename Pred>
error_code_t read_util(socket_t &socket, vector<char> &buffer, std::size_t &read_offset, yield_context_t &yield, Pred pred)
{
	error_code_t ec;
	while (true)
	{
		buffer.resize(read_offset + 2);
		auto read_size = socket.async_read_some(
			asio::buffer(buffer.data() + read_offset, 2), yield[ec]);
		if (ec)
		{
			LOG_ERROR("log", ec.message());
			return ec;
		}

		LOG_DEBUG("log", "read_size:{0}", read_size);

		read_offset += read_size;

		if (pred())
		{
			return ec;
		}
	}
}

error_code_t write_n(socket_t &socket, const vector<char> &buffer, std::size_t read_offset, yield_context_t &yield)
{
	error_code_t ec;
	std::size_t write_offset = 0;
	while (true)
	{
		std::size_t size = std::min(read_offset - write_offset, std::size_t(2));
		LOG_DEBUG("log", "size:{0}", size);
		auto write_size = socket.async_write_some(
			asio::buffer(buffer.data() + write_offset, size), yield[ec]);

		if (ec)
		{
			return ec;
		}

		write_offset += write_size;

		LOG_DEBUG("log", "write_size:{0}", write_size);

		LOG_DEBUG("log", "write_offset:{0} read_offset:{1}",
			write_offset,
			read_offset);

		if (write_offset == read_offset)
		{
			return ec;
		}
	}
}

void session_t::run()
{
	auto self(shared_from_this());
	asio::spawn(socket->get_io_context(),
		[this, self](yield_context_t yield)
	{
		error_code_t ec;
		while (true)
		{
			vector<char> buffer;

			std::size_t read_offset = 0;

			ec = read_util(*socket, buffer, read_offset, yield,
				[&buffer, &read_offset]()
			{
				return buffer[read_offset - 1] == '\0';
			});

			if (ec)
			{
				LOG_ERROR("log", ec.message());
				return;
			}

			LOG_INFO("log", "address:{0} ip:{1} server message:{2}",
				address(),
				port(),
				buffer.data());

			ec = write_n(*socket, buffer, read_offset, yield);
			if (ec)
			{
				LOG_ERROR("log", ec.message());
				return;
			}
		}
	});
}

int main()
{
	auto logger = spdlog::stdout_color_mt("log");
	spdlog::set_level(spdlog::level::level_enum::debug);
	io_context_t io_context;
	asio::spawn(io_context, 
		[&](yield_context_t yield) 
	{
		acceptor_t acceptor(io_context, endpoint_t(address_t::from_string("0.0.0.0"), 12500));

		while (true)
		{
			error_code_t ec;
			auto session = make_shared<session_t>(io_context);
			acceptor.async_accept(*(session->socket), yield[ec]);
			if (ec)
			{
				LOG_ERROR("log", ec.message());
				exit(1);
			}

			LOG_INFO("log", "address:{0} port:{1}", 
				session->address(), 
				session->port());

			session->run();
		}
	});
	io_context.run();
	return 0;
}
