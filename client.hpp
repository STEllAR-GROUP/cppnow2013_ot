// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2012-2013 Hartmut Kaiser
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(CPPNOW_677B0E1E_FFE2_4EFF_91AE_F6F99282EE99)
#define CPPNOW_677B0E1E_FFE2_4EFF_91AE_F6F99282EE99

#include <atomic>
#include <thread>

#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "server.hpp"

typedef std::vector<std::pair<std::string, boost::uint16_t> > node_vector;

struct client_connection; 

typedef std::map<asio_tcp::endpoint, std::shared_ptr<client_connection> >
    client_connection_map;

struct client 
{
  private:
    server& server_; // The local server instance.

    node_vector nodes_;

    client_connection_map connections_;

  public:
    client(server& s, std::string const& host, boost::uint16_t port)
      : server_(s)
      , nodes_()
      , connections_()
    {
        nodes_.push_back(std::make_pair(host, port));
    }

    client(server& s, node_vector const& nodes)
      : server_(s) 
      , nodes_(nodes)
    {} 

    /// Connect to all nodes. 
    void start(); 

    /// Serializes a task object into a message.
    std::vector<char>* serialize_message(std::shared_ptr<task> t);

    client_connection_map& get_connections()
    {
        return connections_;
    }
};

struct client_connection : std::enable_shared_from_this<client_connection>
{
  private:
    server& server_;
    client& client_;

    asio_tcp::socket socket_;

  public: 
    client_connection(server& s, client& c)
      : server_(s)
      , client_(c)
      , socket_(s.get_io_service())
    {}

    ~client_connection();

    asio_tcp::socket& get_socket()
    {
        return socket_;
    }

    asio_tcp::endpoint get_remote_endpoint() const
    {
        return socket_.remote_endpoint();
    }

    /// Asynchronously write a task to the socket. 
    void async_write(task const& t); 

    /// This function is scheduled in the task_queue by async_write. It does
    /// the actual work of serializing the task.
    void async_write_task(std::shared_ptr<task> t);

    /// Write handler.
    void handle_write(
        boost::system::error_code const& error
      , std::shared_ptr<boost::uint64_t> out_size 
      , std::shared_ptr<std::vector<char> > out_buffer
        )
    {
        BOOST_ASSERT(!error);
    }
};
 
#endif

