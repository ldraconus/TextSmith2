#pragma once
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QUrl>
#include <QVector>

#include <memory>
#include <unordered_map>
#include <vector>

class SoundPool : public QObject {
    Q_OBJECT

public:
    explicit SoundPool(QObject* parent = nullptr);

    enum class Sound {
        KeyWhack,
        SpaceThunk,
        ReturnRoll,
        ReturnClunk,
        MarginDing
    };

    // Preload all sounds
    void load(Sound sound, const QUrl& url, int voices = 4);

    // Play instantly (no lag, no waiting)
    void play(Sound sound);
    void playDelayed(Sound sound, int ms);

    struct Voice {
        std::unique_ptr<QMediaPlayer> mPlayer;
        std::unique_ptr<QAudioOutput> mOutput;

        Voice() = default;

        // Move constructor and move assignment
        Voice(Voice&&) = default;
        Voice& operator=(Voice&&) = default;

        // Delete copy constructor and copy assignment
        Voice(const Voice&) = delete;
        Voice& operator=(const Voice&) = delete;
    };

private:
    struct Bank {
        std::vector<Voice> mVoices;
        int                mIndex = 0; // round-robin
    };

    std::unordered_map<Sound, Bank> mBanks;
};

Q_DECLARE_TYPEINFO(SoundPool::Voice, Q_RELOCATABLE_TYPE);
