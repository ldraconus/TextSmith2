#pragma once

#include <QImage>
#include <QMap>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPaintDevice>
#include <QPainter>
#include <QPdfWriter>
#include <QPrinter>
#include <QPrinterInfo>
#include <QUrl>

#include <List.h>
#include <Map.h>

#include "Novel.h"
#include "Words.h"

class Preferences;

class Printer {
private:
    qlonglong         mId;
    List<qlonglong>   mIds;
    Map<QUrl, QImage> mImages;
    qlonglong         mPageNo;
    QPdfWriter*       mPdf     { nullptr };
    Preferences*      mPrefs   { nullptr };
    QPrinter*         mPrinter { nullptr };

    class Marginal {
    public:
        enum class Justify { Left, Center, Right };

        Marginal() { }
        Marginal(Justify j, qlonglong l, const QString& t)
            : mJustify(j)
            , mLine(l)
            , mText(t) { }
        Marginal(const Marginal& m)
            : mJustify(m.mJustify)
            , mLine(m.mLine)
            , mText(m.mText) { }
        Marginal(Marginal&& m)
            : mJustify(m.mJustify)
            , mLine(m.mLine)
            , mText(m.mText) { }
        Marginal(std::initializer_list<Marginal> m) {
            if (m.size() != 1) return;
            const Marginal& first = *m.begin();
            mJustify = first.mJustify;
            mLine =    first.mLine;
            mText =    first.mText;
        }

        virtual ~Marginal() { }

        Marginal& operator=(const Marginal& m) { if (this != &m) { mJustify = m.mJustify; mLine = m.mLine; mText = m.mText; } return *this; }
        Marginal& operator=(Marginal&& m) { mJustify = m.mJustify; mLine = m.mLine; mText = m.mText;  return *this; }

        Justify   justify() const           { return mJustify; }
        qlonglong line() const              { return mLine; }
        void      setText(const QString& t) { mText = t; }
        QString   text() const              { return mText; }

    private:
        Justify   mJustify = Justify::Left;
        qlonglong mLine =    0;
        QString   mText =    0;
    };

    const QPageLayout::Unit toPdfUnits(QPrinter::Unit unit) const {
        switch(unit) {
        case QPrinter::Millimeter:  return QPageLayout::Millimeter;
        case QPrinter::Point:       return QPageLayout::Point;
        case QPrinter::Inch:        return QPageLayout::Inch;
        case QPrinter::Pica:        return QPageLayout::Pica;
        case QPrinter::Didot:       return QPageLayout::Didot;
        case QPrinter::Cicero:      return QPageLayout::Cicero;
        case QPrinter::DevicePixel: return QPageLayout::Point;
        }
        return QPageLayout::Point;
    }

    const QRectF pdfPageRect(QPrinter::Unit unit) const {
        return mPdf->pageLayout().paintRect(toPdfUnits(unit));
    }

    List<Marginal> parseMarginal(const QString& marginal);
    void           printMarginal(QPainter& painter, qlonglong y, const Marginal& object, const qreal& xFactor, const qreal& yFactor);
    QString        resolveTag(const QString& key);

public:
    Printer(QPrinter::PrinterMode mode = QPrinter::ScreenResolution)
        : mPrinter(new QPrinter(mode)) { }
    Printer(const QPrinterInfo& printer, QPrinter::PrinterMode mode = QPrinter::ScreenResolution)
        : mPrinter(new QPrinter(printer, mode)) { }
    Printer(const QString& filename)
        : mPdf(new QPdfWriter(filename)) { }
    virtual ~Printer() { }

    static constexpr qreal PointsPerInch    { 72.0 };
    static constexpr bool  WithAlignment    { true };
    static constexpr bool  WithoutAlignment { false };

