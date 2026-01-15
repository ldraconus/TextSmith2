#include "HtmlExporter.h"

#include <QDir>
#include <QFileInfo>
#include <QTextBlock>
#include <QUrl>

#include "Main.h"

void HtmlExporter::extracted(QString& str, const QString& dir, QMap<QUrl, QImage>& images) {
    auto keys = images.keys();
    for (auto i = 0; i < keys.size(); ++i) {
        const auto& key = keys[i];
        const auto& image = images[key];
        QString name = key.fileName();
        QString newPath = dir + "/internal_" + name + ".png";
        image.save(newPath, "PNG");
        QString oldUrl = key.toString();
        str = str.replace(oldUrl, newPath);
    }
}

bool HtmlExporter::convert() {
    if (mFilename.isEmpty())
        return false;

    const auto& defaults = collectMetadataDefaults();
    QString cover = fetchValue(1, defaults, "cover");
    QList<QString> tags = { mChapterTag, mSceneTag, (cover.isEmpty()) ? mCoverTag : "" };
    QString html = convert(mNovel, mItemIds, cover, tags);
    QString title = fetchValue(0, defaults, "title");
    if (!title.isEmpty()) {
        if (html.contains("<head>"))
            html.replace("<head>", QString("<head><title>%1</title>").arg(title.toHtmlEscaped()));
        else
            html.prepend(QString("<title>%1</title>").arg(title.toHtmlEscaped()));
    }

    // create the <path>/<basename>
    //            <path>/<basename>/index.html
    QFileInfo info(mFilename);
    QString path = info.absolutePath();
    QString base = info.baseName();
    QString ext = "." + info.completeSuffix();
    Main::ref().setDocDir(path);

    QString dir = path + "/" + base;
    QDir work;
    work.mkpath(dir);

    // save all of the internal files as
    // <path>/<basename>/internal_<url_filename>.png and re-write html
    extracted(html, dir, mImages);

    QFile file(dir + "/index" + ext);
    if (!file.open(QIODeviceBase::WriteOnly))
        return false;
    return file.write(html.toUtf8()) != -1;
}

QString HtmlExporter::addParagraphs(const QString& html) {
    QString work = html;
    auto pos = work.indexOf("<p ");
    work = work.mid(pos);
    QString paragraphs;
    while ((pos = work.indexOf("<p ")) != -1) {
        auto end = work.indexOf("</p>");
        auto paragraph = work.mid(pos, end - pos + 4);
        if (!paragraph.startsWith("<p style=\"-qt-paragraph-type:empty")) {
            if (!paragraphs.isEmpty()) paragraphs += "\n";
            paragraphs += paragraph;
        }
        work = work.mid(end + 4);
    }
    return paragraphs;
}

static constexpr int Chapter = 0;
static constexpr int Cover   = 2;
static constexpr int Scene   = 1;

QString HtmlExporter::convert(Novel& novel, QList<qlonglong>& ids, const QString& cover, const QList<QString>& tag) {
    QString html = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                   "<html>\n"
                   "    <head>\n"
                   "        <meta name=\"qrichtext\" content=\"1\" />\n"
                   "        <meta charset=\"utf-8\" />\n"
                   "        <style type=\"text/css\">\n"
                   "            p, li { white-space: pre-wrap; }\n"
                   "            hr { height: 1px; border-width: 0; }\n"
                   "            li.unchecked::marker { content: \"\\2610\"; }\n"
                   "            li.checked::marker { content: \"\\2612\"; }\n"
                   "            img {\n"
                   "                margin: 0;\n"
                   "                padding: 0;\n"
                   "                background: #000;\n"
                   "                align-items: center;\n"
                   "                justify-content: center;\n"
                   "                max-width: 100%;\n"
                   "                max-height: 100%;\n"
                   "                width: auto;\n"
                   "                height: auto;\n"
                   "                object-fit: contain;\n"
                   "                display: block;\n"
                   "            }\n"
                   "        </style>\n"
                   "    </head>\n"
                   "    <body style=\" font-family:'Segoe UI'; font-size:14pt; font-weight:400; font-style:normal;\">\n";
    QString final = "    </body>\n"
                    "</html>";
    if (!cover.isEmpty()) html += generateImageHtml(cover);
    for (auto& id: ids) {
        Item& item = novel.findItem(id);
        if (item.hasTag(tag[Chapter]) || item.hasTag(tag[Scene]) || item.hasTag(tag[Cover])) {
            html += "\n" + addParagraphs(item.html());
            continue;
        }
    }
    return html + final;
}

QString HtmlExporter::generateImageHtml(const QString& url) {
    return "<p><img src=\"" + url + "\" alt=\"Picture\"></p>\n";
}
