/////////////////////////////////////////////////////////////////////////////////
// The MIT License(MIT)
// 
// Copyright (c) 2014 Tiangang Song
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////

#ifndef CLANY_TIMER_HPP
#define CLANY_TIMER_HPP

#if defined(WIN32) || defined(_WIN32) || defined(__MINGW32__)
#  include <ctime>
#  define CLOCK()  clock()
#  define TICK_FACTOR  CLOCKS_PER_SEC
#else
#  include <unistd.h>
#  include <sys/times.h>
#  define CLOCK()  ::times(NULL)
#  define TICK_FACTOR  TICK_PER_SECOND
   const int TICK_PER_SECOND = ::sysconf(_SC_CLK_TCK);
#endif

#include <iostream>
#include <iomanip>

#include "clany_defs.h"


_CLANY_BEGIN
class ScopeTimer
{
public:
    explicit ScopeTimer(int precision = 3) :timer_ (CLOCK()), precision_ (precision) {}

    ~ScopeTimer()
    {
        timer_ = CLOCK() - timer_;
        cout << setprecision(precision_) << "Time elapsed: "
             << static_cast<double>(timer_) / TICK_FACTOR << "s"
             << endl;
    }

private:
    clock_t timer_;
    int precision_;
};


class CPUTimer
{
public:
    explicit CPUTimer(bool is_started = true) :
        timer_(0), duration_berfore_(0), is_stopped_(false)
    {
        if (is_started) {
            start();
        }
    }

    double elapsed(string process_name = "Since begin", int precision = 3)
    {
        if (is_stopped_) {
            cout << "Paused" << endl;
            return -1;
        }

        double last_duration = static_cast<double>(CLOCK() - timer_) / TICK_FACTOR;
        double since_begin = last_duration + duration_berfore_;
        cout << setprecision(precision) << process_name.c_str() << ": "
             << since_begin << "s" << endl;
             
        timer_ = CLOCK();
        duration_berfore_ += last_duration;
        
        return since_begin;
    }
    
    double delta(string process_name = "Since last check", int precision = 3)
    {
        if (is_stopped_) {
            cout << "Paused" << endl;
            return -1;
        }

        double last_duration = static_cast<double>(CLOCK() - timer_) / TICK_FACTOR;
        cout << setprecision(precision) << process_name.c_str() << ": "
             << last_duration << "s" << endl;

        timer_ = CLOCK();
        duration_berfore_ += last_duration;
        
        return last_duration;
    }

    void start()
    {
        is_stopped_ = false;
        timer_ = CLOCK();
    }

    void pause()
    {
        if (!is_stopped_) {
            is_stopped_ = true;
            duration_berfore_ += static_cast<double>(CLOCK() - timer_) / TICK_FACTOR;
        }
    }

    void resume()
    {
        start();
    }

    void reset(bool is_started = true)
    {
        duration_berfore_ = 0;
        is_stopped_ = false;
        if (is_started) {
            start();
        }
    }

private:
    clock_t timer_;
    double duration_berfore_;
    bool is_stopped_;
};
_CLANY_END

#endif // CLANY_TIMER_HPP
