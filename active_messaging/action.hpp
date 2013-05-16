// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2012-2013 Hartmut Kaiser
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(CPPNOW_BAA1C7EE_658B_42B8_900A_73FA5BBED365)
#define CPPNOW_BAA1C7EE_658B_42B8_900A_73FA5BBED365

struct runtime;

struct action
{
    virtual ~action() {}

    virtual void operator()(runtime&) = 0;
    
    virtual action* clone() const = 0;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int) {}
};

#endif

