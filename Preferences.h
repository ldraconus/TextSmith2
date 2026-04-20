#pragma once

#include <QApplication>
#include <QJsonArray>
#include <QObject>
#include <QPageLayout>
#include <QPalette>
#include <QPushButton>
#include <QRect>

#include <Json5.h>
#include <List.h>
#include <Map.h>
#include <StringList.h>

class Preferences {
public:
    Preferences();
    explicit Preferences(Json5Object& obj) { read(obj); mWasDark = !isDark(); }
    ~Preferences()                         { save(); }

    static constexpr auto Bottom = 3;
    static constexpr auto Left =   0;
    static constexpr auto Right =  2;
    static constexpr auto Top =    1;

    static constexpr QChar sep = QChar(0x001F);

    struct Recent {
        QString sTitle;
        QString sPath;

        bool operator<(const Recent& r)  const { return (sTitle < r.sTitle) ? true : (sTitle == r.sTitle && sPath < r.sPath); }
        bool operator==(const Recent& r) const { return (sTitle == r.sTitle) && (sPath == r.sPath); }
    };

    enum class Units { Inches, Points };

    void                  addNovel(const Recent& novel);
    void                  applyFontToTree(QWidget* w, const QFont& f);
    QString               checkPath(const QString& path, bool checked);
    bool                  isDark();
    bool                  load();
    List<qreal>           margins(Units unit = Units::Inches) const;
    QString               newPath(const QString& path);
    QPageSize::PageSizeId pageSizeToPid(const QString& size);
    QString               novelPath(const QString& title);
    QString               pidToSize(const QPageSize::PageSizeId pid);
    bool                  read(Json5Object& obj);
    void                  resetIcons(bool isDark);
    bool                  save();
    void                  setDarkTheme();
    void                  setLightTheme();
    void                  setSystemTheme();
    Json5Object           write();

    StringList   actingScripts() const     { return mActingScripts; }
    bool         autoSave() const          { return mAutoSave; }
    qlonglong    autoSaveIntyerval() const { return mAutoSaveInterval; }
    QString      bold() const              { return mBold; }
    QString      chapterTag() const        { return mChapterTag.isEmpty() ? "Chapter" : mChapterTag; }
    QString      coverTag() const          { return mCoverTag.isEmpty() ? "Cover" : mCoverTag; }
    QString      fontFamily() const        { return mFontFamily.isEmpty() ? "Segoe UI" : mFontFamily; }
    qlonglong    fontSize() const          { return mFontSize; }
    QString      footer() const            { return mFooter.isEmpty() ? "&&" : mFooter; }
    QString      header() const            { return mHeader.isEmpty() ? "&&" : mHeader; }
    bool         isDarkTheme() const       { return mIsDark; }
    QString      italic() const            { return mItalic; }
    List<int>    mainSplitter() const      { return mMainSplitter; }
    auto         orientation() const       { return mOrientation; }
    List<int>    otherSplitter() const     { return mOtherSplitter; }
    QString      pageSize() const          { return mPageSize; }
    qlonglong    position() const          { return mPosition; }
    List<Recent> recentNovels() const      { return mRecentNovels; }
    QString      separator() const         { return mSeparator; }
    QString      sceneTag() const          { return mSceneTag.isEmpty() ? "Scene" : mSceneTag; }
    qlonglong    theme() const             { return mTheme; }
    bool         toolbarVisible() const    { return mToolbarVisible; }
    qreal        top()  const              { return mMargins.at(0); }
    bool         typingSounds() const      { return mTypingSounds; }
    QString      uiFontFamily() const      { return mUiFontFamily.isEmpty() ? "Segoe UI" : mUiFontFamily; }
    qlonglong    uiFontSize() const        { return mUiFontSize; }
    QString      underline() const         { return mUnderline; }
    bool         useSeparator() const      { return mUseSeparator; }
    qlonglong    voice() const             { return mVoice; }
    bool         wasDark() const           { return mWasDark; }
    QRect        windowLocation() const    { return mWindow; }

