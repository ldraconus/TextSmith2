#include "Novel.h"

#include <QFontMetrics>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>

#include "Main.h"

qsizetype            Item::sNextID = 0;
Map<QString, QImage> Item::sImages;

Item::Item(bool noId)
    : mCount(0)
    , mID(noId ? sNextID : sNextID++)
    , mDoc(new QTextDocument()) {
}

Item::Item(Json5Object& obj)
    : mCount(0) {
    if (obj.contains(Novel::NakedDoc)) fromDocObject(obj);
    else fromObject(obj);
}

void Item::changeFont(const QFont& font) {
    mDoc->setHtml(changeFont(mDoc, font));
}

QString Item::changeFont(QTextDocument* doc, const QFont& font) {
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

    return doc->toHtml();
}

QString Item::setupDocument(const QFont& font, QTextDocument* doc) {
    TextEdit text;
    if (doc == nullptr) doc = text.document();
    QFontMetrics metrics(font);
    int lineHeight = metrics.height();
    int indent = 4 * metrics.averageCharWidth();
    QTextBlockFormat format;
    format.setTextIndent(indent);
    format.setBottomMargin(lineHeight);
    QTextCursor cursor(doc);
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    QString html = doc->toHtml();
    return html;
}

void Item::clear() {
    mCount = 0;
    mDoc = nullptr;
    mID = 0;
    mName.clear();
    mTags.clear();
    mPosition = 0;
}

void Item::init() {
    mDoc->setHtml("");
}

void Item::buildTree(Json5Object& obj, TreeNode& current) {
    qlonglong id = Item::hasNum(obj, Novel::V1Id, qlonglong(-1));
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
    QString plain = mDoc->toPlainText();
    StringList words(plain.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts));
    return mCount = words.count();
}

bool Item::fromDocArray(Json5Array &arr){
    QTextCursor cursor(mDoc);
    bool first = true;
    for (auto& blk: arr) {
        if (blk.isObject()) {
            Json5Object obj = blk.toObject();
            QTextBlockFormat format = fromTextBlockFormatObject(obj);
            QTextCharFormat charFormat = fromTextCharFormatObject(obj);
            cursor.movePosition(QTextCursor::End);
            if (first) {
                first = false;
                cursor.setBlockFormat(format);
                cursor.setBlockCharFormat(charFormat);
            } else cursor.insertBlock(format, charFormat);
            Json5Array fragments = hasArr(obj, Novel::Fragments, { });
            for (auto& fragment: fragments) {
                if (fragment.isObject()) {
                    obj = fragment.toObject();
                    if (obj.contains("Image")) {
                        QString imageId = hasStr(obj, Novel::Image, "");
                        QTextImageFormat imgFmt;
                        imgFmt.setName(imageId);
                        auto height = hasNum(obj, Novel::Height, qlonglong(-1));
                        auto width = hasNum(obj, Novel::Width, qlonglong(-1));
                        if (height != -1) imgFmt.setHeight(height);
                        if (width != -1) imgFmt.setWidth(width);
                        cursor.insertImage(imgFmt);
                        if (imageId.startsWith("internal:")) mDoc->addResource(QTextDocument::ImageResource, imageId, sImages[imageId]);
                    } else {
                        QString text = hasStr(obj, Novel::Text, "");
                        charFormat = fromTextCharFormatObject(obj);
                        cursor.insertText(text, charFormat);
                    }
                }
            }
        }
    }
    return true;
}

bool Item::fromDocObject(Json5Object &obj) {
    delete mDoc;
    mDoc = new QTextDocument();
    mDoc->setParent(nullptr);
    Preferences* prefs = &Main::ref().prefs();
    QFont font(prefs->fontFamily(), prefs->fontSize());
    mDoc->setDefaultFont(font);

    Json5Array doc = hasArr(obj, Novel::NakedDoc);
    if (!fromDocArray(doc)) {
        delete mDoc;
        return false;
    }
    mPosition = hasNum(obj, Novel::Position, 0.0);
    auto id =   hasNum(obj, Novel::Id,       qlonglong(0));
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
    loadInternalImages();
}

