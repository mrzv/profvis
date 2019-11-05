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
#include <nanogui/vscrollpanel.h>
#include <nanogui/slider.h>
namespace ng = nanogui;

#include <profvis/profile-canvas.h>
namespace pv = profvis;

class ProfVis: public ng::Screen
{
    public:
        using ColorButtons = std::unordered_map<std::string, std::tuple<ng::Button*, ng::ColorPicker*>>;

                            ProfVis(const pv::Profile& profile, std::string suffix = ""):
                                ng::Screen(ng::Vector2i(1200, 800), "Profile visualizer" + suffix),
                                profile_(new pv::ProfileCanvas(profile, this))
        {
            setup_controls();
            performLayout(mNVGContext);
        }

        void                setup_controls();
        void                update_button_colors();

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

        std::string         time_to_string(pv::Profile::Time time) const
        {
            return fmt::format("{:02d}:{:02d}:{:02d}.{:06d}",
                               time/1000000/60/60,
                               time/1000000/60 % 60,
                               time/1000000 % 60,
                               time % 1000000);
        }

    private:
        pv::ProfileCanvas*  profile_;
        ColorButtons        color_buttons_;
};

void
ProfVis::setup_controls()
{
    auto event_window = new ng::Window(this, "Event");
    event_window->setPosition({ 870, 600 });
    event_window->setFixedWidth(300);
    event_window->setLayout(new ng::GroupLayout);

    auto name_box  = new ng::TextBox(event_window, "");
    auto begin_box = new ng::TextBox(event_window, "");
    auto end_box   = new ng::TextBox(event_window, "");
    auto rank_box  = new ng::TextBox(event_window, "");
    profile_->set_callback([this,name_box,begin_box,end_box,rank_box](const pv::Profile::Event& e, int rk)
    {
        if (rk != -1)
        {
            name_box->setValue(profile_->profile().name(e));
            begin_box->setValue(time_to_string(e.begin));
            end_box->setValue(time_to_string(e.end));
            rank_box->setValue(std::to_string(rk));
        } else
        {
            name_box->setValue("");
            begin_box->setValue("");
            end_box->setValue("");
            rank_box->setValue("");
        }
    });

    auto window = new ng::Window(this, "Controls");
    window->setPosition({ 15, 15 });
    window->setLayout(new ng::GroupLayout);

    auto events = new ng::PopupButton(window, "Events");
    auto events_popup = events->popup();
    events_popup->setLayout(new ng::GroupLayout);

    auto layout = new ng::PopupButton(window, "Layout");
    auto layout_popup = layout->popup();
    layout_popup->setLayout(new ng::GridLayout(ng::Orientation::Horizontal, 2, ng::Alignment::Fill, 10, 5));
    auto setup_filter = [layout_popup](std::string name, size_t* variable, size_t max)
    {
        new ng::Label(layout_popup, name);
        auto filter = new ng::IntBox<int>(layout_popup, *variable);
        filter->setCallback([variable](int x) { *variable = x; });
        filter->setEditable(true);
        filter->setSpinnable(true);
        filter->setMinValue(0);
        //auto filter = new ng::Slider(layout_popup);
        //filter->setValue(float(*variable) / max);
        //filter->setCallback([variable,max](float x) { *variable = x * max; });

        return filter;
    };
    auto width_filter = setup_filter("width",  &profile_->width, 10000);
    width_filter->setValueIncrement(100);
    setup_filter("height", &profile_->init_height, 100);
    setup_filter("inset",  &profile_->inset,       25);
    setup_filter("gap",    &profile_->rank_gap,    100);

    new ng::Label(window, "Time (min duration shown)");
    auto time_filter = new ng::IntBox<pv::Profile::Time>(window, profile_->time_filter);
    time_filter->setCallback([this](pv::Profile::Time t) { profile_->time_filter = t; });
    time_filter->setEditable(true);

    new ng::Label(window, "Colors");

    auto select_colors = new ng::PopupButton(window, "Select");
    auto color_popup = select_colors->popup();
    color_popup->setLayout(new ng::GroupLayout);

    // fill colors both in Select and Events popups
    for (size_t i = 0; i < profile_->colors().size(); ++i)
    {
        auto& c = profile_->colors()[i];
        auto name = profile_->profile().name(i);

        auto button = new ng::Button(events_popup, name);
        button->setFlags(ng::Button::ToggleButton);
        button->setBackgroundColor(c);
        button->setTextColor(c.contrastingColor());
        button->setChangeCallback([name, this](bool state) { profile_->toggle(name); });

        auto cb = new ng::ColorPicker(color_popup, c);
        cb->setCaption(name);
        cb->setTextColor(c.contrastingColor());
        cb->setFinalCallback([this,button,cb,name](const ng::Color& c)
        {
            button->setBackgroundColor(c);
            button->setTextColor(c.contrastingColor());
            cb->setTextColor(c.contrastingColor());

            profile_->set_color(name, c);
        });

        color_buttons_[name] = { button, cb };
    }

    auto randomize_colors = new ng::Button(window, "Randomize");
    randomize_colors->setCallback([this]()
    {
        profile_->randomize_colors();
        update_button_colors();
    });

    auto save_colors = new ng::Button(window, "Save");
    save_colors->setCallback([&]
    {
        auto filename = ng::file_dialog({ {"clr", "Colors"}, {"txt", "Text file"} }, true);
        std::ofstream out(filename);
        for (size_t i = 0; i < profile_->colors().size(); ++i)
        {
            auto& c = profile_->colors()[i];
            auto name = profile_->profile().name(i);
            fmt::print(out, "{} {} {} {}\n", name, c.r(), c.g(), c.b());
        }
    });

    auto load_colors = new ng::Button(window, "Load");
    load_colors->setCallback([this]
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

void
ProfVis::
update_button_colors()
{
    // update button colors
    for (auto& x : color_buttons_)
    {
        auto                name = x.first;
        ng::Button*         b    = std::get<0>(x.second);
        ng::ColorPicker*    cb   = std::get<1>(x.second);
        {
            auto c = profile_->colors()[profile_->profile().id(name)];
            b->setBackgroundColor(c);
            b->setTextColor(c.contrastingColor());
            cb->setBackgroundColor(c);
            cb->setTextColor(c.contrastingColor());
        }
    }
}


int main(int argc, char *argv[])
{
    using namespace opts;
    Options ops;

    bool help;
    bool caliper;
    ops
        >> Option('h', "help",  help,           "show help")
        >> Option('c', "caliper", caliper,      "parse caliper format")
    ;

    std::string     infn;
    if (!ops.parse(argc,argv) || !(ops >> PosOption(infn)) || help)
    {
        fmt::print("Usage: {} FILE.prf\n", argv[0]);
        fmt::print("\nCaliper format is produced via: cali-query -t *.cali -q \"SELECT * WHERE event.end#annotation\"\n\n");
        fmt::print("{}", ops);
        return 1;
    }

    try
    {
        nanogui::init();

        pv::Profile profile;
        if (!caliper)
            profile = pv::read_profile(infn);
        else
            profile = pv::read_caliper(infn);
        ProfVis*    app     = new ProfVis(profile, " - " + infn);

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

