#pragma once

#include <unordered_map>
#include <random>

#include "canvas.h"
#include <profvis/profile.h>

namespace profvis
{

using NameColors = std::unordered_map<std::string, ng::Color>;

NameColors
name_to_color(const Profile& profile);

class ProfileCanvas: public Canvas
{
    public:
        using HideNames = std::unordered_map<std::string, bool>;
        using Callback  = std::function<void(const Profile::Event&,int)>;

    public:
                                ProfileCanvas(const Profile& profile, nanogui::Widget* parent):
                                    Canvas(parent),
                                    profile_(profile),
                                    colors_(name_to_color(profile_)),
                                    callback_([](const Profile::Event&,int) {})
                                {}
        virtual void            drawContents(NVGcontext* ctx) override;
        virtual ng::Vector2i    preferredSize(NVGcontext *ctx) const override       { return mParent->size(); }

        void                    draw_events(NVGcontext* ctx, const Profile::Events& events, size_t hoffset, size_t voffset, size_t height);

        const NameColors&       colors() const                                      { return colors_; }
        void                    set_color(std::string name, ng::Color c)            { colors_[name] = c; }

        void                    toggle(std::string name)                            { hide[name] = !hide[name]; }

        void                    randomize_colors();

        void                    set_callback(const Callback& callback)              { callback_ = callback; }

        virtual bool            mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

        size_t                  base_height() const                                 { return init_height + 2*inset*profile_.max_depth(); }

    public:
        size_t                  time_filter     = 1000;
        size_t                  width           = 1000;
        size_t                  init_height     = 30;
        size_t                  inset           = 5;
        size_t                  rank_gap        = 30;

    private:
        const Profile&          profile_;
        NameColors              colors_;

        size_t                  init_voffset    = 30;
        size_t                  init_hoffset    = 30;

        HideNames               hide;

        Callback                callback_;
};

void
fill_colors(const Profile::Events& events, NameColors& colors, std::mt19937& gen);

}


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

        auto color = colors_[e.name];

        float x = hoffset + float(e.begin) / profile_.max_time() * width;
        float y = voffset;
        float w = float(e.end - e.begin) / profile_.max_time() * width;
        float h = height;

        if (!hide[e.name])
        {
            nvgRect(vg, x, y, w, h);
            //fmt::print("Event ({}, {}, {})\n", e.name, e.begin, e.end);
            //fmt::print("Rectangle ({}, {}, {}, {})\n", x, y, w, h);
            nvgStrokeColor(vg, color);
            nvgStrokeWidth(vg, 1.0f);
            nvgFillColor(vg, color);
            nvgFill(vg);
            nvgStroke(vg);
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

    Profile::Event dummy { "", 0, 0 };

    if (rk < 0 || rk > profile_.events.size() - 1)
    {
        callback_(dummy, -1);
        return false;
    }

    if (y - init_voffset - rk*(base_height() + rank_gap) > base_height())        // gap between ranks
    {
        callback_(dummy, -1);
        return false;
    }

    // translate x to time
    Profile::Time time = (x - init_hoffset) / width * profile_.max_time();

    // find the event
    for (auto& e : profile_.events[rk])
    {
        if (e.end - e.begin < time_filter) continue;

        if (time <= e.end && time >= e.begin)
        {
            //if (hide[e.name])
            // recursively search children
            callback_(e, rk);
            return true;
        }
    }

    callback_(dummy, -1);

    return true;
}

void
profvis::ProfileCanvas::
randomize_colors()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rand(0., 1.);

    for (auto& c : colors_)
        c.second = ng::Color { rand(gen), rand(gen), rand(gen), 1. };
}

void
profvis::
fill_colors(const Profile::Events& events, NameColors& colors, std::mt19937& gen)
{
    for (auto& e : events)
    {
        if (colors.find(e.name) == colors.end())
        {
            std::uniform_real_distribution<float> rand(0., 1.);
            colors[e.name] = ng::Color { rand(gen), rand(gen), rand(gen), 1. };
        }
        fill_colors(e.events, colors, gen);
    }
}

profvis::NameColors
profvis::
name_to_color(const Profile& profile)
{
    std::random_device rd;
    std::mt19937 gen(rd());

    NameColors  colors;
    for (auto& events : profile.events)
        fill_colors(events, colors, gen);

    return colors;
}
