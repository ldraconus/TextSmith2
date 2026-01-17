#include "Novel.h"

#include <QFontMetrics>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>

#include "5th.h"

qsizetype Item::sNextID = 0;

Item::Item(bool noId)
    : mCount(0)
    , mID(noId ? sNextID : sNextID++) {
}

Item::Item(Json5Object& obj)
    : mCount(0) {
    fromObject(obj);
}

void Item::changeFont(const QFont& font) {
    mHtml = changeFont(mHtml, font);
}

QString Item::changeFont(const QString& html, const QFont& font) {
    QTextEdit text;
    text.setHtml(html);
    QTextDocument* doc = text.document();
    QTextCursor cursor(doc);

    struct Range {
        int pos;
        int len;
    };
    QVector<Range> ranges;

    QFontMetrics metrics(font);
    int lineHeight = metrics.height();
    int indent = 2 * lineHeight;
    QTextBlockFormat toIndent;
    toIndent.setTextIndent(indent);
    toIndent.setBottomMargin(lineHeight);

    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); it != block.end(); ++it) {
            if (!it.fragment().isValid()) continue;
            const QTextFragment frag = it.fragment();
            ranges.append({ frag.position(), frag.length() });
        }

        cursor.setPosition(block.position());
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.mergeBlockFormat(toIndent);
    }

    for (const auto& range: ranges) {
        cursor.setPosition(range.pos);
        cursor.setPosition(range.pos + range.len, QTextCursor::KeepAnchor);

        QTextCharFormat fmt;
        fmt.setFontFamilies({ font.family() });
        fmt.setFontPointSize(font.pointSize());
        cursor.mergeCharFormat(fmt);
    }

    return text.toHtml();
}

QString Item::setupHtml(const QFont& font) {
    QTextEdit text;
    QFontMetrics metrics(font);
    int lineHeight = metrics.height();
    int indent = 4 * metrics.averageCharWidth();
    QTextBlockFormat format;
    format.setTextIndent(indent);
    format.setBottomMargin(lineHeight);
    QTextCursor cursor(text.document());
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    text.setTextCursor(cursor);
    return text.document()->toHtml();
}

void Item::clear() {
    mCount = 0;
    mHtml.clear();
    mID = 0;
    mName.clear();
    mTags.clear();
    mPosition = 0;
}

void Item::init() {
    QTextEdit text;
    mHtml = text.toHtml();
}

void Item::buildTree(Json5Object& obj, TreeNode& current) {
    qlonglong id = Item::hasNum(obj, Novel::V1Id, -1);
    if (id != -1) current.setId(id);
    Json5Array arr = Item::hasArr(obj, Novel::Children, {});
    for (auto& node: arr) {
        if (!node.isObject()) continue;
        TreeNode branch;
        buildTree(node.toObject(), branch);
        current.addBranch(branch);
    }
}

void Item::clearTag(const QString &tag) {
    auto idx = mTags.indexOf(tag);
    if (idx >= 0) mTags.takeAt(idx);
}

qlonglong Item::count() {
    QTextEdit text;
    text.insertHtml(mHtml);
    QString plain = text.toPlainText();
    StringList words(plain.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts));
    return mCount = words.count();
}

void Item::fromV1Object(Json5Object& obj, Item& node, TreeNode& tree) {
    node.setName(Item::hasStr(obj, Novel::V1Name, ""));
    node.setHtml(Item::hasStr(obj, Novel::Doc,    ""));
    Json5Array arr = Item::hasArr(obj, Novel::V1Tags, {});
    for (auto& tag: arr) {
        if (!tag.isString()) continue;
        node.addTag(tag.toString());
    }
    node.setId(Item::getNextID());
    tree.setId(node.id());
    auto children = Item::hasArr(obj, Novel::Children, {});
    for (auto& child: children) {
        if (!child.isObject()) continue;
        Json5Object& item = child.toObject();
        TreeNode branch;
        Item next(NoID);
        next.fromV1Object(item, next, branch);
        if (!branch.isEmpty()) tree.addBranch(branch);
    }
    Novel::ref().addItem(node);
}

bool Item::hasTag(const StringList& tags) const {
    for (const auto& tag: tags) {
        if (hasTag(tag)) return true;
    }
    return false;
}

Json5Object Item::toObject() {
    Json5Object obj;
    obj[Novel::Html] =     mHtml;
    obj[Novel::Position] = mPosition;
    obj[Novel::Id] =       mID;
    obj[Novel::Name] =     mName;
    Json5Array tags;
    for (auto i = 0; i < mTags.count(); ++i) tags.append(mTags[i]);
    obj[Novel::Tags] =     tags;
    return obj;
}

