#pragma once

#include <QImage>
#include <QMap>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPrinter>
#include <QPrinterInfo>
#include <QUrl>

class Printer: public QPrinter {
private:
    QList<qlonglong>   mIds;
    QMap<QUrl, QImage> mImages;
    QPrinter*          mPrinter { nullptr };

public:
    Printer(QPrinter::PrinterMode mode = QPrinter::ScreenResolution)
        : mPrinter(new QPrinter(mode)) { }
    Printer(const QPrinterInfo& printer, QPrinter::PrinterMode mode = QPrinter::ScreenResolution)
        : mPrinter(new QPrinter(printer, mode)) { }
    virtual ~Printer() { }

    void                      addImage(const QUrl& url)                   { loadfUrl(url); }
    const QList<qlonglong>&   ids() const                                 { return mIds; }
    const QMap<QUrl, QImage>& images() const                              { return mImages; }
    QPrinter*                 qprinter()                                  { return mPrinter; }
    void                      setIds(const QList<qlonglong>& ids)         { mIds = ids; }
    void                      setImages(const QMap<QUrl, QImage>& images) { mImages = images; }

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
};
