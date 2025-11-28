#include "Novel.h"
#include "Main.h"

#include <QFontMetrics>
#include <QTextCursor>
#include <QTextEdit>

qsizetype Item::sNextID = 0;

Item::Item()
    : mID(sNextID++) {
}

Item::Item(Json5Object& obj) {
    fromObject(obj);
}

void Item::clear() {
    for (auto& child: mChildren) child.second.clear();
    mChildren.clear();
    mHtml.clear();
    mID = 0;
    mName.clear();
    mOrder.clear();
    mTags.clear();
    sNextID = 0;
}

void Item::init() {
    newHtml();
}

void Item::changeFont(qlonglong skip, const QFont& font) {
    if (mID != skip) {
        QTextEdit text;
        text.setHtml(mHtml);
        Main::changeDocumentFont(text.document(), font);
        mHtml = text.toHtml();
    }
    for (auto& child: mChildren) child.second.changeFont(skip, font);
}

void Item::clearTag(const QString &tag) {
    auto idx = mTags.indexOf(tag);
    if (idx >= 0) mTags.takeAt(idx);
}

Item* Item::findItem(qlonglong id) {
    if (id == mID) return this;
    for (auto& child: mChildren) {
        if (auto* found = child.second.findItem(id); found != nullptr) return found;
    }
    return nullptr;
}

Json5Object Item::toObject() {
    Json5Object obj;
    obj["HTML"] = mHtml;
    obj["ID"] = mID;
    obj["Name"] = mName;
    Json5Object children;
    for (auto& child: children) children[child.first] = child.second.toObject();
    obj["Children"] = children;
    Json5Array order;
    for (auto i = 0; i < mOrder.count(); ++i) order.append(mOrder[i]);
    obj["Order"] = order;
    Json5Array tags;
    for (auto i = 0; i < mTags.count(); ++i) tags.append(mTags[i]);
    obj["Tags"] = tags;
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
    if (valid(obj, str, Json5::String)) return obj[str].toInt(def);
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

bool Item::fromObject(Json5Object& obj) {
    mHtml = hasStr(obj, "HTML");
    mID = hasNum(obj, "ID");
    mName = hasStr(obj, "Name");

    if (mID >= sNextID) sNextID = mID + 1;
    if (mName.isEmpty()) mName = name();

    Json5Object children = hasObj(obj, "Children", {});
    for (auto& child: children) {
        bool ok;
        const auto& idx = child.first.toLongLong(&ok);
        if (ok && idx >= 0 && child.second.isObject()) {
            Item item(child.second.toObject());
            if (item.id() != idx) continue;
            QString name = item.name();
            if (item.name().isEmpty()) continue;
            mChildren[idx] = item;
            mNamesToID[name] = idx;
            if (idx >= sNextID) sNextID = idx + 1;
            mOrder.append(idx);
        }
    }

    Json5Array order = hasArr(obj, "Order", {});
    for (qsizetype i = 0; i < order.count(); ++i) {
        if (i > mOrder.count()) continue;
        qsizetype idx = hasNum(order, i, -1);
        qsizetype old = mOrder[i];
        qsizetype at = mOrder.indexOf(idx);
        mOrder[i] = idx;
        mOrder[at] = old;
    }

    Json5Array tags = hasArr(obj, "Tags", {});
    for (qsizetype i = 0; i < tags.count(); ++i) {
        QString tag = hasStr(tags, i, "");
        if (tag.isEmpty()) continue;
        mTags.append(tag);
    }
    return true;
}

void Item::newHtml() {
    QTextEdit text;
    text.setText("");
    Preferences& prefs = Main::ref().prefs();
    QFont font(prefs.fontFamily(), prefs.fontSize());
    QFontMetrics metrics(font);
    int lineHeight = metrics.height();
    int indent = 4 * lineHeight;
    QTextBlockFormat format;
    format.setTextIndent(indent);
    format.setBottomMargin(lineHeight);

    QTextCursor cursor(text.document());
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    text.setTextCursor(cursor);
    text.document()->setDefaultFont(font);
    mHtml = text.toHtml();
}

Novel::Novel()
    : Item()
    , mFilename("") {
}

Novel::Novel(Json5Object obj)
    : Item(obj) {
    Novel::fromObject(obj);
}

Novel::Novel(const QString& filename) {
    Json5Document doc;
    bool readOk = doc.read(filename);
    if (readOk && doc.top().isObject()) Novel(doc.top().toObject());
}

Json5Object Novel::toObject() {
    auto obj = Item::toObject();
    obj["Filename"] = mFilename;
    obj["Extra"] = mExtra;
    return obj;
}

void Novel::clear() {
    Item::clear();
    mExtra.clear();
    mFilename.clear();
}

void Novel::init() {
    Item::init();
}

bool Novel::open() {
    Json5Document doc;
    noChanges();
    if (bool success = doc.read(mFilename) && doc.top().isObject(); success) fromObject(doc.top().toObject());
    else return false;
    return true;
}

bool Novel::save() {
    auto obj = toObject();
    Json5Document doc;
    doc.setTop(obj);
    if (bool success = doc.write(mFilename); success) noChanges();
    else return false;
    return true;
}

void Novel::setHtml(qlonglong node, const QString& html) {
    Item* item = findItem(node);
    item->setHtml(html);
}

bool Novel::fromObject(Json5Object& obj) {
    Item::fromObject(obj);
    mFilename = hasStr(obj, "Filename", "");
    mExtra = hasObj(obj, "Extra", {});
    return true;
}
