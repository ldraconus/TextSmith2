#include "Speech.h"
#include <QRegularExpression>
#include <QTextCursor>

#include <StringList.h>

Speech::Speech(QObject* parent)
    : QObject(parent) {
    mTts = new QTextToSpeech("winrt", this);
    connect(mTts, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state) {
             if (state == QTextToSpeech::Ready && !mPaused && mCurrentIndex < mSentences.size()) speakNextSentence();
        else if (state == QTextToSpeech::Ready && mCurrentIndex >= mSentences.size()) emit speakingFinished();
        else if (state == QTextToSpeech::Speaking) emit speakingStarted();
        else if (state == QTextToSpeech::Error) emit errorOccurred(mTts->errorString());
    });
}

void Speech::setVoice(qlonglong voiceIdx) {
    auto voices = mTts->availableVoices();
    StringList voiceNames;
    for (auto& voice: voices) voiceNames << voice.name();
    voiceNames.sort();
    if (voiceIdx >= voices.size() || voiceIdx < 0) voiceIdx = 0;
    auto name = voiceNames[voiceIdx];
    for (voiceIdx = 0; voiceIdx < voices.size(); ++voiceIdx) {
        if (voices[voiceIdx].name() == name) break;
    }
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
    extractSentences(text);
    mCurrentIndex = 0;

    speakNextSentence();
}

void Speech::speakNextSentence() {
    if (mNeedsPrimed) {
        mNeedsPrimed = false;
        mTts->say("One...");
        speakNextSentence();
        return;
    }
    if (mCurrentIndex < mSentences.size()) {
        Sentence data = mSentences[mCurrentIndex++];
        QString sentence = data.mSentence.trimmed() + data.mPunct;
        highlightSentence(data);
        mTts->say(sentence);
    }
}

void Speech::highlightSentence(const Sentence &sentence) {
    emit speaking(sentence.mSentence, sentence.mStart);
}

void Speech::stop() {
    mTts->stop();
    mSentences.clear();
    mNeedsPrimed = true;
    mCurrentIndex = 0;
}
