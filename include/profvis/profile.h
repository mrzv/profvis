#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <fstream>

#include <zstr/zstr.hpp>

namespace profvis
{

struct Profile
{
    using   Time  = unsigned long;

    struct Event;
    using  Events = std::vector<Event>;

    struct Event
    {
        std::string     name;
        Time            begin;
        Time            end;

        Events          events;
    };

    int                 max_depth() const   { return max_depth_; }
    Time                max_time() const    { return max_time_; }

    std::vector<Events> events;     // one per rank
    int                 max_depth_;
    Time                max_time_;
};

Profile::Time   parse_time(std::string stamp);
Profile         read_profile(std::string fn);

}
