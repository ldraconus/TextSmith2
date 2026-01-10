#include "HtmlExporter.h"

#include <QDir>
#include <QFileInfo>
#include <QTextBlock>
#include <QUrl>

#include "Main.h"
#include "ui_Main.h"

bool HtmlExporter::convert() {
    if (mFilename.isEmpty()) return false;

    TextEdit* doc = Main::ref().ui()->textEdit;
    QString preHtml = doc->toHtml();
    QTextDocument* preDoc = doc->document();
    QTextDocument* tempDoc = new QTextDocument();
    tempDoc->setParent(nullptr);
    doc->setDocument(tempDoc);
    doc->registerInternalImages();
    const auto& defaults = collectMetadataDefaults();
    QString cover = fetchValue(1, defaults, "cover");
    bool result = convert(mNovel, mItemIds, cover, *doc, { mChapterTag, mSceneTag, mCoverTag });

    QString title = fetchValue(0, defaults, "title");

    QString html = doc->toHtml();
    if (!title.isEmpty()) {
        if (html.contains("<head>")) html.replace("<head>", QString("<head><title>%1</title>").arg(title.toHtmlEscaped()));
        else html.prepend(QString("<title>%1</title>").arg(title.toHtmlEscaped()));
    }

    // create the <path>/<basename>
    //            <path>/<basename>/index.html
    QFileInfo info(mFilename);
    QString path = info.absolutePath();
    QString base = info.baseName();
    QString ext =  "." + info.completeSuffix();
    Main::ref().setDocDir(path);

    QString dir = path + "/" + base;
    QDir work;
    work.mkpath(dir);

    // save all of the internal files as <path>/<basename>/internal_<url_filename>.png and re-write html
    auto keys = mImages.keys();
    for (auto i = 0; i < keys.size(); ++i) {
        const auto& key = keys[i];
        const auto& image = mImages[key];
        QString name = key.fileName();
        QString newPath = dir + "/internal_" + name + ".png";
        image.save(newPath, "PNG");
        QString oldUrl = key.toString();
        html = html.replace(oldUrl, newPath);
    }
    doc->setDocument(preDoc);
    doc->setHtml(preHtml);

    QFile file(dir + "/index" + ext);
    if (!file.open(QIODeviceBase::WriteOnly)) return false;
    return result && (file.write(html.toUtf8()) != -1);
}

void HtmlExporter::pageBreak(QTextDocument* doc, const QString& html) {
    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::End);

    int startPos = cursor.position();

    // Insert the HTML
    cursor.insertHtml(html);

    // Find the first block that belongs to the inserted HTML
    QTextBlock block = doc->findBlock(startPos);
    if (!block.isValid())
        return; // nothing inserted, bail

    // In many cases, the HTML starts in this block;
    // in others, it may be the next one. You can adjust if needed.
    QTextCursor blockCursor(block);
    QTextBlockFormat fmt = blockCursor.blockFormat();
    fmt.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
    blockCursor.setBlockFormat(fmt);
}

bool HtmlExporter::convert(Novel& novel, QList<qlonglong>& ids, const QString& cover, TextEdit& edit,
                           const QList<QString>& tag) {
    if (!cover.isEmpty()) {
        auto cursor = edit.textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertHtml(generateCoverHtml(cover));
    }
    for (auto& id: ids) {
        Item& item = novel.findItem(id);
        if (item.hasTag(tag[0])) {
            pageBreak(edit.document(), item.html());
            continue;
        } else if (item.hasTag(tag[2])) {
            QString html = item.html();
            StringList imgParts { html.split("<img src=\"") };
            for (auto i = 1; i < imgParts.count(); ++i) {
                StringList name { imgParts[i].split("\"")};
                if (name.count() >= 2) {
                    auto cursor = edit.textCursor();
                    cursor.movePosition(QTextCursor::End);
                    cursor.insertHtml(generateCoverHtml(name[0]));
                    continue;
                }
            }
            if (imgParts.count() > 1) continue;
        } else if (!item.hasTag(tag[1])) continue;
        auto cursor = edit.textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertHtml(item.html());
    }
    return true;
}

bool HtmlExporter::convert(Novel& novel,
                           QList<qlonglong>& ids,
                           const QString& cover, QTextDocument& doc,
                           const QList<QString>& tag) {
    QTextCursor cursor(&doc);
    if (!cover.isEmpty()) {
        cursor.insertHtml(generateCoverHtml(cover));
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 0);
    }
    for (auto& id: ids) {
        Item& item = novel.findItem(id);
        if (item.hasTag(tag[0])) {
            pageBreak(&doc, item.html());
            continue;
        } else if (item.hasTag(tag[2])) {
            QString html = item.html();
            StringList imgParts { html.split("<img src=\"") };
            for (auto i = 1; i < imgParts.count(); ++i) {
                StringList name { imgParts[i].split("\">")};
                if (name.count() >= 2) {
                    cursor.insertHtml(generateCoverHtml(name[0]));
                    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 0);
                }
            }
            if (imgParts.count() > 1) continue;
        } else if (!item.hasTag(tag[1])) continue;
        cursor.insertHtml(item.html());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 0);
    }
    return true;
}

QString HtmlExporter::generateCoverHtml(const QString& cover) {
    return "<!DOCTYPE html>\n"
           "<html lang=\"en\">\n"
           "    <head>\n"
           "        <meta charset=\"utf-8\">\n"
           "        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
           "        <style>\n"
           "             html, body {\n"
           "                 height: 100%;\n"
           "                 display: flex;\n"
           "             }\n"
           "             img {\n"
           "                 max-width: 100%;\n"
           "                 max-height: 100%;\n"
           "                 width: auto;\n"
           "                 height: auto;\n"
           "                 object-fit: contain;\n"
           "                 display: block;\n"
           "            }\n"
           "        </style>\n"
           "    </head>\n"
           "    <body>\n"
           "        <img src=\"" + cover + "\" alt=\"Cover\">\n"
           "    </body>\n"
           "</html>\n";
}
