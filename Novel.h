#pragma once

#include <QString>

#include <Json5.h>
#include <List.h>
#include <Map.h>
#include <StringList.h>

class TreeNode {
public:
    TreeNode() { }
    TreeNode(qlonglong id) { mId = id; }
    TreeNode(Json5Object& obj);

    void            addBranch(TreeNode node) { mBranches.append(node); }
    List<TreeNode>& branches()               { return mBranches; }
    void            clear()                  { mBranches.clear(); }
    bool            empty() const            { return mId == -1; }
    qlonglong       id() const               { return mId; }
    bool            isEmpty() const          { return empty(); }
    void            setId(qlonglong i)       { mId = i; }
    qlonglong       size() const             { return mBranches.count(); }

    TreeNode&   find(TreeNode& branch, qlonglong i);
    void        fromV1Object(Json5Object& o);
    Json5Object toObject();

private:
    qlonglong      mId { 0 };
    List<TreeNode> mBranches;
};

class Item {
public:
    static constexpr bool NoID =                true;
    static constexpr bool GiveID =              !NoID;

    Item(const Item& i) { copy(i); }
    Item(Item&& i)      { move(std::move(i)); }

    Item(bool noId = GiveID);
    Item(Json5Object& obj);

    Item& operator=(const Item& i) { if (this != &i) copy(i); return *this; }
    Item& operator=(Item&& i)      { move(std::move(i)); return *this; }

    virtual void clear();
    virtual void init();

    auto html()             { return mHtml; }
    auto id() const         { return mID; }
    auto name() const       { return mName.isEmpty() ? "<unnamed_#" + QString::number(mID) + ">" : mName; }
    auto position() const   { return mPosition; }
    void setId(qlonglong i) { mID = i++; if (i > sNextID) sNextID = i; }

    void addTag(const QString& tag)       {
        if (!hasTag(tag)) {
            mTags.append(tag);
        }
    }
    bool hasTag(const QString& tag) const {
        bool temp = mTags.contains(tag, Qt::CaseInsensitive);
        return temp;
    }
    void setTags(const StringList& tags)  {
        mTags = tags;
    }
    StringList tags() const                     { return mTags; }

    bool isNull() const  { return mName.isEmpty() && mHtml.isEmpty(); }

    void setHtml(const QString& h) { mHtml = h; count(); }
    void setName(const QString& n) { mName = n; }
    void setPosition(qlonglong p)  { mPosition = p; }

    void      buildTree(Json5Object& obj, TreeNode& current);
    void      changeFont(const QFont& font);
    void      clearTag(const QString& tag);
    qlonglong count();
    bool      fromObject(Json5Object& obj);
    void      fromV1Object(Json5Object& obj, Item& node, TreeNode& tree);
    bool      hasTag(const StringList& tags) const;
    void      newHtml();
    QString   toPlainText();

    virtual Json5Object toObject();

    static auto getNextID()                  { return sNextID; }
    static void resetLastID(qlonglong i = 0) { sNextID = i; }

    static Json5Array  hasArr(Json5Object& obj,  const QString&  str, const Json5Array&  def = { });
    static bool        hasBool(Json5Object& obj, const QString&  str, bool               def = false);
    static bool        hasBool(Json5Array& obj,  const qsizetype idx, bool               def = false);
    static qsizetype   hasNum(Json5Object& obj,  const QString&  str, const qsizetype    def = 0);
    static qsizetype   hasNum(Json5Array& arr,   const qsizetype idx, const qsizetype    def = 0);
    static Json5Object hasObj(Json5Object& obj,  const QString&  str, const Json5Object& def = { });
    static QString     hasStr(Json5Object& obj,  const QString&  str, const QString&     def = "");
    static QString     hasStr(Json5Array& arr,   const qsizetype idx, const QString&     def = "");

protected:
    void copy(const Item& i);
    void move(Item&& i);

    static qsizetype nextID() { return sNextID++; }

private:
    qlonglong               mCount;
    QString                 mHtml;
    qsizetype               mID { -1 };
    QString                 mName;
    qlonglong               mPosition { 0 };
    StringList              mTags;

    static qsizetype sNextID;
};

class Novel {
public:
    Novel();
    Novel(Json5Object obj);
    Novel(const QString& filename);

    static constexpr auto BranchId = "Id";
    static constexpr auto Branches = "Branches";
    static constexpr auto Children = "children";
    static constexpr auto Doc      = "doc";
    static constexpr auto Document = "document";
    static constexpr auto Extra    = "Extra";
    static constexpr auto Filename = "Filename";
    static constexpr auto Html     = "HTML";
    static constexpr auto Id       = "ID";
    static constexpr auto Items    = "Items";
    static constexpr auto Name     = "Name";
    static constexpr auto Options  = "options";
    static constexpr auto Position = "Position";
    static constexpr auto Root     = "Root";
    static constexpr auto Tags     = "Tags";
    static constexpr auto V1       = "v1";
    static constexpr auto V1Id     = "id";
    static constexpr auto V1Root   = "root";
    static constexpr auto V1Tags   = "tags";
    static constexpr auto V1Name   = "name";
    static constexpr auto Windows  = "windows";

    Json5Object toObject();
    void        addItem(const Item& i)         { auto s = Item::getNextID(); mItems[i.id()] = i; Item::resetLastID(s); }
    TreeNode    branches()                     { return mBranches; }
    void        change()                       { mChanged = true; }
    void        clear()                        { init(); }
    Json5Object extra()                        { return mExtra; }
    auto        filename()                     { return mFilename; }
    auto        isChanged()                    { return mChanged; }
    void        noChanges()                    { mChanged = false; }
    qlonglong   root()                         { return mRoot; }
    bool        saveAs(const QString& f)       { mFilename = f; return save(); }
    void        setBranches(const TreeNode& b) { mBranches = b; }
    void        setExtra(Json5Object& obj)     { mExtra = obj; change(); }
    void        setFilename(const QString& f)  { mFilename = f; change(); }

    void      changeFont(const QFont& font);
    qlonglong countAll();
    void      deleteItem(qlonglong id);
    Item&     findItem(qlonglong id);
    void      init();
    bool      open();
    bool      save();
    void      setHtml(qlonglong node, const QString& html);

    static Novel* ptr() { return sNovel; }
    static Novel& ref() { return *ptr(); }

private:
    TreeNode             mBranches;
    bool                 mChanged { false };
    Json5Object          mExtra;
    QString              mFilename;
    Map<qlonglong, Item> mItems;
    qlonglong            mRoot { 0 };

    bool fromObject(Json5Object& obj);
    bool fromV1Object(Json5Object& obj);

    static Novel* sNovel;
};
