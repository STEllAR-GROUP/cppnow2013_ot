// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "coordinate.hpp"

namespace asio = boost::asio;
typedef boost::asio::ip::tcp asio_tcp;

int main()
{
    asio_tcp::iostream s("localhost", "2000");

    {
        boost::archive::binary_oarchive sa(s);

        coordinate c{17, 42};
        sa << c; 
    }
}

