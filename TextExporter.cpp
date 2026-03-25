#include "TextExporter.h"

#include <QDir>
#include <QFileInfo>

#include "Main.h"

bool TextExporter::convert() {
    if (mFilename.isEmpty()) return false;

    const auto& defaults = collectMetadataDefaults();
    QFileInfo info(mFilename);
    QString path = info.absolutePath();
    QString base = info.baseName();
    QString ext =  "." + (info.completeSuffix().isEmpty() ? Extension() : info.completeSuffix());
    Main::ref().setDocDir(path);

    QString dir = path;

    QString text;
    for (auto&& id: mItemIds) {
        Item& item = Main::ref().novel().findItem(id);
        text += item.doc()->toPlainText() + "\n";
    }

    QFile file(dir + "/" + base + ext);
    if (file.open(QIODeviceBase::WriteOnly)) return file.write(text.toUtf8()) != -1;
    return false;
}
