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

#include <chrono>
#include <mutex>
#include <cstdio>
#include "clany_defs.h"


_CLANY_BEGIN
class ScopeTimer {
    using sys_clock = chrono::system_clock;
    using seconds   = chrono::duration<double>;

public:
    explicit ScopeTimer(int precision = 3)
        :start(sys_clock::now()), prec(precision) {}

    ~ScopeTimer()
    {
        seconds delta = sys_clock::now() - start;
        printf("Time elapsed: %.*fs\n", prec, delta.count());
    }

private:
    sys_clock::time_point start;
    int prec;
};

class CPUTimer {
    using sys_clock = chrono::system_clock;
    using seconds   = chrono::duration<double>;

public:
    explicit CPUTimer(bool is_started = true)
        : start_point(), since_begin(seconds::zero()),
          is_stopped(false), need_print(true)
    {
        if (is_started) start();
    }

    double elapsed(const string& title = "Since begin", int precision = 3)
    {
        if (is_stopped) {
            printf("Paused\n");
            return -1;
        }

        auto now     = sys_clock::now();
        since_begin += now - start_point;
        start_point  = now;

        if(need_print) printf("%s: %.*fs\n", title.c_str(),
                              precision, since_begin.count());

        return since_begin.count();
    }

    double delta(const string& title = "Since last check", int precision = 3)
    {
        if (is_stopped) {
            printf("Paused\n");
            return -1;
        }

        auto now      = sys_clock::now();
        seconds delta = now - start_point;
        since_begin  += delta;
        start_point   = now;

        if (need_print) printf("%s: %.*fs\n", title.c_str(),
                               precision, delta.count());

        return delta.count();
    }

    void start()
    {
        is_stopped  = false;
        start_point = sys_clock::now();
    }

    void pause()
    {
        if (!is_stopped) {
            is_stopped    = true;
            seconds delta = sys_clock::now() - start_point;
            since_begin  += delta;
        }
    }

    void resume()
    {
        start();
    }

    void reset(bool is_started = true)
    {
        since_begin = seconds::zero();
        is_stopped  = false;
        if (is_started) start();
    }

    void printOff() { need_print = false; }
    void printOn()  { need_print = true; }

private:
    sys_clock::time_point start_point;
    seconds since_begin;
    bool    is_stopped;
    bool    need_print;
};
_CLANY_END

#endif // CLANY_TIMER_HPP