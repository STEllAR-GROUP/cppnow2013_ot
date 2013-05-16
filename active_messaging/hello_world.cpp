// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2012-2013 Hartmut Kaiser
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <condition_variable>

#include <boost/scoped_ptr.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "runtime.hpp"

namespace po = boost::program_options;

struct hello_world_action : action
{
    void operator()(runtime& rt)
    {
        std::cout << "hello world\n";

        rt.stop();
    }

    action* clone() const
    {
        return new hello_world_action;
    }

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int)
    {
        ar & boost::serialization::base_object<action>(*this);
    }
};

BOOST_CLASS_EXPORT_GUID(hello_world_action, "hello_world_action");
BOOST_CLASS_TRACKING(hello_world_action, boost::serialization::track_never);

void hello_world_main(runtime& rt)
{
    hello_world_action t;

    auto conns = rt.get_connections();

    std::shared_ptr<boost::uint64_t> count(new boost::uint64_t(conns.size()));

    for (auto node : conns) 
        node.second->async_write(t,
            [count,&rt](boost::system::error_code const& ec)
            {
                if (--(*count) == 0)
                    rt.stop();
            });
}

int main(int argc, char** argv)
{
    // Parse command line.
    po::variables_map vm;

    po::options_description
        cmdline("Usage: hello_world --port <port> "
                " [--remote-host <hostname> --remote-port <port>]");

    cmdline.add_options()
        ( "help,h"
        , "print out program usage (this message)")

        ( "port"
        , po::value<std::string>()->default_value("9000")
        , "TCP port to listen on")

        ( "remote-host"
        , po::value<std::string>()
        , "hostname or IP to connect to")

        ( "remote-port"
        , po::value<std::string>()
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

    std::string port = vm["port"].as<std::string>();

    std::shared_ptr<runtime> rt;

    if (vm.count("remote-host") || vm.count("remote-port"))
    {
        rt.reset(new runtime(port));

        std::cout << "Running as client, will not execute hello_world_main\n";

        std::string remote_host = "localhost", remote_port = port;

        if (vm.count("remote-host"))
            remote_host = vm["remote-host"].as<std::string>();

        if (vm.count("remote-port"))
            remote_port = vm["remote-port"].as<std::string>();

        rt->connect(remote_host, remote_port);
    }

    else
    {
        rt.reset(new runtime(port, hello_world_main));

        std::cout << "Running as server, will execute hello_world_main\n";
    }

    rt->start();

    rt->run();

    return 0;
}

