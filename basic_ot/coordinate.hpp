// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(CPPNOW_677B0E1E_FFE2_4EFF_91AE_F6F99282EE99)
#define CPPNOW_677B0E1E_FFE2_4EFF_91AE_F6F99282EE99

#include <boost/cstdint.hpp>

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

#endif

