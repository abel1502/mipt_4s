#ifndef AGF_WIDGETS_LABEL_H
#define AGF_WIDGETS_LABEL_H

#include <AGF/widget.h>
#include <string>


namespace abel::gui::widgets {


class Label : public Widget {
public:
    EVENT_HANDLER_USING(Widget);

    Label(Widget *parent_, const Rect<double> &region_, const char *text_ = "");

    inline const char *getText() const noexcept { return text.data(); }
    void setText(const char *text_);

    inline const Color &getTextColor() const { return textColor; }
    void setTextColor(const Color &color);

    EVENT_HANDLER_OVERRIDE(Render);

protected:
    std::string text;
    unsigned format;
    Color textColor{Color::BLACK};

    void updated();

};


}


#endif // AGF_WIDGETS_LABEL_H

