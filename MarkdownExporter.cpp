#include "MarkdownExporter.h"
#include "HtmlExporter.h"

#include <QDir>
#include <QFileInfo>

#include "Main.h"

QString MarkdownExporter::paragraphs(const QString& html) {
    QString work = html;
    auto pos = work.indexOf("<p ");
    work = work.mid(pos);
    QString paragraphs;
    while ((pos = work.indexOf("<p ")) != -1) {
        auto end = work.indexOf("</p>");
        auto paragraph = work.mid(pos + 3, end - pos - 3);
        if (paragraph.startsWith("style=\"-qt-paragraph-type:empty")) continue;
        end = paragraph.indexOf(">");
        if (end == -1) continue;
        paragraph = paragraph.mid(end + 1);
        if (!paragraphs.isEmpty()) paragraphs += "\n\n";
        paragraphs += paragraph;
        work = work.mid(end + 4);
    }
    return paragraphs;
}

bool MarkdownExporter::convert() {
    if (mFilename.isEmpty()) return false;

    const auto& defaults = collectMetadataDefaults();
    QString cover = fetchValue(0, defaults, "cover");
    QString html = HtmlExporter::convert(mNovel, mItemIds, cover, { mChapterTag, mSceneTag, mCoverTag });

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

    QString md = htmlToMarkdown(html, dir);
    HtmlExporter::extracted(md, dir, mImages);

    QFile file(dir + "/" + base + ext);
    if (file.open(QIODeviceBase::WriteOnly)) return file.write(md.toUtf8()) != -1;
    return false;
}

void MarkdownExporter::convertHex(QString& paragraphs) {
    qlonglong pos = 0;
    while ((pos = paragraphs.indexOf("&#x")) != -1) {
        auto end = paragraphs.indexOf(";", pos);
        if (end == -1) continue;
        QString hexStr = paragraphs.mid(pos + 3, end - pos - 3);
        QString search = paragraphs.mid(pos, end - pos);
        bool ok = true;
        int hex = hexStr.toInt(&ok, 16);
        if (ok) paragraphs.replace(search, QChar(hex));
    }
}

void MarkdownExporter::convertDecimal(QString paragraphs)
{
    qlonglong pos;
    while ((pos = paragraphs.indexOf("&#")) != -1) {
        auto end = paragraphs.indexOf(";", pos);
        if (end == -1) continue;
        QString numStr = paragraphs.mid(pos + 2, end - pos - 2);
        QString search = paragraphs.mid(pos, end - pos);
        bool ok = true;
        int num = numStr.toInt(&ok);
        if (ok) paragraphs.replace(search, QChar(num));
    }
}

QString MarkdownExporter::htmlToMarkdown(const QString& html, const QString& dir) {
    static std::unordered_map<QString, QString> conversions {
        { "<b>",       { "**", } },
        { "</b>",      { "**", } },
        { "<i>",       { "*",  } },
        { "</i>",      { "*",  } },
        { "<img src=", { "![](", } },
        { "\">",       { ")", } },
        { "&quot;",    { "\"" } },
        { "&apos;",    { "'" } },
        { "&nbsp;",    { " " } },
        { "&emsp;",    { " " } },
        { "&ensp;",    { " " } },
        { "&thinsp;",  { " " } },
        { "&ndash;",   { "-" } },
        { "&mdash;",   { "-" } },
        { "&lsquo;",   { "‘" } },
        { "&rsquo;",   { "’" } },
        { "&ldquo;",   { "“" } },
        { "&rdquo;",   { "”" } },
        { "&hellip;",  { "…" } },
        { "&copy;",    { "©" } },
        { "&reg;",     { "®" } },
        { "&trade;",   { "™" } },
        { "&euro;",    { "€" } },
        { "&pound;",   { "£" } },
        { "&yen;",     { "¥" } },
        { "&rarr;",    { "→" } },
        { "&larr;",    { "←" } },
        { "&uarr;",    { "↑" } },
        { "&darr;",    { "↓" } },
    };

    const auto& defaults = collectMetadataDefaults();
    QString title = fetchValue(0, defaults, "title");
    QString paras = paragraphs(html);
    if (!title.isEmpty()) paras = "# " + title + "\n" + paras;
    for (const auto& conversion: conversions) paras.replace(conversion.first, conversion.second, Qt::CaseInsensitive);

    convertHex(paras);
    convertDecimal(paras);

    paras.replace("&lt;", "<", Qt::CaseInsensitive);
    paras.replace("&gt;", ">", Qt::CaseInsensitive);

    QString prev;
    while (paras != prev) {
        prev = paras;
        paras.replace("&amp;", "&");
    }

    return paras;
}