Json5Array Item::hasArr(Json5Object& obj, const QString& str, const Json5Array& def) {
    if (valid(obj, str, Json5::Array)) return obj[str].toArray();
    return def;
}

bool Item::hasBool(Json5Object& obj, const QString& str, bool def) {
    if (valid(obj, str, Json5::Boolean)) return obj[str].toBoolean();
    return def;
}

bool Item::hasBool(Json5Array& arr, const qsizetype idx, bool def) {
    if (idx >= 0 && idx < arr.count() && arr[idx].isBoolean()) return arr[idx].toBoolean();
    return def;
}

qsizetype Item::hasNum(Json5Object& obj, const QString& str, const qsizetype def) {
    if (valid(obj, str, Json5::Number)) return obj[str].toInt(def);
    return def;
}

qsizetype Item::hasNum(Json5Array& arr, const qsizetype idx, const qsizetype def) {
    if (arr.count() > idx && idx >= 0 && arr[idx].isNumber()) return arr[idx].toInt(def);
    return def;

}

Json5Object Item::hasObj(Json5Object& obj, const QString& str, const Json5Object& def) {
    if (valid(obj, str, Json5::Object)) return obj[str].toObject();
    return def;
}

QString Item::hasStr(Json5Object& obj, const QString& str, const QString& def) {
    if (valid(obj, str, Json5::String)) return obj[str].toString(def);
    else return def;
}

QString Item::hasStr(Json5Array& arr, const qsizetype idx, const QString& def) {
    if (arr.count() > idx && idx >= 0 && arr[idx].isString()) return arr[idx].toString(def);
    return def;
}

void Item::copy(const Item& i) {
    mCount = i.mCount;
    mHtml = i.mHtml;
    mID = i.mID;
    mName = i.mName;
    mPosition = i.mPosition;
    mTags = i.mTags;
}

bool Item::fromObject(Json5Object& obj) {
    mHtml =     hasStr(obj, Novel::Html);
    mPosition = hasNum(obj, Novel::Position);
    auto id =   hasNum(obj, Novel::Id);
    mName =     hasStr(obj, Novel::Name);

    setId(id);
    if (mName.isEmpty()) mName = name();

    Json5Array tags = hasArr(obj, Novel::Tags, {});
    for (qsizetype i = 0; i < tags.count(); ++i) {
        QString tag = hasStr(tags, i, "");
        if (tag.isEmpty()) continue;
        mTags.append(tag);
    }
    Novel::ref().addItem(*this);
    return true;
}

void Item::move(Item&& i) {
    copy(i);
    i.mCount = 0;
    i.mHtml.clear();
    i.mID = 0;
    i.mName.clear();
    i.mPosition = 0;;
    i.mTags.clear();
}

void Item::newHtml(const QFont& font) {
    QTextEdit text(nullptr);
    text.setText("");
    setupHtml(font);

    mHtml = text.toHtml();
    mCount = 0;
}

QString Item::toPlainText() {
    QTextEdit converter;
    converter.setHtml(mHtml);
    QString text = converter.toPlainText();
    return text;
}

Novel* Novel::sNovel = nullptr;

Novel::Novel():
    mFilename("") {
    sNovel = this;
    init();
}

Novel::Novel(Json5Object obj) {
    sNovel = this;
    Novel::fromObject(obj);
}

Novel::Novel(const QString& filename) {
    sNovel = this;
    Json5Document doc;
    bool readOk = doc.read(filename);
    if (readOk && doc.top().isObject()) Novel(doc.top().toObject());
}

Json5Object Novel::toObject() {
    Json5Object obj;
    obj[Filename] = mFilename;
    obj[Extra] =    mExtra;
    obj[Root] =     mRoot;
    Json5Object items;
    for (auto& node: mItems) items[QString::number(node.first)] = node.second.toObject();
    obj[Items] =    items;
    obj[Branches] = mBranches.toObject();
    return obj;
}

void Novel::changeFont(const QFont& font) {
    for (auto& child: mItems) child.second.changeFont(font);
}

qlonglong Novel::countAll() {
    qlonglong total = 0;
    for (auto& node: mItems) {
        Item& item = node.second;
        total += item.count();
    }
    return total;
}

void Novel::deleteItem(qlonglong id) {
    if (id == mRoot) clear();
    else mItems.erase(id);
}

Item& Novel::findItem(qlonglong id) {
    static Item nil;
    if (mItems.keys().contains(id)) {
        return mItems[id];
    }
    return nil;
}

