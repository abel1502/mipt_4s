#pragma once

#include <AGF/widget.h>
#include <AGF/widgets/all.h>
#include <ACL/general.h>
#include <ACL/vector.h>
#include <ACL/gui/coords.h>
#include <ACL/signal.h>


using abel::gui::Rect;
using abel::gui::Color;
using abel::math::Vector2d;
namespace widgets = abel::gui::widgets;


class PlotWidget : public abel::gui::Widget {
public:
    using Base = abel::gui::Widget;
    EVENT_HANDLER_USING(Base);


    struct Point {
        Vector2d value{};


        inline explicit Point(const Vector2d &value_) :
            value{value_} {}

        inline bool operator<(const Point &other) const noexcept {
            return value.x() < other.value.x();
        }
    };

    abel::Signal<bool (PlotWidget &plot)> sigChanged{};


    PlotWidget(Widget *parent_, const Rect<double> &region_);

    EVENT_HANDLER_OVERRIDE(abel::gui::Render);

    constexpr void setLimits(const Rect<double> &limits_, bool vFlip = true) {
        limits = limits_;

        if (vFlip) {
            limits.y(limits.y() - limits.h());
            limits.h(-limits.h());
        }
    }
    constexpr const Rect<double> &getLimits() const { return limits; }

    constexpr void setColor(const Color &color_) { color = color_; }
    constexpr const Color &getColor() const { return color; }

    void addPoint(const Vector2d &value);

    void resetPoints();

    void clear();

protected:
    abel::vector<Point> points{};

    Rect<double> limits{Rect<double>::wh(0, 1, 1, -1)};
    Color color{Color::RED * 0.8f};

    mutable bool _cachedTextureExists = false;
    mutable bool _cachedTextureValid = false;
    mutable abel::unique_ptr<abel::gui::Texture> _cachedTexture = nullptr;

    constexpr abel::gui::Coords getCoords() const {
        return abel::gui::Coords{getBounds(), limits};
    }

    constexpr Vector2d valueToPos(const Vector2d &value) const {
        return getCoords().virt2screen(value);
    }

    Vector2d posToValue(const Vector2d &pos) const {
        return getCoords().screen2virt(pos);
    }

    constexpr Rect<double> getBounds() const {
        return region/*.padded(5)*/;
    }

    void invalidateCache(bool keepOld = true);

    void renderBackground(abel::gui::Texture &target, const Rect<double> &at) const;

    void renderPlot(abel::gui::Texture &target) const;

};

