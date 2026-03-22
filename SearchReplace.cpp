#include "SearchReplace.h"
#include "TextEdit.h"

void SearchSelection::buildResults(const QString& text) {
    mTarget = text;
    buildResults();
}

void SearchSelection::buildResults() {
    mResults.clear();
    qlonglong pos = mBegin;
    auto strLen = mSearchString.length();
    if (strLen == 0) return;
    auto splits = StringList { mTarget.split(mSearchString, Qt::KeepEmptyParts,
                                             mCaseInsensitive ? Qt::CaseInsensitive : Qt::CaseSensitive) };
    if (splits.size() > 1) {
        bool first = true;
        for (const auto& split: splits) {
            QString mid = mTarget.mid(pos, strLen);
            if (!mid.compare(mSearchString, mCaseInsensitive ? Qt::CaseInsensitive : Qt::CaseSensitive)) mResults.append(pos);
            if (first) first = false;
            else pos += strLen;
            pos += split.length();
        }
    } else mResults.clear();
}

SearchSelection::SearchSelection(Novel& novel,
                                 qlonglong id,
                                 const QString& str,
                                 qlonglong begin,
                                 qlonglong end,
                                 bool caseInsensitive)
    : SearchCore(novel, str, caseInsensitive)
    , mBegin(begin)
    , mEnd(end)
    , mId(id) {
    Item& item = mNovel.findItem(mId);
    mTarget = item.doc()->toPlainText().mid(mBegin, mEnd - mBegin);
    mIndex = 0;
}

const NovelPosition SearchSelection::findNext() {
    if (mResults.size() < 1) return invalid;
    if (mIndex >= mResults.size()) mIndex = 0;
    return NovelPosition(mId, mResults[mIndex++]);
}

SearchItem::SearchItem(Novel& novel, qlonglong id, qlonglong start, const QString& str, bool caseInsensitive)
    : SearchSelection(novel, id, str, 0, 0, caseInsensitive) {
    Item& item = mNovel.findItem(mId);
    TextEdit edit;
    edit.setHtml(item.html());
    auto text = edit.toPlainText();
    mEnd = text.length();
    buildResults(text);

    qlonglong index = 0;
    for (const auto& result: mResults) {
        if (start >= result) {
            mIndex = index;
            break;
        } else index++;
    }
}

void SearchTree::buildResults(qlonglong id) {
    mTarget = id;
    buildResults();
}

void SearchTree::buildResults() {
    mResults.clear();
    Item& item = mNovel.findItem(mTarget);
    TextEdit edit;
    edit.setHtml(item.html());
    auto text = edit.toPlainText();
    auto strLen = mSearchString.length();
    if (strLen == 0) return;
    qlonglong pos = 0;
    bool first = true;
    auto splits = StringList { text.split(mSearchString, Qt::KeepEmptyParts,
                                        mCaseInsensitive ? Qt::CaseInsensitive : Qt::CaseSensitive) };
    if (splits.size() > 1) {
        for (const auto& split: splits) {
            if (first) first = false;
            else pos += strLen;
            pos += split.length();
            mResults.append(pos);
        }
    } else mResults.clear();
}

void SearchTree::flattenTree(QTreeWidgetItem* branch, qlonglong id) {
    mFlattenedTree.append(id);
    auto num = branch->childCount();
    for (auto i = 0; i < num; ++i) {
        auto* child = branch->child(i);
        qlonglong childId = child->data(0, Qt::UserRole).toLongLong();
        flattenTree(child, childId);
    }
}

SearchTree::SearchTree(Novel& novel,
                       QTreeWidgetItem* tree,
                       qlonglong id,
                       qlonglong start,
                       const QString& str,
                       bool caseInsensitive)
    : SearchCore(novel, str, caseInsensitive)
    , mId(id) {
    qlonglong index = 0;
    for (const auto& result: mResults) {
        if (start >= result) {
            mIndex = index;
            break;
        } else index++;
    }

    flattenTree(tree, id);
}

const NovelPosition SearchTree::findNext() {
    if (mResults.size() < 1 || mIndex >= mResults.size()) {
        if (mIndex >= mResults.size()) mIndex = 0;
        mBranchIndex++;
        if (mBranchIndex > mFlattenedTree.size()) {
            mBranchIndex = 0;
            if (mNeverFound) return invalid;
        }
        buildResults(mFlattenedTree[mBranchIndex]);
        return findNext();
    }
    mNeverFound = false;
    return NovelPosition(mFlattenedTree[mBranchIndex], mResults[mIndex++]);
}
