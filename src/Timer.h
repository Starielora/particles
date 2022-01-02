#pragma once

#include <chrono>
#include <vector>

template<class Duration>
struct Timer final
{
    const std::chrono::steady_clock::time_point t1;
    std::vector<float>& vals;
    Timer(std::vector<float>& vals) : t1(std::chrono::steady_clock::now()), vals(vals)
    {}
    ~Timer()
    {
        const auto t2 = std::chrono::steady_clock::now();
        const auto time = std::chrono::duration_cast<Duration>(t2 - t1).count();
        vals.erase(vals.begin());
        vals.push_back(time);
    }
};
