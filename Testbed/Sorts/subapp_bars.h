#if 0
#pragma once
#include <AGF/llgui_pre.h>
#include "sort_provider.h"
#include "tracer.h"
#include <AGF/helpers/widget_ref.h>
#include <ACL/vector.h>
#include <functional>


using abel::gui::Rect;
using abel::gui::Color;
using abel::math::Vector2d;
namespace widgets = abel::gui::widgets;


class BarsSubapp {
private:
    using _MyWidgetBase =
        widgets::StaticGroup<
            BarsWidget,  // TODO: Implement
            widgets::LayoutOf<
                widgets::Button
            >,
            widgets::Button,
            widgets::Button
        >;

    struct _MarkerExtra {};

public:
    using elem_t = int;
    using wrapper_t = Tracer<elem_t, _MarkerExtra>;

    class MyWidget : public _MyWidgetBase {
    public:
        using Base = _MyWidgetBase;
        EVENT_HANDLER_USING(Base);


        static constexpr double BTN_HEIGHT = 30;
        static constexpr double PAD = 5;


        MyWidget(abel::gui::Widget *parent_, const Rect<double> &region_, const Vector2d &scale);

        void populate(const abel::vector<wrapper_t> *data);

        void swapItems(size_t a, size_t b);

        void swapItems(const wrapper_t *a, const wrapper_t *b);

        void addSortButton(const Color &color, const char *name, std::function<void ()> &&callback);

        void setRegenCallback(std::function<bool ()> &&callback);

    protected:
        const abel::vector<wrapper_t> *data = nullptr;


        SGRP_DECLARE_BINDING_I(barChart, 0);
        SGRP_DECLARE_BINDING_I(sortBtns, 1);
        SGRP_DECLARE_BINDING_I(resetBtn, 2);
        SGRP_DECLARE_BINDING_I(regenBtn, 3);

    };


    PlotSubapp();

    PlotSubapp(const PlotSubapp &other) = delete;
    PlotSubapp &operator=(const PlotSubapp &other) = delete;
    PlotSubapp(PlotSubapp &&other) = delete;
    PlotSubapp &operator=(PlotSubapp &&other) = delete;


    inline bool isActive() const { return wnd; }

    void activate(bool active = true);

protected:
    unsigned testSize = 100;
    abel::gui::WidgetRefTo<abel::gui::widgets::Window> wnd{nullptr};
    struct {
        unsigned cmps  = 0;
        unsigned asgns = 0;
        bool collecting = false;
    } results{};
    abel::vector<abel::unique_ptr<SortProvider<wrapper_t>>> sorters{};
    abel::vector<wrapper_t> modelArray{};


    inline MyWidget &getWidget() {
        REQUIRE(isActive());

        return dynamic_cast<MyWidget &>(wnd->getContents());
    }

    inline const MyWidget &getWidget() const {
        return const_cast<PlotSubapp *>(this)->getWidget();
    }

    void registerSorts();

    void registerSort(SortProvider<wrapper_t> *provider);

    void createWidget();

    void runTest(SortProvider<wrapper_t> &sorter);

    void runOneTest(SortProvider<wrapper_t> &sorter, unsigned size);

    void onWrapperOp(wrapper_t::Op op, const wrapper_t *inst, const wrapper_t *other, const char *opSym);

    void regenerateModelArray();

};
#endif
