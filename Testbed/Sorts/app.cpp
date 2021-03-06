#include <AGF/llgui.h>
#include <AGF/widgets/all.h>
#include <AGF/helpers/widget_ref.h>
#include "app.h"
#include <ACL/debug.h>


abel::gui::Application::app_ptr_t abel::gui::Application::create() {
    return app_ptr_t(new MyApp());
}


MyApp::MyApp() :
    Application() {}


MyApp::~MyApp() = default;


void MyApp::init(int argc, const char **argv) {
    if (initialized)
        return;

    abel::verbosity = 2;

    Application::init(argc, argv);

    using namespace widgets;
    using abel::gui::Rect;
    using abel::gui::WidgetRef;
    using abel::gui::WidgetRefTo;

    subappPlot = new PlotSubapp();
    subappCounter = new CounterSubapp();

    Layout *lay = new Layout(nullptr, Rect<double>::wh(0, 0, 160, 190), LAD_VERTICAL, 10);

    lay->createChild<Button>(Rect<double>::wh(0, 0, 140, 50), "A plot")
        .sigClick += [this](){

        subappPlot->activate();

        return false;
    };

    lay->createChild<Button>(Rect<double>::wh(0, 0, 140, 50), "A bar chart")
        .sigClick += [this](){
        // TODO

        return false;
    };

    lay->createChild<Button>(Rect<double>::wh(0, 0, 140, 50), "I like my console")
        .sigClick += [this](){
        subappCounter->activate();

        return false;
    };

    WindowManager *mgr = new WindowManager(nullptr, Rect<double>::wh(0, 0, 800, 600));
    mgr->createWindow(Rect<double>::wh(140, 50, 160, 190), "Pick:", lay).markEssential();

    #if 0
    PlotWidget *testPlot = new PlotWidget(nullptr, Rect<double>::wh(0, 0, 200, 200));
    testPlot->setLimits(Rect<double>::se(-5, -5, 5, 5));
    testPlot->addPoint({-1, -1});
    testPlot->addPoint({0, -1});
    testPlot->addPoint({1, 0});
    testPlot->addPoint({2, 2});
    mgr->createWindow(Rect<double>::wh(340, 150, 200, 200), "Test plot", testPlot);
    #endif

    mainWidget = mgr;

    #if 0
    {
        Layout &lay = grp->createChild<Layout>(Rect<double>::wh(140, 0, 150, 170));

        lay.createChild<Button>(Rect<double>::wh(0, 0, 140, 30), "Create layer")
            .sigClick += [canvas = WidgetRefTo(canvas)]() {
            if (!canvas) {
                return true;
            }

            canvas->addLayer();
            DBG("Now on layer %u", canvas->getActiveLayerIdx());

            return false;
        };

        lay.createChild<Button>(Rect<double>::wh(0, 0, 140, 30), "Next layer")
            .sigClick += [canvas = WidgetRefTo(canvas)]() {
            if (!canvas) {
                return true;
            }

            canvas->nextLayer();
            DBG("Now on layer %u", canvas->getActiveLayerIdx());

            return false;
        };

        lay.createChild<Button>(Rect<double>::wh(0, 0, 140, 30), "Prev layer")
            .sigClick += [canvas = WidgetRefTo(canvas)]() {
            if (!canvas) {
                return true;
            }

            canvas->nextLayer();
            DBG("Now on layer %u", canvas->getActiveLayerIdx());

            return false;
        };

        lay.createChild<Button>(Rect<double>::wh(0, 0, 140, 30), "Load image")
            .sigClick += [canvas = WidgetRefTo(canvas)]() {
            if (!canvas) {
                return true;
            }

            char path[MAX_PATH + 1] = "";
            printf("img > ");
            fgets(path, MAX_PATH, stdin);
            size_t len = strlen(path);
            if (len > 0) {
                path[len - 1] = 0;
            }

            std::fs::path imgPath{path};

            try {
                canvas->loadImage(imgPath);
            } catch (const abel::error &e) {
                ERR("Failed to load image from '%ls': \"%s\"",
                    imgPath.c_str(), e.what());
            }

            return false;
        };

        lay.createChild<TextBox>(Rect<double>::wh(0, 0, 140, 30), "Default")
            .sigSubmit += [](const char *text) {

            DBG("Got \"%s\"", text);

            return false;
        };
    }
    #endif
}


void MyApp::deinit() {
    if (!initialized)
        return;

    Application::deinit();
}


void MyApp::forceRender() {
    /// See the prerequisites in app.h!

    abel::gui::Texture texture{*wnd};
    texture.setFillColor(Color::WHITE);
    texture.clear();

    enqueueEvent(abel::gui::RenderEvent{texture}, P_IMMEDIATE, true);

    wnd->render(texture);

    // TODO: Maybe remove
    // wnd->update();
}
