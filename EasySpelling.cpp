#include "EasySpelling.h"

#include <QDir>
#include <QTemporaryFile>
#include <QThread>

#include <string>
#include <vector>

#include <hunspell.hxx>

EasySpelling::EasySpelling() {
    std::thread([this] {
        QDir tempDir(QDir::temp().filePath("TextSmith2_dict"));
        tempDir.mkpath(".");
        mDir = tempDir.absolutePath();

        mAffPath = tempDir.filePath("en_us.aff");
        mDictPath = tempDir.filePath("en_us.dic");

        QFile(":/dict/index.aff").copy(mAffPath);
        QFile(":/dict/index.dic").copy(mDictPath);

        mEngine = std::make_unique<Hunspell>(this->mAffPath.toStdString().c_str(), this->mDictPath.toStdString().c_str());

        mReady.store(true, std::memory_order_release);
    }).detach();
}

EasySpelling::~EasySpelling() {
    mEngine.release();
    QDir tempDir(mDir);
    tempDir.removeRecursively();
}

StringList EasySpelling::check(const QString& word) {
    StringList suggestions;
    if (mWords.contains(word)) return suggestions;

    std::vector<std::string> fromEngine;
    auto spellWord = word.toUtf8().constData();
    fromEngine = mEngine->suggest(spellWord);
    for (auto& s: fromEngine) suggestions << QString::fromUtf8(s.c_str());
    return suggestions;
}
