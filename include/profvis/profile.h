#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

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
        size_t          id;
        Time            begin;
        Time            end;

        Events          events;
    };

    int                 max_depth() const   { return max_depth_; }
    Time                max_time() const    { return max_time_; }

    std::string         name(const Event& e) const      { return name(e.id); }
    std::string         name(size_t id) const           { return names[id]; }

    size_t              id(std::string name) const      { return ids.find(name)->second; }

    std::vector<Events>                         events;     // one per rank
    std::vector<std::string>                    names;
    std::unordered_map<std::string,size_t>      ids;

    int                         max_depth_;
    Time                        max_time_;
};

Profile::Time   parse_time(std::string stamp);
Profile         read_profile(std::string fn);

Profile         read_caliper(std::string fn);

}
