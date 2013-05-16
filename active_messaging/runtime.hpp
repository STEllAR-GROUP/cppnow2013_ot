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
#include <boost/lexical_cast.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/lockfree/queue.hpp> 

#include "asio_aliases.hpp"
#include "action.hpp"

struct connection; 

struct runtime
{
    typedef std::map<asio_tcp::endpoint, std::shared_ptr<connection> >
        connection_map;

  private:
    asio::io_service io_service_;

    asio_tcp::acceptor acceptor_;

    connection_map connections_;

    std::thread exec_thread_;

    boost::lockfree::queue<std::vector<char>*> parcel_queue_;
    boost::lockfree::queue<std::function<void(runtime&)>*> local_queue_;

    std::atomic<bool> stop_flag_;

    std::function<void(runtime&)> main_;

    // The # of clients to wait for before executing main_.
    boost::uint64_t wait_for_; 

  public:
    runtime(
        std::string port
      , std::function<void(runtime&)> f = std::function<void(runtime&)>() 
      , boost::uint64_t wait_for = 1 
        )
      : io_service_()
      , acceptor_(io_service_, asio_tcp::endpoint(asio_tcp::v4(),
            boost::lexical_cast<boost::uint16_t>(port))) 
      , connections_()
      , exec_thread_()
      , parcel_queue_(64) // Pre-allocate some nodes.
      , local_queue_(64) // Pre-allocate some nodes.
      , stop_flag_(false)
      , main_(f)
      , wait_for_(wait_for) 
    {
        BOOST_ASSERT(wait_for != 0);
    }

    ~runtime()
    {
        stop();
    }

    asio::io_service& get_io_service()
    {
        return io_service_;
    }

    boost::lockfree::queue<std::vector<char>*>& get_parcel_queue()
    {
        return parcel_queue_;
    }

    boost::lockfree::queue<std::function<void(runtime&)>*>& get_local_queue()
    {
        return local_queue_;
    }

    connection_map& get_connections()
    {
        return connections_;
    }

    /// Launch the execution thread. Then, start accepting connections.
    void start(); 

    /// Stop the I/O service and execution thread. 
    void stop();

    /// Accepts connections and parcels until stop() is called. 
    void run();

    /// Connect to another node. 
    std::shared_ptr<connection> connect(
        std::string host
      , std::string port
        ); 

    /// Asynchronously accept a new connection.
    void async_accept();

    /// Handler for new connections.
    void handle_accept(
        error_code const& error
      , std::shared_ptr<connection> conn
        );

  private:
    friend struct connection;

    /// Execute actions until stop() is called. 
    void exec_loop();

    /// Serializes a action object into a parcel.
    std::vector<char>* serialize_parcel(action const& act);

    /// Deserializes a parcel into a action object.
    action* deserialize_parcel(std::vector<char>& raw_msg);
};

struct connection : std::enable_shared_from_this<connection>
{
  private:
    runtime& runtime_;

    asio_tcp::socket socket_;

    boost::uint64_t in_size_;
    std::vector<char>* in_buffer_;

  public:
    connection(runtime& s)
      : runtime_(s)
      , socket_(s.get_io_service())
      , in_size_(0)
      , in_buffer_()
    {}

    ~connection();

    asio_tcp::socket& get_socket()
    {
        return socket_;
    }

    asio_tcp::endpoint get_remote_endpoint() const
    {
        return socket_.remote_endpoint();
    }

    /// Asynchronously read a parcel from the socket.
    void async_read();

    /// Handler for the parcel size.
    void handle_read_size(error_code const& error);

    /// Handler for the data.
    void handle_read_data(error_code const& error);

    /// Asynchronously write a action to the socket. 
    void async_write(action const& act)
    {
        std::function<void(error_code const&)> h;
        async_write(act, h);
    } 

    /// Asynchronously write a action to the socket. 
    void async_write(
        action const& act
      , std::function<void(error_code const&)> handler
        ); 

    /// This function is scheduled in the local_queue by async_write. It does
    /// the actual work of serializing the action.
    void async_write_worker(
        std::shared_ptr<action> act
      , std::function<void(error_code const&)> handler
        );

    /// Write handler.
    void handle_write(
        error_code const& error
      , std::shared_ptr<boost::uint64_t> out_size 
      , std::shared_ptr<std::vector<char> > out_buffer
      , std::function<void(error_code const&)> handler
        )
    {
        if (handler)
            handler(error);
    }
};

#endif

