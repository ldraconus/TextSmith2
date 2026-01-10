#include "MarkdownExporter.h"
#include "HtmlExporter.h"
#include "html2md-main/include/html2md.h"

#include <QDir>
#include <QFileInfo>

#include "Main.h"
#include "ui_Main.h"

class SwapDoc {
private:
    QTextDocument* mDoc;
    QTextEdit*     mEdit;
    QTextDocument* mOrigDoc;
    QString        mOrigHtml;

public:
    SwapDoc(QTextEdit* edit)
        : mEdit(edit)
        , mOrigDoc(edit->document())
        , mOrigHtml(edit->toHtml()) {
        mDoc = new QTextDocument();
        mDoc->setParent(nullptr);
        edit->setDocument(mDoc);
    }
    ~SwapDoc() {
        mEdit->setDocument(mOrigDoc);
        mEdit->setHtml(mOrigHtml);
        delete mDoc;
    }

    QTextDocument* document() { return mDoc; }
};

bool MarkdownExporter::convert() {
    if (mFilename.isEmpty()) return false;

    TextEdit* edit = Main::ref().ui()->textEdit;
    SwapDoc save(edit);
    edit->registerInternalImages();
    const auto& defaults = collectMetadataDefaults();
    QString cover = fetchValue(0, defaults, "cover");
    bool result = HtmlExporter::convert(mNovel, mItemIds, cover, *edit, { mChapterTag, mSceneTag, mCoverTag });
    QString html = edit->toHtml();

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

    QString md = htmlToMarkdown(html);
    QFile file(dir + "/" + base + ext);
    if (!file.open(QIODeviceBase::WriteOnly)) return false;
    return result && (file.write(md.toUtf8()) != -1);
}

QString MarkdownExporter::htmlToMarkdown(const QString& html) {
    QString md = QString::fromStdString(html2md::Convert(html.toStdString()));
    const auto& defaults = collectMetadataDefaults();
    QString title = fetchValue(0, defaults, "title");
    if (!title.isEmpty()) md = "# " + title + "\n" + md;
    return md;
}
