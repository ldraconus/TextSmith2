#pragma once

#include <QTreeWidgetItem>
#include <QObject>

#include "Novel.h"

#include <List.h>
#include <StringList.h>

class NovelPosition {
private:
    qlonglong mId           { -1 };
    qlonglong mTextPosition { -1 };

public:
    NovelPosition() { }
    NovelPosition(qlonglong id, qlonglong textPosition)
        : mId(id)
        , mTextPosition(textPosition) { }

    bool operator==(const NovelPosition& a) { return mId == a.mId && mTextPosition == a.mTextPosition; }

    qlonglong id()           { return mId; }
    bool      empty()        { return isEmpty(); }
    bool      isEmpty()      { return mId < 0; }
    bool      isValid()      { return isEmpty() && mTextPosition != -1; }
    qlonglong textPosition() { return mTextPosition; }
    bool      valid()        { return isValid(); }
};

class SearchCore {
protected:
    bool    mCaseInsensitive { true };
    Novel&  mNovel;
    QString mSearchString;

public:
    static constexpr bool CaseInsensitive    = true;
    static constexpr bool NotCaseInsensitive = false;

    const NovelPosition invalid { NovelPosition() };

    SearchCore(Novel& novel, const QString& str, bool caseInsensitive = NotCaseInsensitive)
        : mCaseInsensitive(caseInsensitive)
        , mNovel(novel)
        , mSearchString(str) { }
    virtual ~SearchCore() { }

    virtual const NovelPosition findNext() = 0;

    void setText(const QString& s) { mSearchString = s; }

    QString text() const { return mSearchString; }
};

class SearchSelection: public SearchCore {
protected:
    qlonglong       mBegin;
    qlonglong       mEnd;
    qlonglong       mId;
    qlonglong       mIndex;
    List<qlonglong> mResults;

    void setBegin(qlonglong b) { mBegin = b; }
    void setEnd(qlonglong e)   { mEnd = e; }

    void buildResults(const QString& text);

public:
    SearchSelection() = delete;
    SearchSelection(Novel& novel,
                    qlonglong id,
                    const QString& str,
                    qlonglong begin,
                    qlonglong end,
                    bool caseInsensitive = NotCaseInsensitive);

    const NovelPosition findNext() override;
};

class SearchItem: public SearchSelection {
public:
    SearchItem() = delete;
    SearchItem(Novel& novel, qlonglong id, qlonglong start, const QString& str, bool caseInsensitive = NotCaseInsensitive);
};

class SearchTree: public SearchCore {
protected:
    qlonglong       mIndex;
    qlonglong       mId;
    List<qlonglong> mFlattenedTree;
    bool            mNeverFound { true };
    List<qlonglong> mResults;
    qlonglong       mBranchIndex;

    void buildResults(qlonglong id);
    void flattenTree(QTreeWidgetItem* tree, qlonglong id);

public:
    SearchTree() = delete;
    SearchTree(Novel& novel,
               QTreeWidgetItem* branch,
               qlonglong id,
               qlonglong start,
               const QString& str,
               bool caseInsensitive = NotCaseInsensitive);

    const NovelPosition findNext() override;

};
