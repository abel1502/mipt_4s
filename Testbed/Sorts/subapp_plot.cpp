#include <AGF/llgui.h>
#include "subapp_plot.h"
#include "app.h"


#pragma region PlotSubapp::MyWidget
PlotSubapp::MyWidget::MyWidget(abel::gui::Widget *parent_,
                               const Rect<double> &region_,
                               const Vector2d &scale) :
    Base(parent_, region_,
         nullptr,
         nullptr,
         nullptr,
         nullptr,
         nullptr) {

    Vector2d plotSize = region.getDiag() - Vector2d{2 * PAD};
    plotSize.x() *= 0.7;
    plotSize.x() -= PAD / 2;
    plotSize.y() *= 0.5;
    plotSize.y() -= PAD / 2;

    Vector2d btnsSize = region.getDiag() - Vector2d{2 * PAD};
    btnsSize.x() *= 0.3;
    btnsSize.x() -= PAD / 2;
    btnsSize.y() -= PAD / 2 + PAD + BTN_HEIGHT * 2;

    plotCmp (new PlotWidget(nullptr, Rect<double>::wh(Vector2d{PAD, PAD},                    plotSize), "Comparisons"));
    plotAsgn(new PlotWidget(nullptr, Rect<double>::wh(Vector2d{PAD, PAD * 2 + plotSize.y()}, plotSize), "Assignments"));

    constexpr double SCALE_MARGINS = 0.05;
    Rect<double> limits = Rect<double>::se(-scale * SCALE_MARGINS, scale * (1 + SCALE_MARGINS));

    plotCmp ().setLimits(limits);
    plotAsgn().setLimits(limits);

    sortBtns(new widgets::LayoutOf<widgets::Button>(nullptr,
        Rect<double>::wh(Vector2d{PAD * 2 + plotSize.x(), PAD}, btnsSize),
        widgets::LAD_VERTICAL, PAD));

    regenBtn(new widgets::Button(nullptr,
        Rect<double>::wh(Vector2d{PAD * 2 + plotSize.x(), PAD * 2 + btnsSize.y()},
                         Vector2d{btnsSize.x(), BTN_HEIGHT}),
        "New testcase"));

    resetBtn(new widgets::Button(nullptr,
        Rect<double>::wh(Vector2d{PAD * 2 + plotSize.x(), PAD * 3 + btnsSize.y() + BTN_HEIGHT},
                         Vector2d{btnsSize.x(), BTN_HEIGHT}),
        "Reset"));

    resetBtn().sigClick += [this]() {
        clearAll();

        return false;
    };
}

void PlotSubapp::MyWidget::beginPlot(const Color &color) {
    REQUIRE(!plotActive);

    plotCmp ().resetPoints();
    plotAsgn().resetPoints();
    plotCmp ().setColor(color);
    plotAsgn().setColor(color);
    plotActive = true;
}

void PlotSubapp::MyWidget::endPlot() {
    REQUIRE(plotActive);

    plotActive = false;
}

void PlotSubapp::MyWidget::addPoint(double size, double cmps, double asgns) {
    REQUIRE(plotActive);

    plotCmp ().addPoint(Vector2d{size, cmps });
    plotAsgn().addPoint(Vector2d{size, asgns});
}

void PlotSubapp::MyWidget::clearAll() {
    REQUIRE(!plotActive);

    plotCmp ().clear();
    plotAsgn().clear();
}

void PlotSubapp::MyWidget::addSortButton(const Color &color, const char *name, std::function<void ()> &&callback) {
    widgets::Button &btn = sortBtns().createChild(
        Rect<double>::wh(0, 0, sortBtns().getRegion().w() - PAD * 2, BTN_HEIGHT),
        name);

    btn.getLabel().setTextColor(color);
    btn.sigClick += [this, color, callback]() {
        beginPlot(color);
        callback();
        endPlot();

        return false;
    };
}

