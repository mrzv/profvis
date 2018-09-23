#include <opts/opts.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/button.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/textbox.h>
namespace ng = nanogui;

#include <profvis/widgets/profile-canvas.h>
namespace pv = profvis;

class ProfVis: public ng::Screen
{
    public:
                            ProfVis(const pv::Profile& profile):
                                ng::Screen(ng::Vector2i(1200, 800), "Profile visualizer"),
                                profile_(new pv::ProfileCanvas(profile, this))
        {
            setup_controls();
            performLayout(mNVGContext);
        }

        void                setup_controls();

        virtual bool        resizeEvent(const ng::Vector2i& sz) override        { profile_->setSize(sz); return true; }
        virtual bool        keyboardEvent(int key, int scancode, int action, int modifiers) override
        {
            if (ng::Screen::keyboardEvent(key, scancode, action, modifiers))
                return true;

            if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
            {
                setVisible(false);
                return true;
            }

            return false;
        }

    private:
        pv::ProfileCanvas*  profile_;
};

void
ProfVis::setup_controls()
{
    auto window = new ng::Window(this, "Controls");
    window->setPosition({ 15, 15 });
    window->setLayout(new ng::GroupLayout);

    new ng::Label(window, "Events");

    for (auto& c : profile_->colors())
    {
        auto name = c.first;
        auto button = new ng::Button(window, name);
        button->setFlags(ng::Button::ToggleButton);
        button->setBackgroundColor(c.second);
        button->setTextColor(c.second.contrastingColor());
        button->setChangeCallback([name, this](bool state) { profile_->toggle(name); });
    }

    new ng::Label(window, "Filter");
    auto time_filter = new ng::IntBox<pv::Profile::Time>(window, profile_->time_filter);
    time_filter->setCallback([this](pv::Profile::Time t) { profile_->time_filter = t; });
    time_filter->setEditable(true);
}

int main(int argc, char *argv[])
{
    using namespace opts;
    Options ops;

    bool help;
    ops
        >> Option('h', "help",  help,           "show help")
    ;

    std::string     infn;
    if (!ops.parse(argc,argv) || !(ops >> PosOption(infn)) || help)
    {
        fmt::print("Usage: {} FILE.prf\n{}", argv[0], ops);
        return 1;
    }

    try
    {
        nanogui::init();

        pv::Profile profile = pv::read_profile(infn);
        ProfVis*    app     = new ProfVis(profile);

        app->drawAll();
        app->setVisible(true);

        nanogui::mainloop();

        delete app;

        nanogui::shutdown();
    } catch (const std::runtime_error &e)
    {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(WIN32)
            MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            fmt::print(std::cerr, "{}\n", error_msg);
        #endif
        return -1;
    }

    return 0;
}

