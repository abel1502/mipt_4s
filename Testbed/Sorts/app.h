#pragma once

#include <AGF/llgui_pre.h>
#include <ACL/general.h>
#include <AGF/application.h>
#include <AGF/widgets/window.h>


using abel::gui::Rect;
using abel::gui::Color;
using abel::math::Vector2d;
namespace widgets = abel::gui::widgets;


class MyApp final : public abel::gui::Application {
public:
    MyApp();

    virtual void init(int argc, const char **argv) override;

    virtual void deinit() override;

    virtual ~MyApp() override;

    static inline MyApp &getInstance() {
        return dynamic_cast<MyApp &>(abel::gui::Application::getInstance());
    }

    widgets::WindowManager &getWindowMgrWidget() {
        return dynamic_cast<widgets::WindowManager &>(*mainWidget);
    }

    void forceRender();

protected:
    //

};
