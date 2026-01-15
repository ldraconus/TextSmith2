#include "SoundPool.h"

#include <QTimer>

SoundPool::SoundPool(QObject* parent)
    : QObject(parent) {
}

void SoundPool::load(Sound sound, const QUrl& url, int voices) {
    Bank bank;

    for (int i = 0; i < voices; ++i) {
        Voice v;
        v.mOutput = std::make_unique<QAudioOutput>();
        v.mPlayer = std::make_unique<QMediaPlayer>();

        v.mPlayer->setAudioOutput(v.mOutput.get());
        v.mPlayer->setSource(url);

        // Pre-buffer the audio
        v.mPlayer->setLoops(1);
        if (i == 0) { // Warm up the 1st voice so FFmpeg + audio backing don't lag on 1st keypress.
            v.mOutput->setVolume(0.0);
            v.mPlayer->play();
            v.mPlayer->stop();
        }
        v.mOutput->setVolume(1.0);

        bank.mVoices.push_back(std::move(v));
    }

    mBanks[sound] = std::move(bank);
}

void SoundPool::play(Sound sound) {
    if (!mBanks.contains(sound)) return;

    Bank& bank = mBanks[sound];

    Voice& v = bank.mVoices[bank.mIndex];
    bank.mIndex = (bank.mIndex + 1) % bank.mVoices.size();

    // Restart from beginning
    v.mPlayer->stop();
    v.mPlayer->setPosition(0);
    v.mPlayer->play();
}

void SoundPool::playDelayed(Sound sound, int ms) {
    QTimer::singleShot(ms, this, [this, sound]() {
        play(sound);
    });
}
