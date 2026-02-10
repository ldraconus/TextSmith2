#pragma once

#include <QObject>

namespace Words {

    enum class Tags {
        None =       0,
        Bold =       1 << 0,
        Image =      1 << 1,
        Italic =     1 << 2,
        Underline =  1 << 3,
        Partial =    1 << 4,
        AllFormats = Bold + Italic + Underline,
        All = AllFormats + Partial
    };

    inline Tags operator|(const Tags a, const Tags b) {
        return static_cast<Tags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline Tags operator&(const Tags a, const Tags b) {
        return static_cast<Tags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline Tags operator|=(Tags& a, const Tags b) {
        a = a | b;
        return a;
    }

    inline Tags operator&=(Tags& a, const Tags b) {
        a = a & b;
        return a;
    }

    inline Tags operator~(const Tags a) {
        return static_cast<Tags>(~static_cast<uint32_t>(a) & static_cast<uint32_t>(Tags::All));
    }

    class Word {
    private:
        Tags    mTags { Tags::None };
        QString mStr;

    public:
        Word() { }
        Word(const QString& s, Tags t = Tags::None)
            : mTags(t)
            , mStr(s) { }

        void clear()                     { mStr.clear(); mTags = Tags::None; }
        bool hasTag(const Tags& t) const { return (mTags & t) == t; }
        bool isBold() const              { return hasTag(Tags::Bold); }
        bool isImage() const             { return hasTag(Tags::Image); }
        bool isItalic() const            { return hasTag(Tags::Italic); }
        bool isUnderline() const         { return hasTag(Tags::Underline); }
        bool isPartial() const           { return hasTag(Tags::Partial); }
        bool isNull() const              { return mTags == Tags::None; }
        bool isPlain() const             { return isNull(); }

        QString str() const              { return mStr; }
        void    setStr(const QString& s) { mStr = s; }
        Tags    tags() const             { return mTags; }
        void    setTags(const Tags& t)   { mTags = t; }

        Word& operator+=(const Tags& t)  { mTags |= t; return *this; }
        Word& operator-=(const Tags& t)  { mTags &= ~t; return *this; }
    };

}
