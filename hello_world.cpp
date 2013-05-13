// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2012-2013 Hartmut Kaiser
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/scoped_ptr.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/base_object.hpp>

#include "server.hpp"
#include "client.hpp"
#include "portable_binary_iarchive.hpp"
#include "portable_binary_oarchive.hpp"

namespace po = boost::program_options;

server* server_ = 0; 
client* client_ = 0; 

struct hello_world_task : task
{
    void operator()()
    {
        std::cout << "hello world\n";
    }

    task* clone() const
    {
        return new hello_world_task;
    }

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int)
    {
        ar & boost::serialization::base_object<task>(*this);
    }
};

BOOST_CLASS_EXPORT_GUID(hello_world_task, "hello_world_task");
BOOST_CLASS_TRACKING(hello_world_task, boost::serialization::track_never);

void hello_world_main()
{
    hello_world_task t;

    for (auto node : client_->get_connections()) 
        node.second->async_write(t);
}

int main(int argc, char** argv)
{
    // Parse command line.
    po::variables_map vm;

    po::options_description
        cmdline("Usage: hello_world --port <port> "
                " --remote-host <hostname> --remote-port <port> [options]");

    cmdline.add_options()
        ( "help,h"
        , "print out program usage (this message)")

        ( "port"
        , po::value<std::string>()->default_value("9000")
        , "TCP port to listen on")

        ( "remote-host"
        , po::value<std::string>()->default_value("localhost")
        , "hostname or IP to connect to")

        ( "remote-port"
        , po::value<std::string>()->default_value("9001")
        , "TCP port to connect to")
    ;

    po::store(po::command_line_parser(argc, argv).options(cmdline).run(), vm);

    po::notify(vm);

    // Print help screen.
    if (vm.count("help"))
    {
        std::cout << cmdline;
        return 1;
    }

    boost::uint16_t port = boost::lexical_cast<boost::uint16_t>(
        vm["port"].as<std::string>());

    std::string remote_host = vm["remote-host"].as<std::string>();
    boost::uint16_t remote_port = boost::lexical_cast<boost::uint16_t>(
        vm["remote-port"].as<std::string>());

    server_ = new server(port, hello_world_main);
    client_ = new client(*server_, remote_host, remote_port);

    boost::scoped_ptr<server> s(server_);
    boost::scoped_ptr<client> c(client_);

    server_->start();
    client_->start();

    server_->run();

    server_->stop();

    return 0;
}