void Novel::init() {
    mExtra.clear();
    mFilename.clear();
    Item::resetLastID();
    Item item;
    item.init();
    mItems.clear();
    mRoot = item.id();
    auto id = item.id();
    mItems[id] = item;
    mBranches.clear();
}

bool Novel::open() {
    Json5Document doc;
    noChanges();
    if (bool success = doc.read(mFilename) && doc.top().isObject(); success) {
        auto& obj = doc.top().toObject();
        if (obj.contains(Document)) fromV1Object(obj);
        else fromObject(obj);
    } else return false;
    return true;
}

bool Novel::save() {
    if (mFilename.isEmpty()) return false;
    auto obj = toObject();
    Json5Document doc;
    doc.setTop(obj);
    if (bool success = doc.write(mFilename); success) noChanges();
    else return false;
    return true;
}

void Novel::setHtml(qlonglong node, const QString& html) {
    Item& item = findItem(node);
    if (item.isNull()) return;
    item.setHtml(html);
}

void Novel::setupScripting(fifth::vm* vm) {
    vm->addBuiltin("html", [this](fifth::vm* vm) { // id -u-> html
        auto& user = vm->user();
        auto i = user.pop();
        auto id = i.asNumber();

        if (id < 0) user.push("");
        else {
            Item& item = findItem(id);
            if (item.isNull()) user.push("");
            else user.push(item.html());
        }
    });
    vm->addBuiltin("<html", [this](fifth::vm* vm) {   // id html -u-> id
        auto& user = vm->user();
        auto h = user.pop();
        auto i = user.top();
        auto id = i.asNumber();
        auto html = h.asString().str();

        if (id < 0 || html.isEmpty()) return;
        else {
            Item& item = findItem(id);
            if (item.isNull()) user.push(id);
            else item.setHtml(html);
        }
    });
    vm->addBuiltin("html>", [this](fifth::vm* vm) {   // html id -u-> html
        auto& user = vm->user();
        auto i = user.pop();
        auto h = user.top();
        auto id = i.asNumber();
        auto html = h.asString().str();

        if (id < 0 || html.isEmpty()) return;
        else {
            Item& item = findItem(id);
            if (item.isNull()) return;
            else item.setHtml(html);
        }
    });
    vm->addBuiltin("name", [this](fifth::vm* vm) { // id -u-> name
        auto& user = vm->user();
        auto i = user.pop();
        auto id = i.asNumber();

        if (id < 0) user.push("");
        else {
            Item& item = findItem(id);
            if (item.isNull()) user.push("");
            else user.push(item.name());
        }
    });
    vm->addBuiltin("<name", [this](fifth::vm* vm) {   // id name -u-> id
        auto& user = vm->user();
        auto n = user.pop();
        auto i = user.top();
        auto id = i.asNumber();
        auto name = n.asString().str();

        if (id < 0 || name.isEmpty()) return;
        else {
            Item& item = findItem(id);
            if (item.isNull()) user.push(id);
            else item.setName(name);
        }
    });
    vm->addBuiltin("name>", [this](fifth::vm* vm) {   // name id -u-> name
        auto& user = vm->user();
        auto i = user.pop();
        auto n = user.top();
        auto id = i.asNumber();
        auto name = n.asString().str();

        if (id < 0 || name.isEmpty()) return;
        else {
            Item& item = findItem(id);
            if (item.isNull()) return;
            else item.setName(name);
        }
    });
    vm->addBuiltin("tags", [this](fifth::vm* vm) {   // id -u-> tag(n) ... tag(1) n
        auto& user = vm->user();
        auto i = user.pop();
        auto id = i.asNumber();

        if (id < 0) user.push(0);
        else {
            Item& item = findItem(id);
            if (item.isNull()) user.push(0);
            else {
                StringList tags = item.tags();
                for (const auto& tag: tags) user.push(tag);
                user.push(tags.count());
            }
        }
    });
    vm->addBuiltin("+tag", [this](fifth::vm* vm) { // id tag -u-> id
        auto& user = vm->user();                   // tag id -u-> id
        auto t = user.pop();
        auto i = user.pop();
        if (i.isStr() && t.isNum()) swap<fifth::value>(t, i);
        if (!i.isNum()) user.push(i);
        else {
            auto id = i.asNumber();
            auto tag = t.asString().str();
            Item& item = findItem(id);
            if (!item.isNull()) item.addTag(tag);
            user.push(id);
        }
    });
    vm->addBuiltin("-tag", [this](fifth::vm* vm) { // id tag -u-> id
        auto& user = vm->user();                   // tag id -u-> id
        auto t = user.pop();
        auto i = user.pop();
        if (i.isStr() && t.isNum()) swap<fifth::value>(t, i);
        if (i.isNum()) {
            auto id = i.asNumber();
            auto tag = t.asString().str();
            Item& item = findItem(id);
            item.removeTag(tag);
            user.push(id);
        } else user.push(i);
    });
    vm->addBuiltin("?tag", [this](fifth::vm* vm) { // id tag -u-> t/f
        auto& user = vm->user();                   // tag id -u-> t/f
        auto t = user.pop();
        auto i = user.pop();
        if (i.isStr() && t.isNum()) swap<fifth::value>(t, i);
        if (!i.isNum()) user.push(false);
        else {
            auto id = i.asNumber();
            auto tag = t.asString().str();
            Item& item = findItem(id);
            user.push(item.hasTag(tag));
        }
    });
    vm->addBuiltin("ids", [this](fifth::vm* vm) { // -u-> id(n) ... id(1) n;)
        auto& user = vm->user();
        auto keys = mItems.keys();
        for (auto i = keys.size(); i != 0; --i) user.push(keys[i - 1]);
        user.push(keys.size());
    });
    vm->addBuiltin("position", [this](fifth::vm* vm) { // id -u-> position
        auto& user = vm->user();
        auto i = user.pop();
        auto id = i.asNumber();

        if (id < 0) user.push(0);
        else {
            Item& item = findItem(id);
            if (item.isNull()) user.push(0);
            else user.push(item.position());
        }
        vm->addBuiltin("<position", [this](fifth::vm* vm) {   // id position -u-> id
            auto& user = vm->user();
            auto p = user.pop();
            auto i = user.top();
            auto id = i.asNumber();
            auto pos = p.asNumber();

            if (id < 0 || pos < 0) return;
            else {
                Item& item = findItem(id);
                if (item.isNull()) user.push(id);
                else item.setPosition(pos);
            }
        });
        vm->addBuiltin("position>", [this](fifth::vm* vm) {   // position id -u-> position
            auto& user = vm->user();
            auto i = user.pop();
            auto p = user.top();
            auto id = i.asNumber();
            auto pos = p.asNumber();

            if (id < 0 || pos < 0) return;
            else {
                Item& item = findItem(id);
                if (item.isNull()) return;
                else item.setPosition(pos);
            }
        });
    });
}

