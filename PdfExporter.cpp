#include "PdfExporter.h"

#include <QFileInfo>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPrinter>

#include "Main.h"
#include "Printer.h"

bool PdfExporter::render() {
    Printer* pdf = new Printer(mFilename);
    pdf->setPrefs(&Main::ref().prefs());

    pdf->qpdfwriter()->setResolution(72);

    auto pid = Main::ref().prefs().pageSizeToPid(Main::ref().prefs().pageSize());
    pdf->setPageSize(pid);

    QSizeF pageSize = pdf->pageSize(QPrinter::DevicePixel);;
    pdf->setPageMargins({ 0, 0, 0, 0 });

    pdf->setAuthor(mAuthor);
    pdf->setTitle(mTitle);

    QPainter* painter = new QPainter(pdf->paintdevice());

    auto save = Main::ref().exchangePrinter(pdf);

    bool result = pdf->outputNovel(mItemIds, mChapterTag, mSceneTag, mCoverTag, painter, pageSize,
        [this, &pdf, &painter](bool m) {
            if (m) {
                pdf->header(painter);
                pdf->footer(painter);
            }
            pdf->newPage();
        });

    pdf = Main::ref().exchangePrinter(save);

    painter->end();
    delete painter;
    delete pdf;

    return result;
}

bool PdfExporter::convert() {
    double height;
    double width;

    if (mFilename.isEmpty()) return false;

    const auto& defaults = collectMetadataDefaults();
    mCover = fetchValue(Cover, defaults, "cover");

    // Note: we are looking for matches in the base order because the matches
    // are more natural than in alphabetical order.

    mTitle  = fetchValue(Title, defaults, "title");
    mAuthor = fetchValue(Author, defaults, "author");

    render();

    QFile file(mFilename);
    if (!file.exists() || file.size() == 0) return false;

    QFileInfo info(mFilename);
    Main::ref().setDocDir(info.absolutePath());

    return true;
}

QMarginsF PdfExporter::parseMargins(StringList& inputMargins) {
    QMarginsF margins;
    // Margins: if zero: they are all 1"
    //          if only 1: they are all that measure
    //          if only 2: they are [L,R] and [T,B]
    //          if only 3: they are L, R, [T,B]
    //          if 4 or more: they are L, R, T, B
    switch (inputMargins.size()) {
    case 1: {
        double margin = toInches(inputMargins[0]);
        margins = { margin, margin, margin, margin };
        break;
    }
    case 2: {
        double lrmargin = toInches(inputMargins[0]);
        double tbmargin = toInches(inputMargins[1]);
        margins = { lrmargin, tbmargin, lrmargin, tbmargin };
        break;
    }
    case 3: {
        double lmargin = toInches(inputMargins[1]);
        double rmargin = toInches(inputMargins[2]);
        double tbmargin = toInches(inputMargins[3]);
        margins = { lmargin, tbmargin, rmargin, tbmargin };
        break;
    }
    default: {
        if (inputMargins.size() >= 4) {
            double lmargin = toInches(inputMargins[Preferences::Left]);
            double rmargin = toInches(inputMargins[Preferences::Right]);
            double tmargin = toInches(inputMargins[Preferences::Top]);
            double bmargin = toInches(inputMargins[Preferences::Bottom]);
            margins = { lmargin, tmargin, rmargin, bmargin };
        }
        break;
    }
    }
    return margins;
}

double PdfExporter::parseNumber(const QString& s) {
    QString t = s.trimmed();

    // Mixed fraction: "3 1/4"
    QRegularExpression mixed(R"(^([0-9]+)\s+([0-9]+)/([0-9]+)$)");
    auto m = mixed.match(t);
    if (m.hasMatch()) {
        double whole = m.captured(1).toDouble();
        double num   = m.captured(2).toDouble();
        double den   = m.captured(3).toDouble();
        if (den == 0.0) return whole;
        return whole + (num / den);
    }

    // Simple fraction: "1/2"
    QRegularExpression frac(R"(^([0-9]+)/([0-9]+)$)");
    m = frac.match(t);
    if (m.hasMatch()) {
        double num = m.captured(1).toDouble();
        double den = m.captured(2).toDouble();
        if (den == 0.0) return 1.0;
        return num / den;
    }

    // Decimal or integer
    return t.toDouble();
}

double PdfExporter::toInches(const QString& measure) {
    static const QMap<QString, double> factors = {
        { "in", 1.0 },
        { "\"", 1.0 },
        { "inch", 1.0 },
        { "inches", 1.0 },
        { "pt", 1.0 / 72.0 },
        { "mm", 1.0 / 25.4 },
        { "cm", 1.0 / 2.54 },
        { "px", 1.0 / 96.0 }   // if you want CSS-style px
    };

    QRegularExpression re(R"(^\s*((?:[0-9]*\.?[0-9]+)|(?:[0-9]+\s+[0-9]+/[0-9]+)|(?:[0-9]+/[0-9]+))\s*([a-zA-Z"]+)\s*$)");
    auto m = re.match(measure);

    if (!m.hasMatch()) return 1.0;

    double value = parseNumber(m.captured(1));
    QString unit = m.captured(2).toLower();

    if (value == 0.0) return 1.0;
    if (!factors.contains(unit)) return value;

    return value * factors[unit];
}
