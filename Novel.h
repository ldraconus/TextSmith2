#pragma once

#include <QString>

#include <Json5.h>
#include <Map.h>
#include <StringList.h>

class Item {
public:
    Item();
    explicit Item(Json5Object& obj);

    virtual void clear();
    virtual void init();

    auto& children()   { return mChildren; }
    auto  html()       { return mHtml; }
    auto  id() const   { return mID; }
    auto  name() const { return mName.isEmpty() ? "<unnamed_#" + QString::number(mID) + ">" : mName; }
    auto& order()      { return mOrder; }

    void addTag(const QString& tag)       { if (!hasTag(tag)) mTags.append(tag); }
    bool hasTag(const QString& tag) const { return mTags.contains(tag, Qt::CaseInsensitive); }

    bool isEmpty() const { return mChildren.empty(); }
    bool isNull() const  { return isEmpty() && mName.isEmpty() && mHtml.isEmpty(); }

    void setName(const QString& n) { mName = n; }
    void addChild(const Item& i)   { mChildren[i.id()] = i; mOrder.append(i.id()); }
    void setHtml(const QString& h) { mHtml = h; }

    void  changeFont(qlonglong skip, const QFont& font);
    void  clearTag(const QString& tag);
    Item* findItem(qlonglong id);

    virtual Json5Object toObject();

    static void resetLastID() { sNextID = 0; }

    static Json5Array  hasArr(Json5Object& obj, const QString& str, const Json5Array& def = {});
    static bool        hasBool(Json5Object& obj, const QString& str, bool def = false);
    static bool        hasBool(Json5Array& obj, const qsizetype idx, bool def = false);
    static qsizetype   hasNum(Json5Object& obj, const QString& str, const qsizetype def = 0);
    static qsizetype   hasNum(Json5Array& arr, const qsizetype idx, const qsizetype def = 0);
    static Json5Object hasObj(Json5Object& obj, const QString& str, const Json5Object& def = {});
    static QString     hasStr(Json5Object& obj, const QString& str, const QString& def = "");
    static QString     hasStr(Json5Array& arr, const qsizetype idx, const QString& def = "");

protected:
    bool fromObject(Json5Object& obj);
    void newHtml();
    static qsizetype nextID() { return sNextID++; }

private:
    Map<qsizetype, Item>    mChildren;
    QString                 mHtml;
    qsizetype               mID { -1 };
    QString                 mName;
    Map<QString, qsizetype> mNamesToID;
    QList<qsizetype>        mOrder;
    StringList              mTags;

    static qsizetype sNextID;
};

class Novel: public Item {
public:
    Novel();
    Novel(Json5Object obj);
    Novel(const QString& filename);

    Json5Object toObject() override;

    void        change()                      { mChanged = true; }
    Json5Object extra()                       { return mExtra; }
    auto        filename()                    { return mFilename; }
    auto        isChanged()                   { return mChanged; }
    void        noChanges()                   { mChanged = false; }
    bool        saveAs(const QString& f)      { mFilename = f; return save(); }
    void        setExtra(Json5Object& obj)    { mExtra = obj; change(); }
    void        setFilename(const QString& f) { mFilename = f; change(); }

    void clear() override;
    void init() override;

    bool open();
    bool save();
    void setHtml(qlonglong node, const QString& html);

private:
    Json5Object mExtra;
    QString     mFilename;

    bool fromObject(Json5Object& obj);
    bool mChanged { false };
};