bool Item::hasTag(const StringList& tags) const {
    for (const auto& tag: tags) {
        if (hasTag(tag)) return true;
    }
    return false;
}

void Item::loadInternalImages() {
    for (QTextBlock block = mDoc->begin(); block != mDoc->end(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            if (fragment.charFormat().isImageFormat()) {
                QTextImageFormat imgFmt = fragment.charFormat().toImageFormat();
                QString url = imgFmt.name();
                if (url.startsWith("internal:")) mDoc->addResource(QTextDocument::ImageResource, url, sImages[url]);
            }
        }
    }

}

Json5Object Item::toObject() {
    Json5Object obj;

    Json5Array doc;
    for (QTextBlock block = mDoc->begin(); block != mDoc->end(); block = block.next()) {
        Json5Object blk;
        auto format = block.blockFormat();
        toTextBlockFormat(blk, format);
        auto charFormat = block.charFormat();
        toTextCharFormat(blk, charFormat);
        Json5Array fragments;
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            Json5Object frag;

            if (fragment.charFormat().isImageFormat()) {
                QTextImageFormat imgFmt = fragment.charFormat().toImageFormat();
                frag[Novel::Image] = imgFmt.name();
                frag[Novel::Width] = imgFmt.width();
                frag[Novel::Height] = imgFmt.height();
            } else {
                auto fragCharFormat = fragment.charFormat();
                toTextCharFormat(frag, fragCharFormat);
                frag[Novel::Text] = fragment.text();
            }

            fragments.append(frag);
        }
        blk[Novel::Fragments] = fragments;
        doc.append(blk);
    }

    obj[Novel::NakedDoc] = doc;
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

Json5Array Item::hasArr(Json5Object& obj, const StringList& strs, const Json5Array& def) {
    for (const auto& str: strs) {
        if (valid(obj, str, Json5::Array)) return obj[str].toArray();
    }
    return def;
}

bool Item::hasBool(Json5Object& obj, const QString& str, bool def) {
    if (valid(obj, str, Json5::Boolean)) return obj[str].toBoolean();
    return def;
}

