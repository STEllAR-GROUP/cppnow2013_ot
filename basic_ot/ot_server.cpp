// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "coordinate.hpp"

namespace asio = boost::asio;
typedef boost::asio::ip::tcp asio_tcp;

int main()
{
    asio::io_service io_service;

    asio_tcp::endpoint endpoint(asio_tcp::v4(), 2000);
    asio_tcp::acceptor acceptor(io_service, endpoint);

    asio_tcp::iostream s;

    acceptor.accept(*s.rdbuf());

    {
        boost::archive::binary_iarchive la(s);

        coordinate c;
        la >> c;

        std::cout << "{" << c.x << ", " << c.y << "}\n";
    }
}

