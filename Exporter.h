#pragma once

#include <QObject>

#include <List.h>

#include "Novel.h"

struct ExportMetadataField {
    QString                     key;          // "title", "author", "language"
    QString                     label;        // "Book Title", "Author Name"
    QString                     defaultValue; // optional
    bool                        required   { false };
    std::function<StringList()> getChoices { nullptr };
    QString                     value      { "" };
};

class ExporterBase {
public:
    using Factory = std::function<ExporterBase*(Novel&, const QList<qlonglong>&)>;

    virtual ~ExporterBase() = default;

    virtual QMap<QString, QString> collectMetadataDefaults() = 0;
    virtual bool                   convert() = 0;
    virtual QString                fileExtension() = 0;
    virtual QString                name() = 0;
    virtual void                   setChapterTag(const QString& tag) = 0;
    virtual void                   setCoverTag(const QString& tag) = 0;
    virtual void                   setFilename(const QString& tag) = 0;
    virtual void                   setIds(const QList<qlonglong>& ids) = 0;
    virtual void                   setInternalImages(const QMap<QUrl, QImage>& images) = 0;
    virtual void                   setMetadataValue(const QString& meta, const QString& value) = 0;
    virtual void                   setSceneSeparator(const QString& sep) = 0;
    virtual void                   setSceneTag(const QString& tag) = 0;
    virtual void                   setUseSceneSeparator(bool use) = 0;

    virtual QList<ExportMetadataField>& metadataFields() {
        static QList<ExportMetadataField> nil;
        return nil;
    }

    static const QMap<QString, Factory>& registry() {
        return getRegistry();
    }

protected:
    QString fetchValue(qlonglong idx, const QMap<QString, QString>& defaults, const QString& name) {
        QString res = "";
        auto& metaData = metadataFields();
        QString def = defaults.contains(name) ? defaults[name] : "";
        if (!metaData[idx].value.isEmpty()) res = metaData[idx].value;
        else if (!def.isEmpty()) res = def;
        return res;
    }

    static QMap<QString, Factory>& getRegistry() {
        static QMap<QString, Factory> map;
        return map;
    }

    static void registerExporter(const QString& name, Factory factory) {
        auto insert = getRegistry().insert(name, factory);
        Q_ASSERT_X(insert.value(), "ExporterBase::registerExporter", qPrintable(QString("Duplicate '%1'").arg(name)));
    }
};


template<typename Class>
class tExporter: public ExporterBase {
public:
    void setChapterTag(const QString& c) override                     { mChapterTag = c.toLower(); }
    void setCoverTag(const QString& c) override                       { mCoverTag = c.toLower(); }
    void setInternalImages(const QMap<QUrl, QImage>& images) override { mImages = images; }
    void setFilename(const QString& f) override                       { mFilename = f; }
    void setIds(const QList<qlonglong>& ids) override                 { mItemIds = ids; }
    void setSceneSeparator(const QString& sep) override               { mSceneSeparator = sep; }
    void setSceneTag(const QString& s) override                       { mSceneTag = s.toLower(); }
    void setUseSceneSeparator(bool use) override                      { mUseSceneSeparator = use; }

    QMap<QString, QString> collectMetadataDefaults() override {
        QMap<QString, QString> defaults;

        for (int&& id: mItemIds) {
            Item& item = mNovel.findItem(id);

            for (const QString& tag: item.tags()) {
                int idx = tag.indexOf(':');
                if (idx <= 0) continue;

                QString key = tag.left(idx).trimmed().toLower();
                QString value = tag.mid(idx + 1).trimmed();

                // First-found wins
                if (!defaults.contains(key)) defaults[key] = value;
            }
        }

        auto meta = metadataFields();
        for (const auto& field: meta) {
            if (!defaults.contains(field.key)) defaults[field.key] = field.defaultValue;
        }

        return defaults;
    }

    void setMetadataValue(const QString& meta, const QString& value) override {
        for (auto& set: metadataFields()) {
            if (set.key == meta) {
                set.value = value;
                return;
            }
        }
    }

    QString fileExtension() override { return Class::Extension(); }
    QString name() override          { return Class::Name(); }

protected:
    tExporter(Novel& novel,
              const QList<qlonglong>& ids)
        : mItemIds(ids)
        , mNovel(novel) { }

    static bool registerSelf() {
        ExporterBase::registerExporter(Class::Name(), [](Novel& n, const QList<qlonglong>& ids) {
                return new Class(n, ids);
            }
        );
        return true;
    }

protected:
    QString            mChapterTag;
    QString            mCoverTag;
    QString            mFilename;
    QMap<QUrl, QImage> mImages;
    QList<qlonglong>   mItemIds;
    Novel&             mNovel;
    QString            mSceneSeparator;
    QString            mSceneTag;
    bool               mUseSceneSeparator;
};

template<typename Class>
class Exporter : public tExporter<Class> {
public:
    Exporter(Novel& novel,
             const QList<qlonglong>& ids)
        : tExporter<Class>(novel, ids) { }

private:
    static bool   registered;
};

template<typename Class>
bool Exporter<Class>::registered = tExporter<Class>::registerSelf();

/*
class Test: public Exporter<Test> {
public:
    Test(Novel& n, const List<qlonglong>& i)
        : Exporter(n, i) { }

    bool convert() override { return true; }

    QList<ExportMetadataField> metadataFields() const override {
        return {
            { "title",    "Book Title", "", true },
            { "author",   "Author",     "" },
            { "language", "Language",  "en" }
        };
    }

    static QString Extension() { return "tst"; }
    static QString Name()      { return "Test"; }
};
*/
