#include <AGF/llgui.h>
#include "plot.h"
#include "app.h"


PlotWidget::PlotWidget(Widget *parent_, const Rect<double> &region_, const char *title) :
    Base(parent_, region_, nullptr) {

    Vector2d labelStart{region.w() * 0.6, 0};
    Vector2d labelSize{region.w() - labelStart.x(), 32};

    label(new widgets::Label(nullptr, Rect<double>::wh(labelStart, labelSize), title));

    resetPoints();
}

void PlotWidget::addPoint(const Vector2d &value) {
    points.append(Point{value});

    invalidateCache();
}

void PlotWidget::resetPoints() {
    points.clear();

    invalidateCache();
}

void PlotWidget::clear() {
    resetPoints();

    invalidateCache(false);
}

void PlotWidget::invalidateCache(bool keepOld) {
    _cachedTextureValid = false;
    _cachedTextureExists = _cachedTextureExists && keepOld;

    sigChanged(*this);
}

EVENT_HANDLER_IMPL(PlotWidget, abel::gui::Render) {
    EventStatus status = Widget::dispatchEvent(event);

    if (!status.shouldHandle(status.NODE)) {
        return status;
    }

    if (!_cachedTextureValid) {
        if (!_cachedTextureExists) {
            _cachedTexture = new abel::gui::Texture(region);
            _cachedTextureExists = true;

            renderBackground(*_cachedTexture, getBounds());
        }

        renderPoints(*_cachedTexture);
        renderPlot(*_cachedTexture);

        _cachedTextureValid = true;
    }

    event.target.embed(region, *_cachedTexture);

    return _dispatchToChildren(event);
}

void PlotWidget::renderBackground(abel::gui::Texture &target, const Rect<double> &at) const {
    target.setFillColor(Color::WHITE);
    // `region` and not `at`, because we want all of our widget to have this background
    target.drawRect(region);

    // TODO: Perhaps axis lines with arrows instead?
    Vector2d zeroPos = valueToPos(Vector2d{0});

    Vector2d axisXL{at.x0(), zeroPos.y()},
             axisXR{at.x1(), zeroPos.y()},
             axisYB{zeroPos.x(), at.y0()},
             axisYT{zeroPos.x(), at.y1()};

    if (limits.w() < 0) {
        std::swap(axisXL, axisXR);
    }

    if (limits.h() < 0) {
        std::swap(axisYB, axisYT);
    }

    target.setLineColor(Color::WHITE * 0.6f);
    target.setLineWidth(1.f);
    target.clipPush(at.padded(-1, -1, -1, -1));

    target.drawArrow(axisXL, axisXR, 15);
    target.drawArrow(axisYB, axisYT, 15);

    #if 0
    constexpr unsigned GRID_STEPS = 9;

    const Vector2d gridStep = at.getDiag() / (GRID_STEPS - 1);
    Vector2d start = at.getStart();
    const Vector2d gridStepH{gridStep.x(), 0};
    const Vector2d gridStepV{0, gridStep.y()};
    const Vector2d gridSizeH{at.w(), 0};
    const Vector2d gridSizeV{0, at.h()};
    for (unsigned i = 0; i < GRID_STEPS; ++i) {
        target.drawLineAlong(start + gridStepV * i, gridSizeH);
        target.drawLineAlong(start + gridStepH * i, gridSizeV);
    }
    #endif

    target.clipPop();
}

void PlotWidget::renderPoints(abel::gui::Texture &target) const {
    // TODO
}

void PlotWidget::renderPlot(abel::gui::Texture &target) const {
    target.setLineColor(color);
    target.setLineWidth(1.f);
    target.clipPush(getBounds().padded(-1, -1, -1, -1));

    for (unsigned i = 1; i < points.getSize(); ++i) {
        target.drawLine(valueToPos(points[i - 1].value),
                        valueToPos(points[i    ].value));
    }

    target.clipPop();
}


