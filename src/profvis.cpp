#include <opts/opts.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/button.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/textbox.h>
#include <nanogui/popupbutton.h>
#include <nanogui/colorpicker.h>
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

    auto select_colors = new ng::PopupButton(window, "Select colors");
    auto color_popup = select_colors->popup();
    color_popup->setLayout(new ng::GroupLayout);

    new ng::Label(window, "Events");

    std::unordered_map<std::string, std::tuple<ng::Button*, ng::ColorPicker*>>  color_buttons;

    for (auto& c : profile_->colors())
    {
        auto name = c.first;
        auto button = new ng::Button(window, name);
        button->setFlags(ng::Button::ToggleButton);
        button->setBackgroundColor(c.second);
        button->setTextColor(c.second.contrastingColor());
        button->setChangeCallback([name, this](bool state) { profile_->toggle(name); });

        auto cb = new ng::ColorPicker(color_popup, c.second);
        cb->setCaption(c.first);
        cb->setTextColor(c.second.contrastingColor());
        cb->setFinalCallback([this,button,cb,name](const ng::Color& c)
        {
            button->setBackgroundColor(c);
            button->setTextColor(c.contrastingColor());
            cb->setTextColor(c.contrastingColor());

            profile_->set_color(name, c);
        });

        color_buttons[name] = { button, cb };
    }

    new ng::Label(window, "Time filter (min duration shown)");
    auto time_filter = new ng::IntBox<pv::Profile::Time>(window, profile_->time_filter);
    time_filter->setCallback([this](pv::Profile::Time t) { profile_->time_filter = t; });
    time_filter->setEditable(true);

    new ng::Label(window, "Width");
    auto width_filter = new ng::IntBox<size_t>(window, profile_->width);
    width_filter->setCallback([this](size_t w) { profile_->width = w; });
    width_filter->setEditable(true);

    /**********/
    /* Colors */
    /**********/
    auto update_button_colors = [this,window,color_buttons]()
    {
        // update button colors
        for (auto& x : color_buttons)
        {
            auto                name = x.first;
            ng::Button*         b    = std::get<0>(x.second);
            ng::ColorPicker*    cb   = std::get<1>(x.second);
            {
                auto c = profile_->colors().find(name)->second;
                b->setBackgroundColor(c);
                b->setTextColor(c.contrastingColor());
                cb->setBackgroundColor(c);
                cb->setTextColor(c.contrastingColor());
            }
        }
    };

    new ng::Label(window, "Colors");
    auto randomize_colors = new ng::Button(window, "Randomize colors");
    randomize_colors->setCallback([this,update_button_colors]()
    {
        profile_->randomize_colors();
        update_button_colors();
    });

    auto save_colors = new ng::Button(window, "Save colors");
    save_colors->setCallback([&]
    {
        auto filename = ng::file_dialog({ {"clr", "Colors"}, {"txt", "Text file"} }, true);
        std::ofstream out(filename);
        for (auto& c : profile_->colors())
            fmt::print(out, "{} {} {} {}\n", c.first, c.second.r(), c.second.g(), c.second.b());
    });

    auto load_colors = new ng::Button(window, "Load colors");
    load_colors->setCallback([this,update_button_colors]
    {
        auto filename = ng::file_dialog({ {"clr", "Colors"}, {"txt", "Text file"} }, false);

        std::ifstream   in(filename);
        std::string     line;
        while (std::getline(in, line))
        {
            std::string name;
            float r,g,b;
            std::istringstream ins(line);
            ins >> name >> r >> g >> b;
            profile_->set_color(name, ng::Color { r, g, b, 1.f });
        }

        update_button_colors();
    });
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