bool Novel::fromObject(Json5Object& obj) {
    mFilename = Item::hasStr(obj, Filename, "");
    mExtra =    Item::hasObj(obj, Extra, {});
    mItems.clear();
    mRoot =     Item::hasNum(obj, Root, 0);
    Json5Object items = Item::hasObj(obj, Items, {});
    for (auto& node: items) {
        if (!node.second.isObject()) continue;
        auto& branch = node.second.toObject();
        if (node.first.toLongLong() != branch[Id].toInt()) continue;
        Item item(branch);
        addItem(item);
    }
    if (Json5Object treeNode = Item::hasObj(obj, Branches, {}); treeNode.empty()) return false;
    else {
        TreeNode tree(treeNode);
        mBranches = tree;
    }
    countAll();
    return true;
}

bool Novel::fromV1Object(Json5Object& obj) {
    if (obj.size() != 3) return false;
    Json5Object document = Item::hasObj(obj, Document, {});
    Json5Object options =  Item::hasObj(obj, Options,  {});
    Json5Object windows =  Item::hasObj(obj, Windows,  {});
    mExtra[V1] = true;
    mExtra[Options] = options;
    mExtra[Windows] = windows;
    Json5Object root = Item::hasObj(document, V1Root, {});
    Item::resetLastID();
    Item rootItem(Item::NoID);
    TreeNode tree;
    rootItem.fromV1Object(root, rootItem, tree);
    mBranches = tree;
    countAll();
    return true;
}

TreeNode::TreeNode(Json5Object& obj) {
    mId                 = Item::hasNum(obj, Novel::BranchId, 0);
    Json5Array branches = Item::hasArr(obj, Novel::Branches, {});
    for (auto& node: branches) {
        if (!node.isObject()) continue;
        TreeNode branch(node.toObject());
        mBranches.append(branch);
    }
}

TreeNode& TreeNode::find(TreeNode& branch, qlonglong i) {
    static TreeNode nil(-1);

    if (branch.id() == i) return *this;
    for (auto& twig: branch.branches()) {
        auto& res = find(twig, i);
        if (!res.empty()) return res;
    }
    return nil;
}

Json5Object TreeNode::toObject() {
    Json5Object obj;
    obj[Novel::BranchId] = mId;
    Json5Array arr;
    for (auto& branch: mBranches) arr.append(branch.toObject());
    obj[Novel::Branches] = arr;
    return obj;
}
