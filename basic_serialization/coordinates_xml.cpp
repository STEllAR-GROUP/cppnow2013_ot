// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <iostream>
#include <boost/cstdint.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

struct coordinate
{
    boost::uint64_t x;
    boost::uint64_t y; 

    template <typename Archive>
    void serialize(Archive& ar, const unsigned version)
    {
        ar & BOOST_SERIALIZATION_NVP(x);
        ar & BOOST_SERIALIZATION_NVP(y);
    }
};

namespace archive = boost::archive;

int main()
{
    std::stringstream ss;

    {
        archive::xml_oarchive sa(ss);

        coordinate c{17, 42};
        sa << BOOST_SERIALIZATION_NVP(c); 
    }

    std::cout << ss.str();

    {
        archive::xml_iarchive la(ss);

        coordinate c;
        la >> BOOST_SERIALIZATION_NVP(c);

        std::cout << "{" << c.x << ", " << c.y << "}\n";
    }
}

