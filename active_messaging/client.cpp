// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2012-2013 Hartmut Kaiser
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/lexical_cast.hpp>
#include <boost/iostreams/stream.hpp>

#include "client.hpp"
#include "container_device.hpp"
#include "portable_binary_oarchive.hpp"

///////////////////////////////////////////////////////////////////////////////
// client

void client::start()
{
    for (boost::uint64_t i = 0; i < nodes_.size(); ++i)
    {
        asio_tcp::resolver resolver(server_.get_io_service());
        asio_tcp::resolver::query query(asio_tcp::v4(), nodes_[i].first,
            boost::lexical_cast<std::string>(nodes_[i].second));
        asio_tcp::resolver::iterator iterator = resolver.resolve(query);

        std::shared_ptr<client_connection>
            conn(new client_connection(server_, *this));

        for (boost::uint64_t i = 0; i < 64; ++i)
        {
            boost::system::error_code ec;
            asio::connect(conn->get_socket(), iterator, ec);
            if (!ec) break;

            // Otherwise, we sleep and try again.
            std::chrono::milliseconds period(100);
            std::this_thread::sleep_for(period);
        }

        conn->get_socket().set_option(asio_tcp::socket::reuse_address(true));
        conn->get_socket().set_option(asio_tcp::socket::linger(true, 0));

        // Note that if we had multiple I/O threads, we would have to lock
        // before touching the map.
        asio_tcp::endpoint ep = conn->get_remote_endpoint();
        BOOST_ASSERT(connections_.count(ep) == 0);

        connections_[ep] = conn;
    }
}

std::vector<char>* client::serialize_message(std::shared_ptr<task> t)
{
    std::vector<char>* raw_msg = new std::vector<char>();

    typedef container_device<std::vector<char> > io_device_type;
    boost::iostreams::stream<io_device_type> io(*raw_msg);

    task* raw_t = t.get();

    {
        portable_binary_oarchive archive(io);
        archive & raw_t;
    }

    return raw_msg;
}

///////////////////////////////////////////////////////////////////////////////
// client_connection

client_connection::~client_connection()
{
    // Ensure a graceful shutdown.
    if (socket_.is_open())
    {
        boost::system::error_code ec;
        socket_.shutdown(asio_tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }
}

void client_connection::async_write(task const& t)
{
    std::shared_ptr<task> t_ptr(t.clone());

    server_.get_task_queue().push(
        new std_function_task(boost::bind(&client_connection::async_write_task
                                        , shared_from_this(), t_ptr)));
}

void client_connection::async_write_task(std::shared_ptr<task> t)
{
    std::shared_ptr<std::vector<char> >
        out_buffer(client_.serialize_message(t));

    std::shared_ptr<boost::uint64_t> 
        out_size(new boost::uint64_t(out_buffer->size()));

    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&*out_size, sizeof(*out_size)));
    buffers.push_back(boost::asio::buffer(*out_buffer));

    boost::asio::async_write(socket_, buffers,
        boost::bind(&client_connection::handle_write
                  , shared_from_this()
                  , boost::asio::placeholders::error
                  , out_size
                  , out_buffer));
}
