#pragma once

#include <QString>
#include <QTextBlockFormat>
#include <QTextDocument>

#include "5th.h"
#include "List.h"
#include "Map.h"
#include "StringList.h"

#include <Json5.h>

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

    auto*      doc() const                      { return mDoc; }
    auto       html() const                     { return mDoc->toHtml(); }
    auto       id() const                       { return mID; }
    auto       name() const                     { return mName.isEmpty() ? "<unnamed_#" + QString::number(mID) + ">" : mName; }
    auto       position() const                 { return mPosition; }
    void       setId(qlonglong i)               { mID = i++; if (i > sNextID) sNextID = i; }
    void       addTag(const QString& tag)       { if (!hasTag(tag)) { mTags.append(tag); } }
    void       removeTag(const QString& tag)    { if (hasTag(tag)) { mTags.removeAll(tag, Qt::CaseInsensitive); } }
    bool       hasTag(const QString& tag) const { return mTags.contains(tag, Qt::CaseInsensitive); }
    void       setTags(const StringList& tags)  { mTags = tags; }
    StringList tags() const                     { return mTags; }

    bool isNull() const  { return mName.isEmpty() && mDoc == nullptr; }

    void setDoc(QTextDocument* d)  { mDoc = d; }
    void setHtml(const QString& h) { mDoc->setHtml(h); count(); }
    void setName(const QString& n) { mName = n; }
    void setPosition(qlonglong p)  { mPosition = p; }

    void             buildTree(Json5Object& obj, TreeNode& current);
    void             changeFont(const QFont& font);
    void             clearTag(const QString& tag);
    qlonglong        count();
    bool             fromDocArray(Json5Array& arr);
    bool             fromDocObject(Json5Object& obj);
    bool             fromObject(Json5Object& obj);
    QTextBlockFormat fromTextBlockFormatObject(Json5Object& obj);
    QTextCharFormat  fromTextCharFormatObject(Json5Object& obj);
    void             fromV1Object(Json5Object& obj, Item& node, TreeNode& tree);
    bool             hasTag(const StringList& tags) const;
    void             loadInternalImages();
    void             setDocumentFont(const QFont& font);
    QString          toPlainText();
    void             toTextBlockFormat(Json5Object& obj, QTextBlockFormat& formt);
    void             toTextCharFormat(Json5Object& obj, QTextCharFormat& format);

    virtual Json5Object toObject();

    static QString changeFont(QTextDocument* doc, const QFont& font);
    static QString setupDocument(const QFont& font, QTextDocument* doc = nullptr);

    static auto getNextID()                  { return sNextID; }
    static void resetLastID(qlonglong i = 0) { sNextID = i; }

    static Json5Array  hasArr(Json5Object& obj,  const QString&    str,  const Json5Array&  def = { });
    static Json5Array  hasArr(Json5Object& obj,  const StringList& strs, const Json5Array&  def = { });
    static bool        hasBool(Json5Object& obj, const QString&    str,  bool               def = false);
    static bool        hasBool(Json5Object& obj, const StringList& strs, bool               def = { });
    static bool        hasBool(Json5Array& obj,  const qsizetype   idx,  bool               def = false);
    static qsizetype   hasNum(Json5Object& obj,  const QString&    str,  const qsizetype    def = 0);
    static qsizetype   hasNum(Json5Object& obj,  const StringList& strs, const qsizetype    def = 0);
    static double      hasNum(Json5Object& obj,  const QString&    str,  const double       def = 0.0);
    static double      hasNum(Json5Object& obj,  const StringList& strs, const double       def = 0.0);
    static qsizetype   hasNum(Json5Array& arr,   const qsizetype   idx,  const qsizetype    def = 0);
    static double      hasNum(Json5Array& arr,   const qsizetype   idx,  const double       def = 0.0);
    static Json5Object hasObj(Json5Object& obj,  const QString&    str,  const Json5Object& def = { });
    static QString     hasStr(Json5Object& obj,  const QString&    str,  const QString&     def = "");
    static QString     hasStr(Json5Array& arr,   const qsizetype   idx,  const QString&     def = "");
    static QString     hasStr(Json5Object& obj,  const StringList& strs, const QString&     def = "");

    static Map<QString, QImage> sImages;

protected:
    void copy(const Item& i);
    void move(Item&& i);

    static qsizetype nextID() { return sNextID++; }

private:
    qlonglong               mCount;
    QTextDocument*          mDoc      { nullptr };
    qsizetype               mID       { -1 };
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

    static constexpr auto Alignment  = "Alignment";
    static constexpr auto BranchId   = "Id";
    static constexpr auto Branches   = "Branches";
    static constexpr auto Bold       = "Bold";
    static constexpr auto Children   = "children";
    static constexpr auto Current    = "Current";
    static constexpr auto Data       = "Data";
    static constexpr auto Dictionary = "Dictionary";
    static constexpr auto Doc        = "doc";
    static constexpr auto Document   = "document";
    static constexpr auto Extra      = "Extra";
    static constexpr auto Filename   = "Filename";
    static constexpr auto Fragments  = "Fragments";
    static constexpr auto Height     = "Height";
    static constexpr auto Html       = "HTML";
    static constexpr auto Id         = "ID";
    static constexpr auto Ids        = "Ids";
    static constexpr auto Image      = "Image";
    static constexpr auto Images     = "Images";
    static constexpr auto Indent     = "Indent";
    static constexpr auto Italic     = "Italic";
    static constexpr auto Items      = "Items";
    static constexpr auto NakedDoc   = "Document";
    static constexpr auto Name       = "Name";
    static constexpr auto Options    = "options";
    static constexpr auto Position   = "Position";
    static constexpr auto Prefs      = "Prefs";
    static constexpr auto Root       = "Root";
    static constexpr auto State      = "State";
    static constexpr auto Tags       = "Tags";
    static constexpr auto Text       = "Text";
    static constexpr auto Underline  = "Underline";
    static constexpr auto Url        = "Url";
    static constexpr auto V1         = "v1";
    static constexpr auto V1Data     = "data";
    static constexpr auto V1Id       = "id";
    static constexpr auto V1Name     = "name";
    static constexpr auto V1Root     = "root";
    static constexpr auto V1Tags     = "tags";
    static constexpr auto V1Url      = "url";
    static constexpr auto Width      = "Width";
    static constexpr auto Windows    = "windows";

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

    void        changeFont(const QFont& font);
    qlonglong   count(qlonglong id);
    qlonglong   countAll();
    void        deleteItem(qlonglong id);
    Item&       fifthItem(fifth::stack& user);
    Item&       findItem(qlonglong id);
    void        init();
    bool        open();
    bool        save(bool compress = Json5Object::HumanReadable);
    void        setHtml(qlonglong node, const QString& html);
    void        setupScripting(fifth::vm* vm);
    Json5Object toObject();

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
