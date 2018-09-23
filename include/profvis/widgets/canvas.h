#pragma once

#include <nanogui/widget.h>
#include <nanogui/opengl.h>

namespace profvis
{

class Canvas: public nanogui::Widget
{
    public:
        enum class State { None, Drag, Select };

    public:
                        Canvas(nanogui::Widget* parent):
                            nanogui::Widget(parent)                 { nvgTransformIdentity(&mTransform[0]); }

        void            transform(NVGcontext* ctx) const;

        virtual void    draw(NVGcontext* ctx) override;
        virtual void    drawContents(NVGcontext* ctx)                           {}

        virtual void    select(nanogui::Vector2f min, nanogui::Vector2f max)    {}

        // controls
        virtual bool    mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
        virtual bool    mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
        virtual bool    scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;

    protected:
        // controls
        nanogui::Vector2i       mStart, mLast;
        std::array<float, 6>    mTransform;
        bool                    mActive = false;
        State                   mState = State::None;
};

}
