#pragma once

#include <QObject>
#include <QTextToSpeech>
#include <QTextEdit>
#include <StringList.h>

#include <List.h>

class Speech: public QObject {
    Q_OBJECT

public:
    explicit Speech(QObject *parent = nullptr);

    void setVoice(const qlonglong voice);
    void speak(const QString& text);

public slots:
    void stop();

signals:
    void speakingStarted();
    void speakingFinished();
    void speaking(const QString& text, qlonglong start);
    void errorOccurred(const QString &error);

private:
    struct Sentence {
        QString mSentence;   // Without trailing punctuation/closers
        QString mPunct;      // e.g., ".", "?!", "…" plus optional "\"”
        int     mStart = -1; // Start offset in the original text
    };

    int             mCurrentIndex { 0 };
    List<Sentence>  mSentences;
    bool            mNeedsPrimed { true };
    QTextToSpeech   mTts;
    QVoice          mVoice;

    void extractSentences(const QString& text);
    void highlightSentence(const Sentence& sentence);
    void speakNextSentence();
};
