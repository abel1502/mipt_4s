#pragma once
#include <AGF/llgui_pre.h>
#include <ACL/general.h>
#include <AGF/application.h>
#include <AGF/widgets/window.h>
#include <ACL/unique_ptr.h>
#include "subapp_plot.h"
#include "subapp_counter.h"


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

    /// Some prerequisites:
    ///  - Only call inside event dispatch
    ///  - Don't call this inside the dispatch of a RenderEvent
    void forceRender();

protected:
    abel::unique_ptr<PlotSubapp> subappPlot{nullptr};
    abel::unique_ptr<CounterSubapp> subappCounter{nullptr};

};
