#pragma once

#include <random>

#include "canvas.h"
#include "profile.h"

namespace profvis
{

namespace ng = nanogui;
using NameColors = std::vector<ng::Color>;

NameColors
name_to_color(const Profile& profile);

class ProfileCanvas: public Canvas
{
    public:
        using Hide      = std::vector<bool>;
        using Callback  = std::function<void(const Profile::Event&,int)>;

    public:
                                ProfileCanvas(const Profile& profile, nanogui::Widget* parent):
                                    Canvas(parent),
                                    profile_(profile),
                                    colors_(name_to_color(profile_)),
                                    hide(profile_.names.size(), false),
                                    callback_([](const Profile::Event&,int) {})
                                {}
        virtual void            drawContents(NVGcontext* ctx) override;
        virtual ng::Vector2i    preferredSize(NVGcontext *ctx) const override       { return mParent->size(); }

        void                    draw_events(NVGcontext* ctx, const Profile::Events& events, size_t hoffset, size_t voffset, size_t height);

        const NameColors&       colors() const                                      { return colors_; }
        void                    set_color(std::string name, ng::Color c)            { colors_[profile().id(name)] = c; }

        void                    toggle(std::string name)                            { auto id = profile().id(name); hide[id] = !hide[id]; }

        void                    randomize_colors();

        void                    set_callback(const Callback& callback)              { callback_ = callback; }

        virtual bool            mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
        const Profile::Event*   search_events(Profile::Time time, const Profile::Events& events, int level, int max_level) const;

        size_t                  base_height() const                                 { return init_height + 2*inset*profile_.max_depth(); }

        const Profile&          profile() const                                     { return profile_; }

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

        Hide                    hide;

        Callback                callback_;
};

NameColors
fill_colors(const Profile& profile, std::mt19937& gen);

}
