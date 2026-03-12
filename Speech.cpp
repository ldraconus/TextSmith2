#include "Speech.h"
#include <QRegularExpression>
#include <QTextCursor>

#include <StringList.h>

/*
System-Level TTS Backends: Qt 6 on Linux relies on system TTS engines. Install one of the following:

-->Festival: sudo apt install festival festvox-us-slt-hts (supports natural voices like cmu_us_slt_arctic_hts).

SVOX Pico (pico2wave): sudo apt install libttspico0 libttspico-utils — lightweight and decent for basic use.

espeak-ng: sudo apt install espeak-ng — modern alternative to espeak.

Coqui TTS (AI-based, high quality, offline): Install via pipx install coqui-tts, then use with tts --text "Hello" --pipe_out | aplay.

Verify Qt6 TTS Works: Use a minimal Qt6 C++ or Python app with QTextToSpeech. Ensure the backend is detected via QTextToSpeech.availableEngines().

Note: Qt6 TTS performance on Linux is dependent on the underlying system engine.  While Qt6 has improved support, results may vary compared to Windows.
For best results, use Coqui TTS or Festival with high-quality voices.

Install the voice package directly via APT:

sudo apt install festvox-us-slt-hts

This package provides the high-quality HTS-based voice and integrates automatically with Festival.
*/

Speech::Speech(QObject* parent)
    : QObject(parent) {
    // Explicitly check available engines, fallback if winrt isn't there
    // (Optional safety, but good practice)
    //if (QTextToSpeech::availableEngines().contains("winrt")) mTts = new QTextToSpeech("winrt", this);
    //else
    mTts = textEngine(this);

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

bool Speech::speechAvailable(QObject* ths) {
    auto speech = textEngine(ths);
    bool available = !(speech == nullptr || speech->state() == QTextToSpeech::Error || speech->availableVoices().isEmpty());
    delete speech;
    return available;
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

QTextToSpeech *Speech::textEngine(QObject* ths) {
#ifdef Q_OS_WIN
    return new QTextToSpeech("sapi", ths);
#else
    return new QTextToSpeech("", ths);
#endif
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
