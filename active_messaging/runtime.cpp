// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2012-2013 Hartmut Kaiser
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/iostreams/stream.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "runtime.hpp"
#include "container_device.hpp"

///////////////////////////////////////////////////////////////////////////////
// runtime

void runtime::start() 
{
    // Set up acceptor. 
    acceptor_.set_option(asio_tcp::acceptor::reuse_address(true));
    acceptor_.set_option(asio_tcp::acceptor::linger(true, 0));

    // Start the execution thread.
    exec_thread_ = std::thread(boost::bind(&runtime::exec_loop
                                         , boost::ref(*this)));

    // Start accepting connections.
    async_accept();
}

void runtime::stop()
{
    // Tell the execution thread to stop.
    stop_flag_.store(true);

    // Destroy the keep-alive work object, which will cause run() to return
    // when all I/O work is done.
    io_service_.stop();
}

void runtime::run()
{
    // Keep io_service::run() from returning.
    asio::io_service::work work(io_service_);

    io_service_.run();

    if (exec_thread_.joinable())
        exec_thread_.join();        
}

std::shared_ptr<connection> runtime::connect(
    std::string host
  , std::string port
    )
{
    asio_tcp::resolver resolver(io_service_);
    asio_tcp::resolver::query query(asio_tcp::v4(), host, port);

    asio_tcp::resolver::iterator it = resolver.resolve(query);
    asio_tcp::resolver::iterator end;

    for (asio_tcp::resolver::iterator i = it; i != end; ++i) 
        if (connections_.count(*i) != 0)
            return connections_[*i];

    std::shared_ptr<connection> conn(new connection(*this));

    // Waits for up to 6.4 seconds (0.001 * 100 * 64) for the runtime to become
    // available.
    for (boost::uint64_t i = 0; i < 64; ++i)
    {
        error_code ec;
        asio::connect(conn->get_socket(), it, ec);
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

    // Start reading.
    conn->async_read();

    return conn;
}

void runtime::async_accept()
{
    std::shared_ptr<connection> conn;
    conn.reset(new connection(*this));

    acceptor_.async_accept(conn->get_socket(),
        boost::bind(&runtime::handle_accept
                  , boost::ref(*this)
                  , asio::placeholders::error
                  , conn));
}

void runtime::handle_accept(
    error_code const& error
  , std::shared_ptr<connection> conn
    )
{
    if (!error)
    {
        // If there was no error, then we need to insert conn into the
        // the connection table, but first we want to set up the next
        // async_accept.
        std::shared_ptr<connection> old_conn(conn);

        conn.reset(new connection(*this));

        acceptor_.async_accept(conn->get_socket(),
            boost::bind(&runtime::handle_accept
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
            // action queue.
            local_queue_.push(new std::function<void(runtime&)>(main_));
        }

        // Start reading.
        old_conn->async_read();
    } 
}

void runtime::exec_loop()
{
    while (!stop_flag_.load())
    {
        ///////////////////////////////////////////////////////////////////////
        // First, we look for pending actions to execute. 
        std::function<void(runtime&)>* act_ptr = 0;

        if (local_queue_.pop(act_ptr))
        {
            BOOST_ASSERT(act_ptr);

            boost::scoped_ptr<std::function<void(runtime&)> > act(act_ptr); 

            (*act)(*this);
        }

        ///////////////////////////////////////////////////////////////////////
        // If we can't find any work, we try to find a parcel to deserialize
        // and execute.
        std::vector<char>* raw_msg_ptr = 0;

        if (parcel_queue_.pop(raw_msg_ptr))
        {
            BOOST_ASSERT(raw_msg_ptr);

            boost::scoped_ptr<std::vector<char> > raw_msg(raw_msg_ptr); 
            boost::scoped_ptr<action> act(deserialize_parcel(*raw_msg));

            (*act)(*this);
        }
    }
}

std::vector<char>* runtime::serialize_parcel(action const& act)
{
    std::vector<char>* raw_msg_ptr = new std::vector<char>();

    typedef container_device<std::vector<char> > io_device_type;
    boost::iostreams::stream<io_device_type> io(*raw_msg_ptr);

    action const* act_ptr = &act;

    {
        boost::archive::binary_oarchive archive(io);
        archive & act_ptr;
    }

    return raw_msg_ptr;
}

action* runtime::deserialize_parcel(std::vector<char>& raw_msg)
{
    typedef container_device<std::vector<char> > io_device_type;
    boost::iostreams::stream<io_device_type> io(raw_msg);

    action* act_ptr = 0;

    {
        boost::archive::binary_iarchive archive(io);
        archive & act_ptr;
    }

    BOOST_ASSERT(act_ptr);

    return act_ptr; 
}

///////////////////////////////////////////////////////////////////////////////
// connection

connection::~connection()
{
    // Ensure a graceful shutdown.
    if (socket_.is_open())
    {
        error_code ec;
        socket_.shutdown(asio_tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }
}

void connection::async_read()
{
    BOOST_ASSERT(in_buffer_ == 0);
    in_size_ = 0;
    in_buffer_ = new std::vector<char>();

    asio::async_read(socket_,
        asio::buffer(&in_size_, sizeof(in_size_)),
            boost::bind(&connection::handle_read_size
                      , shared_from_this()
                      , asio::placeholders::error));
}

void connection::handle_read_size(error_code const& error)
{
    if (error) return;

    BOOST_ASSERT(in_buffer_);

    (*in_buffer_).resize(in_size_);

    asio::async_read(socket_,
        asio::buffer(*in_buffer_),
            boost::bind(&connection::handle_read_data
                      , shared_from_this()
                      , asio::placeholders::error));
}

void connection::handle_read_data(error_code const& error)
{
    if (error) return;

    std::vector<char>* raw_msg = 0;
    std::swap(in_buffer_, raw_msg);

    runtime_.get_parcel_queue().push(raw_msg);

    // Start the next read.
    async_read();
}

void connection::async_write(
    action const& act
  , std::function<void(error_code const&)> handler
    )
{
    std::shared_ptr<action> act_ptr(act.clone());

    runtime_.get_local_queue().push(new std::function<void(runtime&)>(
        boost::bind(&connection::async_write_worker
                  , shared_from_this(), act_ptr, handler)));
}

void connection::async_write_worker(
    std::shared_ptr<action> act
  , std::function<void(error_code const&)> handler
    )
{
    std::shared_ptr<std::vector<char> >
        out_buffer(runtime_.serialize_parcel(*act));

    std::shared_ptr<boost::uint64_t> 
        out_size(new boost::uint64_t(out_buffer->size()));

    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&*out_size, sizeof(*out_size)));
    buffers.push_back(boost::asio::buffer(*out_buffer));

    boost::asio::async_write(socket_, buffers,
        boost::bind(&connection::handle_write
                  , shared_from_this()
                  , boost::asio::placeholders::error
                  , out_size
                  , out_buffer
                  , handler));
}

