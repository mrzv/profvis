#include <profvis/widgets/canvas.h>

void
profvis::
Canvas::
transform(NVGcontext* ctx) const
{
    nvgTransform(ctx,
                 mTransform[0], mTransform[1], mTransform[2],
                 mTransform[3], mTransform[4], mTransform[5]);
}

void
profvis::
Canvas::
draw(NVGcontext* ctx)
{
    nanogui::Widget::draw(ctx);

    if (!mVisible)
        return;

    NVGcontext* vg = ctx;
    nvgSave(vg);

    Canvas::transform(vg);
    drawContents(vg);

    nvgRestore(vg);
}

bool
profvis::
Canvas::
mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers)
{
    if (nanogui::Widget::mouseButtonEvent(p, button, down, modifiers))
        return true;

    if (!down)
    {
        mActive = false;
        if (mState == State::Select)
        {
            std::array<float,6> inverse;
            nvgTransformInverse(&inverse[0], &mTransform[0]);

            nanogui::Vector2f min,max;
            nvgTransformPoint(&min[0], &min[1], &inverse[0],
                              std::min(mStart[0], mLast[0]),
                              std::min(mStart[1], mLast[1]));
            nvgTransformPoint(&max[0], &max[1], &inverse[0],
                              std::max(mStart[0], mLast[0]),
                              std::max(mStart[1], mLast[1]));

            select(min, max);
        }
        mState = State::None;
    }
    else
    {
        if (button == GLFW_MOUSE_BUTTON_2 || (button == GLFW_MOUSE_BUTTON_1 && modifiers == GLFW_MOD_SHIFT))
        {
            mStart = mLast = p;
            mActive = true;
            mState = State::Drag;
        } else if (button == GLFW_MOUSE_BUTTON_1)
        {
            mStart = mLast = p;
            mActive = true;
            mState = State::Select;
        }
    }

    return true;
}

bool
profvis::
Canvas::
mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers)
{
    if (nanogui::Widget::mouseMotionEvent(p, rel, button, modifiers))
        return true;

    if (mActive && mState == State::Drag)
    {
        nanogui::Vector2i ti = p - mStart;
        mStart = p;

        std::array<float,6> translate;
        nvgTransformTranslate(&translate[0], ti.x(), ti.y());
        nvgTransformMultiply(&mTransform[0], &translate[0]);
    } else if (mActive && mState == State::Select)
        mLast = p;

    return false;
}

bool
profvis::
Canvas::
scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel)
{
    if (nanogui::Widget::scrollEvent(p, rel))
        return true;

    float scale = 1. + rel.y() * .1;

    std::array<float,6> transform;
    std::array<float,6> translate;
    std::array<float,6> scale_transform;

    nvgTransformIdentity(&transform[0]);

    nvgTransformTranslate(&translate[0], -p.x(), -p.y());
    nvgTransformMultiply(&transform[0], &translate[0]);

    nvgTransformScale(&scale_transform[0], scale, scale);
    nvgTransformMultiply(&transform[0], &scale_transform[0]);

    nvgTransformTranslate(&translate[0], p.x(), p.y());
    nvgTransformMultiply(&transform[0], &translate[0]);

    nvgTransformMultiply(&mTransform[0], &transform[0]);

    return true;
}

