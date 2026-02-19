#include "Speech.h"
#include <QRegularExpression>
#include <QTextCursor>

#include <StringList.h>

Speech::Speech(QObject* parent)
    : QObject(parent) {
    // Explicitly check available engines, fallback if winrt isn't there
    // (Optional safety, but good practice)
    //if (QTextToSpeech::availableEngines().contains("winrt")) mTts = new QTextToSpeech("winrt", this);
    //else
    mTts = new QTextToSpeech("sapi", this);

    connect(mTts, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state) {
        if (mPaused) return;
             if (state == QTextToSpeech::Ready && mCurrentIndex < mSentences.size()) speakNextSentence();
        else if (state == QTextToSpeech::Ready && mCurrentIndex >= mSentences.size()) emit speakingFinished();
        else if (state == QTextToSpeech::Speaking) emit speakingStarted();
        else if (state == QTextToSpeech::Error) emit errorOccurred(mTts->errorString());
    });
}

void Speech::setVoice(qlonglong voiceIdx) {
    auto voices = mTts->availableVoices();
    if (voices.isEmpty()) return;

    StringList voiceNames;
    for (auto& voice: voices) voiceNames << voice.name();
    voiceNames.sort();
    if (voiceIdx >= voices.size() || voiceIdx < 0) voiceIdx = 0;
    mTts->setVoice(voices[voiceIdx]);
}

void Speech::extractSentences(const QString& text) {
    QRegularExpression re(
        R"((.+?)([\.\?\!…]+)(["'\)\]\}»”]*)\s*)",
        QRegularExpression::DotMatchesEverythingOption
        );

    mSentences.clear();
    int pos = 0;
    while (pos < text.size()) {
        auto m = re.match(text, pos);
        if (!m.hasMatch()) {
            // Trailing remainder without terminal punctuation: treat as final sentence if non-empty
            QString tail = text.mid(pos).trimmed();
            if (!tail.isEmpty()) {
                Sentence s;
                s.mSentence = tail;
                s.mPunct = QString();
                s.mStart = pos;
                mSentences.push_back(std::move(s));
            }
            return;
        }

        const int start = m.capturedStart(0);
        const int end   = m.capturedEnd(0);

        Sentence s;
        s.mSentence = m.captured(1).trimmed();
        s.mPunct    = m.captured(2) + m.captured(3);
        s.mStart    = start;
        mSentences.push_back(std::move(s));

        pos = end;
    }

    return;
}


void Speech::speak(const QString& text) {
    // Split into sentences
    stop();

    extractSentences(text);
    if (mSentences.isEmpty()) return;

    mCurrentIndex = 0;
    mPaused = false;
    speakNextSentence();
}

void Speech::speakNextSentence() {
/*
    if (mNeedsPrimed) {
        mNeedsPrimed = false;
        mTts->say(" ");
        return;
    }
*/
    if (mCurrentIndex < mSentences.size()) {
        const Sentence& data = mSentences[mCurrentIndex];
        QString sentence = data.mSentence.trimmed() + data.mPunct;
        highlightSentence(data);
        mTts->say(sentence);
        ++mCurrentIndex;
    }
}

void Speech::highlightSentence(const Sentence &sentence) {
    emit speaking(sentence.mSentence, sentence.mStart);
}

void Speech::stop() {
    mPaused = true;
    mTts->stop();
    mSentences.clear();
    mCurrentIndex = 0;
    mNeedsPrimed = true;
}
