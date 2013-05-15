// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <iostream>
#include <boost/cstdint.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

struct coordinate
{
    boost::uint64_t x;
    boost::uint64_t y; 

    template <typename Archive>
    void serialize(Archive& ar, const unsigned version)
    {
        ar & x;
        ar & y;
    }
};

namespace archive = boost::archive;

int main()
{
    std::stringstream ss;

    {
        archive::text_oarchive sa(ss);

        coordinate c{17, 42};
        sa << c; 
    }

    std::cout << ss.str();

    {
        archive::text_iarchive la(ss);

        coordinate c;
        la >> c;

        std::cout << "{" << c.x << ", " << c.y << "}\n";
    }
}

