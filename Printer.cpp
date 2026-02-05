#include "Main.h"
#include "Printer.h"

const QSizeF Printer::pageSize(QPrinter::Unit unit) const {
    const QRectF rect = paperRect(unit);

    Preferences* prefs = mPrefs ? mPrefs : &Main::ref().prefs();
    auto margins = prefs->margins();
    return QSizeF(rect.right() - (margins[Preferences::Left] + margins[Preferences::Right]),
                  rect.bottom() - (margins[Preferences::Top] - margins[Preferences::Bottom]));
}
