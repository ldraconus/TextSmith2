#include "Novel.h"

qsizetype Item::sNextID;

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
    mID = -1;
    mName.clear();
    mOrder.clear();
    mTags.clear();
}

void Item::clearTag(const QString &tag) {
    auto idx = mTags.indexOf(tag);
    if (idx >= 0) mTags.takeAt(idx);
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
    if (mName.isEmpty()) mName = "<unnamed_#" + QString::number(mID) + ">";

    Json5Object children = hasObj(obj, "Children", {});
    for (auto& child: children) {
        bool ok;
        const auto& idx = child.first.toLongLong(&ok);
        if (ok && idx >= 0 && child.second.isObject()) {
            auto item = Item(child.second.toObject());
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

bool Novel::fromObject(Json5Object& obj) {
    Item::fromObject(obj);
    mFilename = hasStr(obj, "Filename", "");
    mExtra = hasObj(obj, "Extra", {});
    return true;
}