    void addNovel(const QString& title, const QString& path) { addNovel({ title, path }); }
    void setActingScripts(const StringList& s)               { mActingScripts = s; }
    void setApplicaiton(QApplication* a)                     { mApp = a; }
    void setAutoSave(bool a)                                 { mAutoSave = a; }
    void setAutoSaveInterval(qlonglong i)                    { mAutoSaveInterval = i; }
    void setBold(const QString& b)                           { mBold = b; }
    void setChapterTag(const QString& c)                     { mChapterTag = c; }
    void setCoverTag(const QString& c)                       { mCoverTag = c; }
    void setFontFamily(const QString& f)                     { mFontFamily = f; }
    void setFontSize(qlonglong s)                            { mFontSize = s; }
    void setFooter(const QString& f)                         { mFooter = f; }
    void setHeader(const QString& h)                         { mHeader = h; }
    void setIsDark(bool x)                                   { mIsDark = x; }
    void setItalic(const QString& i)                         { mItalic = i; }
    void setMainSplitter(const List<int>& s)                 { mMainSplitter = s; }
    void setMargins(const List<qreal>& m)                    { mMargins = m; }
    void setOrientation(const QPageLayout::Orientation o)    { mOrientation = o; }
    void setOtherSplitter(const List<int>& s)                { mOtherSplitter = s; }
    void setPosition(qlonglong pos)                          { mPosition = pos; }
    void setPageSize(const QString& s)                       { mPageSize = s; }
    void setRecentNovels(const List<Recent>& r)              { mRecentNovels = r; }
    void setSceneTag(const QString& s)                       { mSceneTag = s; }
    void setSeparator(const QString& s)                      { mSeparator = s; }
    void setTheme(qlonglong t)                               { mTheme = t; }
    void setToolbarVisible(bool t)                           { mToolbarVisible = t; }
    void setTypingSounds(bool t)                             { mTypingSounds = t; }
    void setUiFontFamily(const QString& f)                   { mUiFontFamily = f; }
    void setUiFontSize(qlonglong s)                          { mUiFontSize = s; }
    void setUnderline(const QString& u)                      { mUnderline = u; }
    void setUseSeparator(bool u)                             { mUseSeparator = u; }
    void setVoice(qlonglong v)                               { mVoice = v; }
    void setWindowLocation(QRect r)                          { mWindow = r; }

private:
    QApplication*            mApp              { nullptr };
    qlonglong                mAutoSaveInterval { 5 * 60 };
    qlonglong                mFontSize         { 9 };
    qlonglong                mPosition         { 0 };
    qlonglong                mTheme            { 2 };
    qlonglong                mUiFontSize       { 9 };
    qlonglong                mVoice            { 0 };
    QString                  mBold             { "*\xF1*" };
    QString                  mChapterTag       { "chapter" };
    QString                  mCoverTag         { "cover" };
    QString                  mFontFamily       { "Segoe UI" };
    QString                  mFooter           { "" };
    QString                  mHeader           { "" };
    QString                  mItalic           { "/\xF1/" };
    List<int>                mMainSplitter     { };
    List<qreal>              mMargins          { };
    List<int>                mOtherSplitter    { };
    QString                  mPageSize         { "Letter" };
    List<Recent>             mRecentNovels     { };
    QString                  mSceneTag         { "scene" };
    QString                  mSeparator        { "###" };
    QString                  mUiFontFamily     { "Segoe UI" };
    QString                  mUnderline        { "_\xF1_" };
    StringList               mActingScripts    { "" };
    QPageLayout::Orientation mOrientation      { QPageLayout::Portrait };
    QRect                    mWindow;
    bool                     mAutoSave         { false };
    bool                     mIsDark           { false };
    bool                     mToolbarVisible   { true };
    bool                     mTypingSounds     { false };
    bool                     mUseSeparator     { false };
    bool                     mWasDark          { false };
};
