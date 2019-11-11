#include <profvis/profile-canvas.h>

void
profvis::ProfileCanvas::
drawContents(NVGcontext* ctx)
{
    NVGcontext* vg = ctx;

    // start-time
    nvgBeginPath(vg);
    nvgMoveTo(vg, init_hoffset, init_voffset);
    nvgLineTo(vg, init_hoffset, init_voffset + base_height() * profile_.events.size() + rank_gap * (profile_.events.size() - 1));
    nvgStrokeColor(vg, ng::Color { 1.f, 1.f, 1.f, 1.f });
    nvgStrokeWidth(vg, 1.);
    nvgStroke(vg);

    // end-time
    nvgBeginPath(vg);
    nvgMoveTo(vg, init_hoffset + width, init_voffset);
    nvgLineTo(vg, init_hoffset + width, init_voffset + base_height() * profile_.events.size() + rank_gap * (profile_.events.size() - 1));
    nvgStrokeColor(vg, ng::Color { 1.f, 1.f, 1.f, 1.f });
    nvgStrokeWidth(vg, 1.);
    nvgStroke(vg);

    for (size_t rk = 0; rk < profile_.events.size(); ++rk)
        draw_events(ctx, profile_.events[rk], init_hoffset, init_voffset + (base_height() + rank_gap)*rk, base_height());
}

void
profvis::ProfileCanvas::
draw_events(NVGcontext* ctx, const Profile::Events& events, size_t hoffset, size_t voffset, size_t height)
{
    for (auto& e : events)
    {
        if (e.end - e.begin < time_filter) continue;

        NVGcontext* vg = ctx;
        nvgBeginPath(vg);

        auto color = colors_[e.id];

        float x = hoffset + (float(e.begin) - profile_.min_time()) / (profile_.max_time() - profile_.min_time()) * width;
        float y = voffset;
        float w = float(e.end - e.begin) / (profile_.max_time() - profile_.min_time()) * width;
        float h = height;

        if (!hide[e.id])
        {
            nvgRect(vg, x, y, w, h);
            //fmt::print("Event ({}, {}, {})\n", e.name, e.begin, e.end);
            //fmt::print("Rectangle ({}, {}, {}, {})\n", x, y, w, h);
            //nvgStrokeColor(vg, color);
            //nvgStrokeWidth(vg, 1.0f);
            nvgFillColor(vg, color);
            nvgFill(vg);
            //nvgStroke(vg);
        }

        draw_events(ctx, e.events, hoffset, voffset + inset, height - 2*inset);
    }
}

bool
profvis::ProfileCanvas::
mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers)
{
    if (Canvas::mouseMotionEvent(p,rel,button,modifiers))
        return true;

    std::array<float,6> inverse;
    nvgTransformInverse(&inverse[0], &mTransform[0]);

    float x, y;
    nvgTransformPoint(&x, &y, &inverse[0], p[0], p[1]);

    // translate y to rank
    int rk = floor((y - init_voffset)/(base_height() + rank_gap));

    Profile::Event dummy { static_cast<size_t>(-1), 0, 0 };

    if (rk < 0 || rk > profile_.events.size() - 1)
    {
        callback_(dummy, -1);
        return false;
    }

    float rel_y = y - init_voffset - rk*(base_height() + rank_gap);
    if (rel_y > base_height())        // gap between ranks
    {
        callback_(dummy, -1);
        return false;
    }

    // translate x to time
    Profile::Time time = profile_.min_time() + (x - init_hoffset) / width * (profile_.max_time() - profile_.min_time());

    int max_level;
    if (rel_y < base_height()/2)
        max_level = rel_y / inset;
    else
        max_level = (base_height() - rel_y) / inset;

    // find the event
    const Profile::Event* event = search_events(time, profile_.events[rk], 0, max_level);
    if (event)
    {
        callback_(*event, rk);
        return true;
    } else
    {
        callback_(dummy, -1);
        return false;
    }
}


const profvis::Profile::Event*
profvis::ProfileCanvas::
search_events(Profile::Time time, const Profile::Events& events, int level, int max_level) const
{
    if (level > max_level)
        return nullptr;

    for (auto& e : events)      // TODO: binary search
    {
        if (e.end - e.begin < time_filter) continue;

        if (time <= e.end && time >= e.begin)
        {
            // search children
            const Profile::Event* event = search_events(time, e.events, level + 1, max_level);

            if (event)
                return event;

            if (hide[e.id])
                return nullptr;

            return &e;
        }
    }

    return nullptr;
}

void
profvis::ProfileCanvas::
randomize_colors()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rand(0., 1.);

    for (auto& c : colors_)
        c = ng::Color { rand(gen), rand(gen), rand(gen), 1. };
}

profvis::NameColors
profvis::
fill_colors(const Profile& profile, std::mt19937& gen)
{
    //.Colors from https://sashat.me/2017/01/11/list-of-20-simple-distinct-colors/
    std::vector<std::tuple<int, int, int>> distinct_colors =
        { {230, 25, 75},   {60, 180, 75},   {255, 225, 25},  {0, 130, 200},   {245, 130, 48}, {145, 30, 180},
           {70, 240, 240}, {240, 50, 230},  {210, 245, 60},  {250, 190, 190}, {0, 128, 128},  {230, 190, 255},
           {170, 110, 40}, {255, 250, 200}, {128, 0, 0},     {170, 255, 195}, {128, 128, 0},  {255, 215, 180},
           {0, 0, 128},    {128, 128, 128}, {255, 255, 255}, {0, 0, 0} };

    NameColors colors; colors.resize(profile.names.size());
    for (size_t id = 0; id < profile.names.size(); ++id)
    {
        int ri,gi,bi;
        std::tie(ri,gi,bi) = distinct_colors[id % colors.size()];
        colors[id] = ng::Color { float(ri)/255, float(gi)/255, float(bi)/255, 1. };
    }
    return colors;
}

profvis::NameColors
profvis::
name_to_color(const Profile& profile)
{
    std::random_device rd;
    std::mt19937 gen(rd());

    NameColors  colors = fill_colors(profile, gen);

    return colors;
}

