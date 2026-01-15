#pragma once

#include <QObject>

#include <atomic>
#include <memory>

#include <StringList.h>

class Hunspell;

class EasySpelling
{
public:
    EasySpelling();
    virtual ~EasySpelling();

    bool ready() { return mReady.load(std::memory_order_acquire); }

    StringList check(const QString& word);

    const StringList& words() const   { return mWords; }
    void setWords(StringList& words)  { mWords = words; }
    void addWord(const QString& word) { mWords += word; }
    void clearWords()                 { mWords.clear(); }

private:
    QString                   mAffPath;
    QString                   mDictPath;
    QString                   mDir;
    std::unique_ptr<Hunspell> mEngine;
    std::atomic<bool>         mReady { false };
    StringList                mWords;
 };
