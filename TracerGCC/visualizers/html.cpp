#include "html.h"


#pragma region HtmlTag
HtmlTag::WriteProxy HtmlTag::writeOpen(OutputFile &ofile) {
    ofile.write("<");
    ofile.write(tag);
    
    return WriteProxy{ofile, false};
}

void HtmlTag::writeClose(OutputFile &ofile) {
    ofile.write("</");
    ofile.write(tag);
    ofile.write(">");
}

HtmlTag::WriteProxy HtmlTag::writeInline(OutputFile &ofile) {
    ofile.write("<");
    ofile.write(tag);
    
    return WriteProxy{ofile, true};
}


HtmlTag::WriteProxy::WriteProxy(OutputFile &ofile_, bool isInline_) :
    ofile{ofile_}, isInline{isInline_} {}

HtmlTag::WriteProxy::~WriteProxy() {
    writeEnd();
}

HtmlTag::WriteProxy &HtmlTag::WriteProxy::arg(const char *name, const std::string_view &value) {
    ofile.write(" ");
    ofile.write(name);
    ofile.write("=\"");
    ofile.write(value);
    ofile.write("\"");

    return *this;
}

void HtmlTag::WriteProxy::writeEnd() const {
    ofile.write(isInline ? "/>" : ">");
}
#pragma endregion HtmlTag