bool Item::hasBool(Json5Object& obj, const StringList& strs, bool def) {
    for (const auto& str: strs) {
        if (valid(obj, str, Json5::Boolean)) return obj[str].toBoolean();
    }
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

qsizetype Item::hasNum(Json5Object& obj, const StringList& strs, const qsizetype def) {
    for (const auto& str: strs) {
        if (valid(obj, str, Json5::Number)) return obj[str].toInt(def);
    }
    return def;
}

double Item::hasNum(Json5Object& obj, const QString& str, const double def) {
    if (valid(obj, str, Json5::Number)) return obj[str].toReal(def);
    return def;
}

double Item::hasNum(Json5Object& obj, const StringList& strs, const double def) {
    for (const auto& str: strs) {
        if (valid(obj, str, Json5::Number)) return obj[str].toReal(def);
    }
    return def;
}

qsizetype Item::hasNum(Json5Array& arr, const qsizetype idx, const qsizetype def) {
    if (arr.count() > idx && idx >= 0 && arr[idx].isNumber()) return arr[idx].toInt(def);
    return def;

}

double Item::hasNum(Json5Array& arr, const qsizetype idx, const double def) {
    if (arr.count() > idx && idx >= 0 && arr[idx].isNumber()) return arr[idx].toReal(def);
    return def;
}

Json5Object Item::hasObj(Json5Object& obj, const QString& str, const Json5Object& def) {
    if (valid(obj, str, Json5::Object)) return obj[str].toObject();
    return def;
}

QString Item::hasStr(Json5Object& obj, const QString& str, const QString& def) {
    if (valid(obj, str, Json5::String)) return obj[str].toString(def);
    return def;
}

QString Item::hasStr(Json5Array& arr, const qsizetype idx, const QString& def) {
    if (arr.count() > idx && idx >= 0 && arr[idx].isString()) return arr[idx].toString(def);
    return def;
}

QString Item::hasStr(Json5Object& obj, const StringList& strs, const QString& def) {
    for (const auto& str: strs) {
        if (valid(obj, str, Json5::String)) return obj[str].toString(def);
    }
    return def;
}

void Item::copy(const Item& i) {
    mCount = i.mCount;
    mDoc = i.mDoc;
    mID = i.mID;
    mName = i.mName;
    mPosition = i.mPosition;
    mTags = i.mTags;
}

bool Item::fromObject(Json5Object& obj) {
    QString html = hasStr(obj, Novel::Html);
    mDoc = new QTextDocument();
    mDoc->setParent(nullptr);
    Preferences* prefs = &Main::ref().prefs();
    QFont font({ prefs->fontFamily() }, prefs->fontSize());
    mDoc->setDefaultFont(font);
    mDoc->setHtml(html);

    mPosition =    hasNum(obj, Novel::Position, 0.0);
    auto id =      hasNum(obj, Novel::Id,       qlonglong(0));
    mName =        hasStr(obj, Novel::Name);

    setId(id);
    if (mName.isEmpty()) mName = name();

    Json5Array tags = hasArr(obj, Novel::Tags, {});
    for (qsizetype i = 0; i < tags.count(); ++i) {
        QString tag = hasStr(tags, i, "");
        if (tag.isEmpty()) continue;
        mTags.append(tag);
    }
    Novel::ref().addItem(*this);
    loadInternalImages();
    return true;
}

QTextBlockFormat Item::fromTextBlockFormatObject(Json5Object &obj) {
    QTextBlockFormat format;
    Preferences* prefs = &Main::ref().prefs();
    QFont font(prefs->fontFamily(), prefs->fontSize());
    QFontMetrics metrics(font);
    int lineHeight =  metrics.lineSpacing();
    int paraIndent =  4 * metrics.averageCharWidth();
    auto alignment =  hasNum(obj, Novel::Alignment,  qlonglong(0));
    auto indent =     hasNum(obj, Novel::Indent,     qlonglong(0));
    Qt::Alignment align = Qt::AlignLeft;
    if (alignment == 1) align = Qt::AlignLeft;
    if (alignment == 3) align = Qt::AlignRight;
    if (alignment == 4) align = Qt::AlignJustify;
    if (alignment == 2) align = Qt::AlignHCenter;
    format.setAlignment(align);
    format.setBottomMargin(lineHeight);
    format.setTextIndent(paraIndent);
    format.setIndent(indent);
    return format;
}

QTextCharFormat Item::fromTextCharFormatObject(Json5Object &obj) {
    QTextCharFormat format;
    Preferences* prefs = &Main::ref().prefs();
    format.setFontFamilies({ prefs->fontFamily() });
    format.setFontPointSize(prefs->fontSize());
    bool bold =      hasBool(obj, Novel::Bold,      false);
    bool italic =    hasBool(obj, Novel::Italic,    false);
    bool underline = hasBool(obj, Novel::Underline, false);
    if (bold) format.setFontWeight(QFont::Bold);
    else format.setFontWeight(QFont::Normal);
    format.setFontItalic(italic);
    format.setFontUnderline(underline);
    return format;
}

void Item::move(Item&& i) {
    copy(i);
    i.mCount = 0;
    i.mDoc = nullptr;
    i.mID = 0;
    i.mName.clear();
    i.mPosition = 0;;
    i.mTags.clear();
}

void Item::setDocumentFont(const QFont& font) {
    TextEdit text(nullptr);
    text.setText("");
    setupDocument(font);

    // don't need this?
    // mHtml = text.toHtml();
    mCount = 0;
}

QString Item::toPlainText() {
    QString text = mDoc->toPlainText();
    return text;
}

void Item::toTextBlockFormat(Json5Object& obj, QTextBlockFormat& format) {
    Qt::Alignment align = format.alignment();
    qlonglong alignment = 0;
    if (align & Qt::AlignLeft) alignment = 1;
    if (align & Qt::AlignRight) alignment = 3;
    if (align & Qt::AlignJustify) alignment = 4;
    if (align & Qt::AlignHCenter) alignment = 2;
    obj[Novel::Alignment] = alignment;
    obj[Novel::Indent] = qlonglong(format.indent());
}

void Item::toTextCharFormat(Json5Object& obj, QTextCharFormat& format) {
    if (format.fontWeight() >= QFont::Bold) obj[Novel::Bold] =      true;
    if (format.fontItalic())                obj[Novel::Italic] =    true;
    if (format.fontUnderline())             obj[Novel::Underline] = true;
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

qlonglong Novel::count(qlonglong id) {
    Item& item = findItem(id);
    return item.count();
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
    static bool first = true;
    static Item fake(Item::NoID);
    if (first) {
        first = false;
        fake.setId(-1);
        fake.addTag("");
        TextEdit edit;
        edit.setPlainText("This page intentionally left blank :-)\n\nI mean it!\n");
        fake.setHtml(edit.toHtml());
    }

    if (id == -1) return fake;

    static Item nil;
    if (mItems.keys().contains(id)) return mItems[id];
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

bool Novel::save(bool compress) {
    if (mFilename.isEmpty()) return false;
    auto obj = toObject();
    Json5Document doc;
    doc.setTop(obj);
    if (bool success = doc.write(mFilename, compress); success) noChanges();
    else return false;
    return true;
}

void Novel::setHtml(qlonglong node, const QString& html) {
    Item& item = findItem(node);
    if (item.isNull()) return;
    item.setHtml(html);
}

Item& Novel::fifthItem(fifth::stack& user) {
    static Item nil;
    auto i = user.pop();
    if (i.isNum()) {
        auto id = i.asNumber();
        if (id < 0) return nil;
        return findItem(id);
    }
    return nil;
}

void Novel::setupScripting(fifth::vm* vm) {
    vm->addBuiltin("html", [this](fifth::vm* vm) { // id -u-> html
        auto& user = vm->user();
        Item& item = fifthItem(user);
        if (item.isNull()) user.push("");
        else user.push(item.html());
    });
    vm->addBuiltin("<html", [this](fifth::vm* vm) {   // id html -u-> id
        auto& user = vm->user();
        auto h = user.pop();
        auto i = user.pop();
        Item& item = fifthItem(user);
        auto html = h.asString().str();

        if (!item.isNull() && !html.isEmpty()) item.setHtml(html);
        user.push(i);
    });
    vm->addBuiltin("html>", [this](fifth::vm* vm) {   // html id -u-> html
        auto& user = vm->user();
        Item& item = fifthItem(user);
        auto h = user.top();
        auto html = h.asString().str();

        if (item.isNull() || html.isEmpty()) return;
        else item.setHtml(html);
    });
    vm->addBuiltin("text", [this](fifth::vm* vm) { // id -u-> text
        auto& user = vm->user();
        Item& item = fifthItem(user);
        if (item.isNull()) user.push("");
        else user.push(item.doc()->toPlainText());
    });
    vm->addBuiltin("<text", [this](fifth::vm* vm) {   // id text -u-> id
        auto& user = vm->user();
        auto t = user.pop();
        auto i = user.pop();
        Item& item = fifthItem(user);
        auto text = t.asString().str();

        if (!item.isNull() && !text.isEmpty()) item.doc()->setPlainText(text);
        user.push(i);
    });
    vm->addBuiltin("text>", [this](fifth::vm* vm) {   // text id -u-> text
        auto& user = vm->user();
        Item& item = fifthItem(user);
        auto t = user.top();
        auto text = t.asString().str();

        if (item.isNull() || text.isEmpty()) return;
        else item.doc()->setPlainText(text);
    });

    vm->addBuiltin("name", [this](fifth::vm* vm) { // id -u-> name
        auto& user = vm->user();
        Item& item = fifthItem(user);
        if (item.isNull()) user.push("");
        else user.push(item.name());
    });
    vm->addBuiltin("<name", [this](fifth::vm* vm) {   // id name -u-> id
        auto& user = vm->user();
        auto n = user.pop();
        auto i = user.top();
        Item& item = fifthItem(user);
        auto name = n.asString().str();

        if (!item.isNull()) item.setName(name);
        user.push(i);
    });
    vm->addBuiltin("name>", [this](fifth::vm* vm) {   // name id -u-> name
        auto& user = vm->user();
        Item& item = fifthItem(user);
        auto n = user.top();
        auto name = n.asString().str();

        if (item.isNull() || name.isEmpty()) return;
        item.setName(name);
    });
    vm->addBuiltin("tags", [this](fifth::vm* vm) {   // id -u-> tag(n) ... tag(1) n
        auto& user = vm->user();
        Item& item = fifthItem(user);
        if (item.isNull()) user.push(0);
        else {
            StringList tags = item.tags();
            for (const auto& tag: tags) user.push(tag);
            user.push(tags.count());
        }
    });
    vm->addBuiltin("+tag", [this](fifth::vm* vm) { // id tag -u-> id
        auto& user = vm->user();                   // tag id -u-> id
        auto t = user.pop();
        auto i = user.pop();
        if (i.isStr() && t.isNum()) swap<fifth::value>(t, i);
        if (i.isNum() && t.isStr()) {
            auto id = i.asNumber();
            auto tag = t.asString().str();
            Item& item = findItem(id);
            if (!item.isNull()) item.addTag(tag);
        }
        user.push(i);
    });
    vm->addBuiltin("-tag", [this](fifth::vm* vm) { // id tag -u-> id
        auto& user = vm->user();                   // tag id -u-> id
        auto t = user.pop();
        auto i = user.pop();
        if (i.isStr() && t.isNum()) swap<fifth::value>(t, i);
        if (i.isNum() && t.isStr()) {
            auto id = i.asNumber();
            auto tag = t.asString().str();
            Item& item = findItem(id);
            item.removeTag(tag);
        }
        user.push(i);
    });
    vm->addBuiltin("?tag", [this](fifth::vm* vm) { // id tag -u-> t/f
        auto& user = vm->user();                   // tag id -u-> t/f
        auto t = user.pop();
        auto i = user.pop();
        if (i.isStr() && t.isNum()) swap<fifth::value>(t, i);
        if (!i.isNum() || !t.isStr()) user.push(false);
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
        Item& item = fifthItem(user);
        if (item.isNull()) user.push(0);
        else user.push(item.position());
    });
    vm->addBuiltin("name", [this](fifth::vm* vm) { // id -u-> name
        auto& user = vm->user();
        Item& item = fifthItem(user);
        if (item.isNull()) user.push("");
        else user.push(item.name());
    });
    vm->addBuiltin("<position", [this](fifth::vm* vm) {   // id position -u-> id
        auto& user = vm->user();
        auto i = user.top();
        Item& item = fifthItem(user);
        auto p = user.pop();
        auto pos = p.asNumber();

        if (!item.isNull() && pos >= 0) item.setPosition(pos);
        user.push(i);
    });
    vm->addBuiltin("position>", [this](fifth::vm* vm) {   // position id -u-> position
        auto& user = vm->user();
        Item& item = fifthItem(user);
        auto p = user.top();
        auto pos = p.asNumber();

        if (item.isNull() || pos < 0) return;
        else item.setPosition(pos);
    });
}

bool Novel::fromObject(Json5Object& obj) {
    mFilename = Item::hasStr(obj, Filename, "");
    mExtra =    Item::hasObj(obj, Extra, {});
    // ok, since we nede images per document, not per file now since document central now,
    // need to grab the images and put them someplace safe until each image is loaded
    // in the appropriate document.
    auto images = Item::hasArr(mExtra, Images, {});
    Item::sImages.clear();
    for (auto& obj: images) {
        if (!obj.isObject()) continue;
        Json5Object image = obj.toObject();
        QString url = Item::hasStr(image, { V1Url, Url }, {});
        if (url.isEmpty()) continue;
        QString str = Item::hasStr(image, { V1Data, Data }, {});
        if (str.isEmpty()) continue;
        QByteArray data = QByteArray::fromBase64(str.toUtf8());
        if (data.isEmpty()) continue;
        QImage img;
        img.loadFromData(data);
        if (img.isNull()) continue;
        Item::sImages[url] = img;
    }

    // load images into Item::sImages;
    mItems.clear();
    mRoot =     Item::hasNum(obj, Root, qlonglong(0));

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
    Item::sImages.clear();
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

void Novel::saveImages(Json5Object& obj) {

}

TreeNode::TreeNode(Json5Object& obj) {
    mId                 = Item::hasNum(obj, Novel::BranchId, qlonglong(0));
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
