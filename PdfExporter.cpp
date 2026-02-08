#include "PdfExporter.h"

#include <QFileInfo>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPrinter>

#include "Main.h"
#include "Printer.h"

void PdfExporter::render() {
    Printer* pdf = new Printer(mFilename);

    QPageSize pageSz(mPid);
    QSizeF pageSize(pageSz.sizePoints());
    pdf->setPageSize(mPid);
    pdf->setPageMargins({ Main::ref().prefs().margins()[Preferences::Left],  Main::ref().prefs().margins()[Preferences::Top],
                          Main::ref().prefs().margins()[Preferences::Right], Main::ref().prefs().margins()[Preferences::Right] });
    pdf->setAuthor(mAuthor);
    pdf->setTitle(mTitle);

    QPainter* painter = new QPainter(dynamic_cast<QPaintDevice*>(pdf));

    Main::ref().setPrinter(pdf);
    pdf->outputNovel(mItemIds, mChapterTag, mSceneTag, mCoverTag, painter, pageSize,
        [this, &pdf, &painter]() {
            pdf->header(painter);
            pdf->footer(painter);
            pdf->newPage();
        });
}

bool PdfExporter::convert() {
    double height;
    double width;

    if (mFilename.isEmpty()) return false;

    const auto& defaults = collectMetadataDefaults();
    mCover = fetchValue(Cover, defaults, "cover");

    // Note: we are looking for matches in the base order because the matches
    // are more natural than in alphabetical order.
    mPaperSize = fetchValue(PaperSize, defaults, "paper size");
    QPageSize::PageSizeId pid = QPageSize::Letter;
    for (int id = 0; id <= QPageSize::LastPageSize; ++id) {
        pid = static_cast<QPageSize::PageSizeId>(id);
        QString name = QPageSize(pid).name();
        if (QPageSize(pid).isValid() && ((name.startsWith("A") && name[1].isDigit()) ||
                                         name.startsWith("Letter") ||
                                         name.startsWith("Legal"))) {
            if ((mPaperSize == "A" && name == "A4") ||
                (name.startsWith(mPaperSize)) ||
                (name.contains(mPaperSize))) {
                mPaperSize = name;
                break;
            }
        }
    }
    if (pid == QPageSize::Letter) mPaperSize = QPageSize(pid).name();

    StringList inputMargins { fetchValue(PaperMargins, defaults, "margins").split(",") };
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
            double lmargin = toInches(inputMargins[1]);
            double rmargin = toInches(inputMargins[2]);
            double tmargin = toInches(inputMargins[3]);
            double bmargin = toInches(inputMargins[4]);
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
