#include <profvis/profile.h>
#include <iterator>
#include <algorithm>

#include <iostream>

// Modified from: https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
static inline std::string trim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}


profvis::Profile::Time
profvis::parse_time(std::string stamp)
{
    Profile::Time hours, minutes, seconds;
    Profile::Time result;
    sscanf(stamp.c_str(), "%ld:%ld:%ld.%ld", &hours, &minutes, &seconds, &result);

    seconds += 60*(minutes + 60*hours);
    result +=  1000000 * seconds;

    return result;
}

profvis::Profile
profvis::
read_profile(std::string fn)
{
    Profile profile;

    zstr::ifstream                  in(fn);
    std::string                     line;
    std::vector<Profile::Event*>    event_stack;
    size_t                          max_depth = 0;
    Profile::Time                   max_time = std::numeric_limits<Profile::Time>::min();
    Profile::Time                   min_time = std::numeric_limits<Profile::Time>::max();
    while(std::getline(in, line))
    {
        std::istringstream ins(line);
        int rank;
        std::string time_stamp;
        std::string name;

        ins >> rank >> time_stamp >> name;
        auto time = parse_time(time_stamp);

        if (time > max_time) max_time = time;
        if (time < min_time) min_time = time;

        bool begin;
        if (name[0] == '<')
            begin = true;
        else
            begin = false;
        name = name.substr(1);

        if (!begin)
        {
            if (event_stack.size() > max_depth)
                max_depth = event_stack.size();
            event_stack.back()->end = time;
            event_stack.pop_back();
        } else
        {
            if (rank >= profile.events.size())
                profile.events.resize(rank + 1);

            Profile::Events* level;
            if (event_stack.empty())
                level = &profile.events[rank];
            else
                level = &(event_stack.back()->events);

            size_t id = profile.names.size();
            auto it = profile.ids.find(name);
            if (it != profile.ids.end())
                id = it->second;
            else
            {
                profile.names.push_back(name);
                profile.ids[name] = id;
            }

            level->emplace_back(Profile::Event { id, time, time });
            event_stack.push_back(&level->back());
        }
    }

    profile.max_depth_  = max_depth;
    profile.max_time_   = max_time;
    profile.min_time_   = min_time;

    return profile;
}

profvis::Profile
profvis::
read_caliper(std::string fn, bool mpi_functions)
{
    Profile profile;

    zstr::ifstream      in(fn);

    std::string line;
    std::vector<std::tuple<int, size_t, size_t, bool>> events;
    while(std::getline(in, line))
    {
        std::istringstream iss(line);
        std::string field;
        int rank = 0;
        size_t offset;
        size_t duration;
        int type = -1; size_t id;
        while(std::getline(iss, field, ','))
        {
            auto eq_pos = field.find('=');
            auto name  = field.substr(0,eq_pos);
            auto value = field.substr(eq_pos+1);

            if (name == "mpi.rank")
                rank = std::stoi(value);
            else if (name == "event.end#annotation" || (mpi_functions && name == "event.end#mpi.function"))
            {
                type = 1;
                id = profile.names.size();
                auto it = profile.ids.find(value);
                if (it != profile.ids.end())
                    id = it->second;
                else
                {
                    profile.names.push_back(value);
                    profile.ids[value] = id;
                }
            }
            else if (name == "time.inclusive.duration")
                duration = std::stol(value);
            else if (name == "time.offset")
                offset = std::stol(value);
        }

        if (type == 1)
        {
            events.emplace_back(rank, offset - duration, id, true);
            events.emplace_back(rank, offset, id, false);
        }
    }

    std::sort(events.begin(), events.end());

    std::vector<Profile::Event*>    event_stack;
    size_t          max_depth = 0;
    Profile::Time   max_time = std::numeric_limits<Profile::Time>::min();
    Profile::Time   min_time = std::numeric_limits<Profile::Time>::max();

    for (auto& event : events)
    {
        bool begin; size_t time; size_t id; size_t rank;
        std::tie(rank,time,id,begin) = event;

        if (time > max_time) max_time = time;
        if (time < min_time) min_time = time;

        if (begin)
        {
            if (rank >= profile.events.size())
                profile.events.resize(rank + 1);

            Profile::Events* level;
            if (event_stack.empty())
                level = &profile.events[rank];
            else
                level = &(event_stack.back()->events);

            level->emplace_back(Profile::Event { id, time, time });
            event_stack.push_back(&level->back());
        } else
        {
            if (event_stack.size() > max_depth)
                max_depth = event_stack.size();
            event_stack.back()->end = time;
            event_stack.pop_back();
        }
    }

    profile.max_depth_  = max_depth;
    profile.max_time_   = max_time;
    profile.min_time_   = min_time;

    return profile;
}
