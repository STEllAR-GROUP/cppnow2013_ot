// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2012-2013 Hartmut Kaiser
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/iostreams/stream.hpp>

#include "server.hpp"
#include "container_device.hpp"
#include "portable_binary_iarchive.hpp"

///////////////////////////////////////////////////////////////////////////////
// server

void server::start() 
{
    // Precondition: the server is in a stopped state.

    // Set up acceptor. 
    acceptor_.set_option(asio_tcp::acceptor::reuse_address(true));
    acceptor_.set_option(asio_tcp::acceptor::linger(true, 0));

    // Start the execution thread.
    exec_thread_ = std::thread(boost::bind(&server::exec_loop
                                         , boost::ref(*this)));

    // Start accepting connections.
    async_accept();
}

void server::stop()
{
    // Precondition: start() has been called.

    // Tell the execution thread to stop.
    stop_flag_.store(true);

    io_service_.stop();

    if (exec_thread_.joinable())
        exec_thread_.join();
}

void server::run()
{
    // Keep io_service::run() from returning.
    asio::io_service::work work(io_service_);

    io_service_.run(); 
}

void server::async_accept()
{
    std::shared_ptr<server_connection> conn;
    conn.reset(new server_connection(*this));

    acceptor_.async_accept(conn->get_socket(),
        boost::bind(&server::handle_accept
                  , boost::ref(*this)
                  , asio::placeholders::error
                  , conn));
}

void server::handle_accept(
    boost::system::error_code const& error
  , std::shared_ptr<server_connection> conn
    )
{
    if (!error)
    {
        // If there was no error, then we need to insert conn into the
        // the connection table, but first we want to set up the next
        // async_accept.
        std::shared_ptr<server_connection> old_conn(conn);

        conn.reset(new server_connection(*this));

        acceptor_.async_accept(conn->get_socket(),
            boost::bind(&server::handle_accept
                      , boost::ref(*this)
                      , asio::placeholders::error
                      , conn));

        // Note that if we had multiple I/O threads, we would have to lock
        // before touching the map.
        asio_tcp::endpoint ep = old_conn->get_remote_endpoint();
        BOOST_ASSERT(connections_.count(ep) == 0);

        connections_[ep] = old_conn;

        // If main exists, do we have enough clients to run it? 
        if (main_ && (connections_.size() >= wait_for_))
        {
            // Instead of running main_ directly, we will stick it in the
            // task queue.
            task_queue_.push(new std_function_task(main_));
        }

        // Start reading.
        old_conn->async_read();
    } 
}

void server::exec_loop()
{
    while (!stop_flag_.load())
    {
        ///////////////////////////////////////////////////////////////////////
        // First, we look for pending tasks to execute. 
        task* raw_t = 0;

        if (task_queue_.pop(raw_t))
        {
            BOOST_ASSERT(raw_t);

            boost::scoped_ptr<task> t(raw_t); 

            (*t)();
        }

        ///////////////////////////////////////////////////////////////////////
        // If we can't find any work, we try to find a message to deserialize.
        // FIXME
        std::vector<char>* raw_msg_ptr = 0;

        if (message_queue_.pop(raw_msg_ptr))
        {
            BOOST_ASSERT(raw_msg_ptr);

            boost::scoped_ptr<std::vector<char> > raw_msg(raw_msg_ptr); 

            task_queue_.push(deserialize_message(*raw_msg));
        }
    }
}

task* server::deserialize_message(
    std::vector<char>& raw_msg
    )
{
    typedef container_device<std::vector<char> > io_device_type;
    boost::iostreams::stream<io_device_type> io(raw_msg);

    task* raw_t = 0;

    {
        portable_binary_iarchive archive(io);
        archive & raw_t;
    }

    BOOST_ASSERT(raw_t);

    return raw_t; 
}

///////////////////////////////////////////////////////////////////////////////
// server_connection

server_connection::~server_connection()
{
    // Ensure a graceful shutdown.
    if (socket_.is_open())
    {
        boost::system::error_code ec;
        socket_.shutdown(asio_tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }
}

void server_connection::async_read()
{
    BOOST_ASSERT(in_buffer_ == 0);
    in_size_ = 0;
    in_buffer_ = new std::vector<char>();

    asio::async_read(socket_,
        asio::buffer(&in_size_, sizeof(in_size_)),
            boost::bind(&server_connection::handle_read_size
                      , shared_from_this()
                      , asio::placeholders::error));
}

void server_connection::handle_read_size(boost::system::error_code const& error)
{
    // Precondition: in_size is the size of the next message, in bytes.
    BOOST_ASSERT(!error);
    BOOST_ASSERT(in_buffer_);

    (*in_buffer_).resize(in_size_);

    asio::async_read(socket_,
        asio::buffer(*in_buffer_),
            boost::bind(&server_connection::handle_read_data
                      , shared_from_this()
                      , asio::placeholders::error));
}

void server_connection::handle_read_data(boost::system::error_code const& error)
{
    // Precondition: in_buffer contains a valid message. 
    BOOST_ASSERT(!error);

    std::vector<char>* raw_msg = 0;
    std::swap(in_buffer_, raw_msg);

    server_.get_message_queue().push(raw_msg);

    // Start the next read.
    async_read();
}

