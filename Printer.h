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

class Preferences;

class Printer {
private:
    QList<qlonglong>   mIds;
    QMap<QUrl, QImage> mImages;
    QPdfWriter*        mPdf     { nullptr };
    Preferences*       mPrefs   { nullptr };
    QPrinter*          mPrinter { nullptr };

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

public:
    Printer(QPrinter::PrinterMode mode = QPrinter::ScreenResolution)
        : mPrinter(new QPrinter(mode)) { }
    Printer(const QPrinterInfo& printer, QPrinter::PrinterMode mode = QPrinter::ScreenResolution)
        : mPrinter(new QPrinter(printer, mode)) { }
    Printer(const QString& filename)
        : mPdf(new QPdfWriter(filename)) { }
    virtual ~Printer() { }

    void                      addImage(const QUrl& url)                      { loadfUrl(url); }
    const QList<qlonglong>&   ids() const                                    { return mIds; }
    const QMap<QUrl, QImage>& images() const                                 { return mImages; }
    void                      newPage()                                      { if (mPrinter) mPrinter->newPage(); else mPdf->newPage(); }
    QPageLayout::Orientation  pageOrientation()                              { if (mPrinter) return mPrinter->pageLayout().orientation();
                                                                               else return mPdf->pageLayout().orientation(); }
    const QRectF              pageRect(QPrinter::Unit units) const           { return mPrinter ? mPrinter->pageRect(units) : pdfPageRect(units); }
    QPaintDevice*             paintdevice()                                  { return mPrinter ? dynamic_cast<QPaintDevice*>(mPrinter) : dynamic_cast<QPaintDevice*>(mPdf); }
    QPainter                  painter()                                      { return mPrinter ? QPainter(mPrinter) : QPainter(mPdf); }
    const QRectF              paperRect(QPrinter::Unit units) const          { return mPrinter ? mPrinter->paperRect(units) : pdfPageRect(units); }
    const qlonglong           physicalDpiX() const                           { return mPrinter ? mPrinter->logicalDpiX() : mPdf->logicalDpiX(); }
    const qlonglong           physicalDpiY() const                           { return mPrinter ? mPrinter->logicalDpiY() : mPdf->logicalDpiY(); }
    Preferences*              prefs()                                        { return mPrefs; }
    QPrinter*                 qprinter()                                     { return mPrinter; }
    QPdfWriter*               qpdfwriter()                                   { return mPdf; }
    void                      setAuthor(const QString& author)               { if (mPdf) mPdf->setAuthor(author); }
    void                      setFullPage(bool to)                           { if (mPrinter) mPrinter->setFullPage(to); }
    void                      setIds(const QList<qlonglong>& ids)            { mIds = ids; }
    void                      setImages(const QMap<QUrl, QImage>& images)    { mImages = images; }
    void                      setPageOrientation(QPageLayout::Orientation o) { if (mPrinter) mPrinter->setPageOrientation(o); else mPdf->setPageOrientation(o); }
    void                      setOutputFileName(const QString& n)            { if (mPrinter) mPrinter->setOutputFileName(n); }
    void                      setOutputFormat(QPrinter::OutputFormat f)      { if (mPrinter) mPrinter->setOutputFormat(f); }
    void                      setPageSize(QPageSize::PageSizeId mPid)        { if (mPdf) mPdf->setPageSize(mPid); }
    void                      setPageMargins(const QMarginsF& m)             { if (mPdf) mPdf->setPageMargins(m); }
    void                      setPrefs(Preferences* prefs)                   { mPrefs = prefs; }
    void                      setPrinterName(const QString& p)               { if (mPrinter) mPrinter->setPrinterName(p); }
    void                      setTitle(const QString& title)                 { if (mPdf) mPdf->setTitle(title); }

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

    const QSizeF pageSize(QPrinter::Unit unit) const;
};
