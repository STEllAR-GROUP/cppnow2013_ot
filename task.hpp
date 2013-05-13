// Copyright (c) 2012-2013 Bryce Adelstein-Lelbach
// Copyright (c) 2012-2013 Hartmut Kaiser
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(CPPNOW_BAA1C7EE_658B_42B8_900A_73FA5BBED365)
#define CPPNOW_BAA1C7EE_658B_42B8_900A_73FA5BBED365

#include <functional> 

#include <boost/assert.hpp>

struct server;

struct task
{
    virtual ~task() {}

    virtual void operator()() = 0;
    
    virtual task* clone() const = 0;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int) {}
};

/// Wrapper for std::function. Non-serializable.
struct std_function_task : task
{
  private:
    std::function<void()> f_;

  public:
    template <typename S>
    std_function_task(S&& s) : f_(s) {} 

    void operator()()
    {
        BOOST_ASSERT(f_);
        f_();
    }

    task* clone() const
    {
        return new std_function_task(f_);
    }
};

#endif