    void                     addImage(const QUrl& url)                      { loadfUrl(url); }
    const List<qlonglong>&   ids() const                                    { return mIds; }
    const Map<QUrl, QImage>& images() const                                 { return mImages; }
    void                     newPage()                                      { if (mPrinter) mPrinter->newPage(); else mPdf->newPage(); }
    QPageLayout::Orientation pageOrientation()                              { if (mPrinter) return mPrinter->pageLayout().orientation();
                                                                              else return mPdf->pageLayout().orientation(); }
    const QRectF             pageRect(QPrinter::Unit units) const           { return mPrinter ? mPrinter->pageRect(units) : pdfPageRect(units); }
    QPaintDevice*            paintdevice()                                  { return mPrinter ? dynamic_cast<QPaintDevice*>(mPrinter) : dynamic_cast<QPaintDevice*>(mPdf); }
    QPainter                 painter()                                      { return mPrinter ? QPainter(mPrinter) : QPainter(mPdf); }
    const QRectF             paperRect(QPrinter::Unit units) const          { return mPrinter ? mPrinter->paperRect(units) : pdfPageRect(units); }
    const qlonglong          physicalDpiX() const                           { return mPrinter ? mPrinter->logicalDpiX() : mPdf->logicalDpiX(); }
    const qlonglong          physicalDpiY() const                           { return mPrinter ? mPrinter->logicalDpiY() : mPdf->logicalDpiY(); }
    Preferences*             prefs()                                        { return mPrefs; }
    QPrinter*                qprinter()                                     { return mPrinter; }
    QPdfWriter*              qpdfwriter()                                   { return mPdf; }
    void                     setAuthor(const QString& author)               { if (mPdf) mPdf->setAuthor(author); }
    void                     setFullPage(bool to)                           { if (mPrinter) mPrinter->setFullPage(to); }
    void                     setIds(const List<qlonglong>& ids)             { mIds = ids; }
    void                     setImages(const Map<QUrl, QImage>& images)     { mImages = images; }
    void                     setPageOrientation(QPageLayout::Orientation o) { if (mPrinter) mPrinter->setPageOrientation(o); else mPdf->setPageOrientation(o); }
    void                     setOutputFileName(const QString& n)            { if (mPrinter) mPrinter->setOutputFileName(n); }
    void                     setOutputFormat(QPrinter::OutputFormat f)      { if (mPrinter) mPrinter->setOutputFormat(f); }
    void                     setPageSize(QPageSize::PageSizeId mPid)        { if (mPdf) mPdf->setPageSize(mPid); }
    void                     setPageMargins(const QMarginsF& m)             { if (mPdf) mPdf->setPageMargins(m); }
    void                     setPrefs(Preferences* prefs)                   { mPrefs = prefs; }
    void                     setPrinterName(const QString& p)               { if (mPrinter) mPrinter->setPrinterName(p); }
    void                     setTitle(const QString& title)                 { if (mPdf) mPdf->setTitle(title); }

    void loadfUrl(const QUrl& url) {
        auto* nam = new QNetworkAccessManager();
        QNetworkReply* reply = nam->get(QNetworkRequest(url));

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, url]() {
            QByteArray data = reply->readAll();
            reply->deleteLater();
            QImage img(data);
            mImages[url] = img;
        });
    }

    void        footer(QPainter& painter, const qreal& xFactor, const qreal& yFactor);
    void        handleCover(Item& item,
                            bool startingPage,
                            QSizeF& pageSize,
                            QMarginsF& margins,
                            QPainter& painter,
                            qreal xFactor,
                            qreal yFactor,
                            std::function<void()> printer);
    void         header(QPainter& painter, const qreal& xFactor, const qreal& yFactor);
    void         newPage(QPainter& painter, std::function<void()> printer, const qreal& xFactor, const qreal& yFactor, bool marginals = false);
    void         outputNovel(List<qlonglong> ids,
                             const QString& chapterTag,
                             const QString& sceneTag,
                             const QString& coverTag,
                             QPainter& painter,
                             QSizeF pageSize,
                             std::function<void()> pager);
    const QSizeF pageSize(QPrinter::Unit unit) const;
    void         printLine(QPainter& painter, List<Words::Word>& line, QFont& font, Words::Tags& current, qreal x, qreal at, qreal fill,
                           qreal& xFactor, qreal& yFactor);
    void         printNovel();
    void         printParagraphs(QPainter& painter, QSizeF& pageSize, std::function<void()>& pager, QMarginsF& margins, QFont& font, qreal& xFactor, qreal& yFactor,
                                 qreal& at, bool& startingPage, StringList& paragraphs);

    static QString            createParagraph(const QString& html);
    static StringList         createParagraphs(Item& item, bool align = WithoutAlignment);
    static QString            fromHtml(const QString& html);
    static StringList         mergeWordFragments(QList<Words::Word>& words);
    static QList<Words::Word> paragraphWords(const QString& paragraph);
};
