// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2012-2013 Hartmut Kaiser
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(CPPNOW_3C5121B2_7086_440B_8E4C_D739BA66C4FA)
#define CPPNOW_3C5121B2_7086_440B_8E4C_D739BA66C4FA

#include <atomic>
#include <thread>

#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/lockfree/queue.hpp> // Boost 1.53 (?)

#include "asio_aliases.hpp"
#include "task.hpp"

// NOTE TO SELF: Do a distributed matrix multiply example.
// NOTE TO SELF: Endianness (e.g. Boost Endian).
// NOTE TO SELF: Special asio archive, asio stream as parameter
// to portable_binary_archive.
// NOTE TO SELF: Put everything into a namespace.

struct server_connection; 

typedef std::map<asio_tcp::endpoint, std::shared_ptr<server_connection> >
    server_connection_map;

struct server
{
  private:
    asio::io_service io_service_;

    asio_tcp::acceptor acceptor_;

    server_connection_map connections_;

    std::thread exec_thread_;

    boost::lockfree::queue<std::vector<char>*> message_queue_;
    boost::lockfree::queue<task*> task_queue_;

    std::atomic<bool> stop_flag_;

    std::function<void()> main_;

    // The # of clients to wait for before executing main_.
    boost::uint64_t wait_for_; 

  public:
    server(
        boost::uint16_t port
      , std::function<void()> f = std::function<void()>() 
      , boost::uint64_t wait_for = 1 
        )
      : io_service_()
      , acceptor_(io_service_, asio_tcp::endpoint(asio_tcp::v4(), port)) 
      , connections_()
      , exec_thread_()
      , message_queue_(64) // Pre-allocate some nodes.
      , task_queue_(64) // Pre-allocate some nodes.
      , stop_flag_(false)
      , main_(f)
      , wait_for_(wait_for) 
    {}

    ~server()
    {
        stop();
    }

    asio::io_service& get_io_service()
    {
        return io_service_;
    }

    boost::lockfree::queue<std::vector<char>*>& get_message_queue()
    {
        return message_queue_;
    }

    boost::lockfree::queue<task*>& get_task_queue()
    {
        return task_queue_;
    }

    /// Launch the execution thread. Then, start accepting connections.
    void start(); 

    /// Stop the I/O service and execution thread. 
    void stop();

    /// Accepts connections and messages until stop() is called. 
    void run();

    /// Asynchronously accept a new connection.
    void async_accept();

    /// Handler for new connections.
    void handle_accept(
        boost::system::error_code const& error
      , std::shared_ptr<server_connection> conn
        );

  private:
    /// Executed tasks until stop() is called. 
    void exec_loop();

    /// Deserializes a message into a task object.
    task* deserialize_message(std::vector<char>& raw_msg);
};

struct server_connection : std::enable_shared_from_this<server_connection>
{
  private:
    server& server_;

    asio_tcp::socket socket_;

    boost::uint64_t in_size_;
    std::vector<char>* in_buffer_;

  public:
    server_connection(server& s)
      : server_(s)
      , socket_(s.get_io_service())
      , in_size_(0)
      , in_buffer_()
    {}

    ~server_connection();

    asio_tcp::socket& get_socket()
    {
        return socket_;
    }

    asio_tcp::endpoint get_remote_endpoint() const
    {
        return socket_.remote_endpoint();
    }

    /// Asynchronously read a message from the socket.
    void async_read();

    /// Handler for the message size.
    void handle_read_size(boost::system::error_code const& error);

    /// Handler for the data.
    void handle_read_data(boost::system::error_code const& error);
};

#endif

