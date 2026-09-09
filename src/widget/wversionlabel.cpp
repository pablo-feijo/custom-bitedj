// Modified for Custom Bite DJ, 2026-09-09: focus the footer on the fork's
// version and retain plain-text attribution to both upstream projects.
#include "widget/wversionlabel.h"

#include "moc_wversionlabel.cpp"
#include "util/versionstore.h"

WVersionLabel::WVersionLabel(QWidget* pParent)
        : WLabel(pParent) {
}

void WVersionLabel::setup(const QDomNode& node, const SkinContext& context) {
    WLabel::setup(node, context);
    setTextFormat(Qt::RichText);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setOpenExternalLinks(false);
    setText(QStringLiteral("<b>Custom Bite DJ %1</b><br/>"
                           "<span style=\"font-size:10px\">Based on "
                           "BiteDJ by Team Deckshark · Mixxx</span>")
                    .arg(VersionStore::version().toHtmlEscaped()));
}
