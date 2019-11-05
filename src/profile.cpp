#include <profvis/profile.h>
#include <iterator>
#include <algorithm>

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
    Profile::Time                   max_time = 0;
    while(std::getline(in, line))
    {
        std::istringstream ins(line);
        int rank;
        std::string time_stamp;
        std::string name;

        ins >> rank >> time_stamp >> name;
        auto time = parse_time(time_stamp);

        if (time > max_time)
            max_time = time;

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

    return profile;
}

profvis::Profile
profvis::
read_caliper(std::string fn)
{
    Profile profile;

    zstr::ifstream      in(fn);

    using size_type = std::string::size_type;

    std::unordered_map<std::string, std::tuple<size_type,size_type>>     columns;
    std::string         header;
    std::getline(in, header);

    bool parsing = false;
    size_type last = 0;
    for (size_type i = 0; i < header.size(); ++i)
    {
        if (header[i] != ' ')
        {
            if (!parsing)
            {
                parsing = true;
                columns[trim(header.substr(last, i - last - 1))] = std::make_tuple(last, i - last - 1);
                last = i;
            }
        } else
        {
            if (parsing)
                parsing = false;
        }
    }
    columns[trim(header.substr(last))] = std::make_tuple(last, std::string::npos);
    bool has_mpi_rank = columns.find("mpi.rank") != columns.end();

    std::vector<std::tuple<int, size_t, size_t, bool>> events;

    auto get_column = [&columns](const std::string& line, const char* name)
    {
        size_type from, to;
        std::tie(from,to) = columns[name];

        return trim(line.substr(from,to));
    };

    std::string         line;
    while(std::getline(in, line))
    {
        std::istringstream iss(line);

        int    rank      = !has_mpi_rank ? 0 : std::stoi(get_column(line, "mpi.rank"));
        size_t offset    = std::stol(get_column(line, "time.offset"));
        size_t duration  = std::stol(get_column(line, "time.inclusive.duration"));

        std::string name = get_column(line, "annotation");
        auto last_slash = name.find_last_of("/");
        name = name.substr(last_slash + 1);

        size_t id = profile.names.size();
        auto it = profile.ids.find(name);
        if (it != profile.ids.end())
            id = it->second;
        else
        {
            profile.names.push_back(name);
            profile.ids[name] = id;
        }

        events.emplace_back(rank, offset - duration, id, true);
        events.emplace_back(rank, offset, id, false);
    }

    std::sort(events.begin(), events.end());

    std::vector<Profile::Event*>    event_stack;
    size_t          max_depth = 0;
    Profile::Time   max_time = 0;

    for (auto& event : events)
    {
        bool begin; size_t time; size_t id; size_t rank;
        std::tie(rank,time,id,begin) = event;

        if (time > max_time)
            max_time = time;

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

    return profile;
}