void PlotSubapp::MyWidget::setRegenCallback(std::function<bool ()> &&callback) {
    regenBtn().sigClick.clear();
    regenBtn().sigClick += std::move(callback);
}
#pragma endregion PlotSubapp::MyWidget


#pragma region PlotSubapp
PlotSubapp::PlotSubapp() {
    registerSorts();

    abel::srand(0x1234567);

    regenerateModelArray();

    wrapper_t::sigOp += [this](wrapper_t::Op op, const wrapper_t *inst, const wrapper_t *other, const char *opSym) {
        if (!isActive()) {
            return false;
        }

        onWrapperOp(op, inst, other, opSym);

        return false;
    };
}

void PlotSubapp::registerSorts() {
    registerSort(new sort_providers::StdSortProvider<wrapper_t>());
    registerSort(new sort_providers::StdStableSortProvider<wrapper_t>());
    registerSort(new sort_providers::InplaceMergeSortProvider<wrapper_t>());
    registerSort(new sort_providers::BubbleSortProvider<wrapper_t>());
    registerSort(new sort_providers::InsertionSortProvider<wrapper_t>());
}

void PlotSubapp::registerSort(SortProvider<wrapper_t> *provider) {
    sorters.appendEmplace(provider);
}

void PlotSubapp::activate(bool active) {
    if (active == isActive()) {
        // Already in the desired state
        return;
    }

    if (active) {
        // Turn on

        createWidget();
        assert(isActive());
    } else {
        // Turn off

        wnd->die();
    }
}

void PlotSubapp::createWidget() {
    REQUIRE(!isActive());

    constexpr Vector2d SIZE{600, 500};

    // TODO: Change limits?

    MyWidget *widget = new MyWidget(nullptr,
                                    Rect<double>::wh(Vector2d{}, SIZE),
                                    Vector2d{(double)testSize, (double)testSize * 32});

    for (auto &sorter : sorters) {
        widget->addSortButton(sorter->getColor(), sorter->getName(),
                              [this, &sorter = *sorter]() { runTest(sorter); });
    }

    widget->setRegenCallback([this]() {
        regenerateModelArray();

        return false;
    });

    wnd = MyApp::getInstance().getWindowMgrWidget().createWindow(
        Rect<double>::wh({100, 50}, SIZE),
        "Have some plots bro",
        widget
    );
}

void PlotSubapp::runTest(SortProvider<wrapper_t> &sorter) {
    // This assumes that the environment has been set up and we are being executed as a callback

    auto &widget = getWidget();

    const unsigned testStep = 5;
    for (unsigned size = 0; size < testSize; size += testStep) {
        runOneTest(sorter, size);

        widget.addPoint(size, results.cmps, results.asgns);

        MyApp::getInstance().forceRender();
    }
}

void PlotSubapp::runOneTest(SortProvider<wrapper_t> &sorter, unsigned size) {
    abel::vector<wrapper_t> data{modelArray.begin(), modelArray.begin() + size};

    results.asgns = 0;
    results.cmps  = 0;
    results.collecting = true;

    sorter.sort(data.getBuf(), data.getSize());

    results.collecting = false;

    for (unsigned i = 1; i < data.getSize(); ++i) {
        if (data[i - 1] > data[i]) {
            ERR("Sorting algorithm '%s' implemented incorrectly. Ignoring", sorter.getName());
            break;
        }
    }
}

void PlotSubapp::onWrapperOp(wrapper_t::Op op, const wrapper_t *inst, const wrapper_t *other, const char *opSym) {
    using Op = wrapper_t::Op;

    if (!results.collecting) {
        return;
    }

    switch (op) {
    case Op::Copy:
    case Op::Move:
        ++results.asgns;
        break;

    case Op::Cmp:
        ++results.cmps;
        break;

    default:
        break;
    }
}

void PlotSubapp::regenerateModelArray() {
    modelArray.resize(testSize);
    for (unsigned i = 0; i < testSize; ++i) {
        modelArray[i] = wrapper_t{(elem_t)abel::randLL(testSize * 10)};
    }
}
#pragma endregion PlotSubapp