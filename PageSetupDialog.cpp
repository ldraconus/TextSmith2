#include "PageSetupDialog.h"
#include "ui_PageSetupDialog.h"
#include "Main.h"
#include "ui_Main.h"
#include "PdfExporter.h"

#include <QDir>
#include <QLayout>
#include <QPrintPreviewWidget>

static constexpr qreal LeftMarginMin = 0.125;
static constexpr qreal RightMarginMin = 0.125;
static constexpr qreal TopMarginMin = 0.125;
static constexpr qreal BottomMarginMin = 0.125;

static constexpr auto Left = 0;
static constexpr auto Center = 1;
static constexpr auto Right = 2;

PageSetupDialog::PageSetupDialog(QWidget* parent)
    : QDialog(parent)
    , mPrefs(new Preferences(Main::ref().prefs()))
    , mPrinter(Main::ref().printer())
    , mUi(new Ui::PageSetupDialog) {
    mUi->setupUi(this);

    mPreviewTimer = new QTimer(this);
    mPreviewTimer->setSingleShot(true);
    mPreviewTimer->setInterval(2000);
    connect(mPreviewTimer, &QTimer::timeout, this, [this]() { updateFootersAndHeaders(); });

    justify(mUi->footerRightTextEdit,  Qt::AlignRight);
    justify(mUi->footerCenterTextEdit, Qt::AlignCenter);
    justify(mUi->footerLeftTextEdit,   Qt::AlignLeft);
    justify(mUi->headerRightTextEdit,  Qt::AlignRight);
    justify(mUi->headerCenterTextEdit, Qt::AlignCenter);
    justify(mUi->headerLeftTextEdit,   Qt::AlignLeft);

    mPrinterList = QPrinterInfo::availablePrinters();
    StringList printers;
    for (QPrinterInfo& info: mPrinterList) printers << info.printerName();
    mUi->printerComboBox->addItems(printers.toQStringList());

    QPrinterInfo def = QPrinterInfo::defaultPrinter();
    if (!def.isNull()) mUi->printerComboBox->setCurrentText(def.printerName());

    mPrinter = new Printer(QPrinter::ScreenResolution);
    if (mPrinter == nullptr || mPrinter->qprinter() == nullptr) {
        Main::ref().msg().Statement("Unable to talk to the preview printer");
        reject();
        return;
    }

    mPrinter->setPrefs(mPrefs);

    mUi->orientationComboBox->setCurrentIndex((mPrinter->pageOrientation() == QPageLayout::Landscape) ? 1 : 0);

    QString header = mPrefs->header();
    StringList lcr { header.split(Preferences::sep, Qt::KeepEmptyParts) };
    if (lcr.size() == 3) {
        mUi->headerLeftTextEdit->setText(lcr[Left]);
        mUi->headerCenterTextEdit->setText(lcr[Center]);
        mUi->headerRightTextEdit->setText(lcr[Right]);
    }
    QString footer = mPrefs->footer();
    lcr = footer.split(Preferences::sep, Qt::KeepEmptyParts);
    if (lcr.size() == 3) {
        mUi->footerLeftTextEdit->setText(lcr[Left]);
        mUi->footerCenterTextEdit->setText(lcr[Center]);
        mUi->footerRightTextEdit->setText(lcr[Right]);
    }

    connect(mUi->footerLeftTextEdit,   &QTextEdit::textChanged, this, [this] { mFocusWidget = mUi->footerLeftTextEdit;   mPreviewTimer->start(); });
    connect(mUi->footerCenterTextEdit, &QTextEdit::textChanged, this, [this] { mFocusWidget = mUi->footerCenterTextEdit; mPreviewTimer->start(); });
    connect(mUi->footerRightTextEdit,  &QTextEdit::textChanged, this, [this] { mFocusWidget = mUi->footerRightTextEdit;  mPreviewTimer->start(); });
    connect(mUi->headerLeftTextEdit,   &QTextEdit::textChanged, this, [this] { mFocusWidget = mUi->headerLeftTextEdit;   mPreviewTimer->start(); });
    connect(mUi->headerCenterTextEdit, &QTextEdit::textChanged, this, [this] { mFocusWidget = mUi->headerCenterTextEdit; mPreviewTimer->start(); });
    connect(mUi->headerRightTextEdit,  &QTextEdit::textChanged, this, [this] { mFocusWidget = mUi->headerRightTextEdit;  mPreviewTimer->start(); });

    auto margins = mPrefs->margins();
    mUi->leftMarginDoubleSpinBox->setValue(margins[Preferences::Left]);
    mUi->rightMarginDoubleSpinBox->setValue(margins[Preferences::Right]);
    mUi->topMarginDoubleSpinBox->setValue(margins[Preferences::Top]);
    mUi->bottomMarginDoubleSpinBox->setValue(margins[Preferences::Bottom]);

    checkMargins();

    connect(mUi->printerComboBox, &QComboBox::currentIndexChanged, this,
            [this]() {
                if (!mPrinter) return;
                Printer* p = new Printer(mPrinterList[this->mUi->printerComboBox->currentIndex()], QPrinter::HighResolution);
                if (p == nullptr) return;
                Main::ref().setPrinter(p);
                QPrinterInfo info(*p->qprinter());
                auto pageSize = info.defaultPageSize();
                mPrinter->setPageSize(pageSize.id());
                QPageLayout::Orientation orientation = QPageLayout::Portrait;
                if (mUi->orientationComboBox->currentIndex() == 1) orientation = QPageLayout::Landscape;
                mPrinter->setPageOrientation(orientation);
                updatePreview();
            });
    connect(mUi->orientationComboBox, &QComboBox::currentIndexChanged, this,
            [this]() {
                QPageLayout::Orientation orientation = QPageLayout::Portrait;
                if (mUi->orientationComboBox->currentIndex() == 1) orientation = QPageLayout::Landscape;
                if (mPrinter) mPrinter->setPageOrientation(orientation);
                updatePreview();
            });

    connect(mUi->leftMarginDoubleSpinBox,   &QDoubleSpinBox::editingFinished, this, [this]() { checkMargins(); });
    connect(mUi->rightMarginDoubleSpinBox,  &QDoubleSpinBox::editingFinished, this, [this]() { checkMargins(); });
    connect(mUi->topMarginDoubleSpinBox,    &QDoubleSpinBox::editingFinished, this, [this]() { checkMargins(); });
    connect(mUi->bottomMarginDoubleSpinBox, &QDoubleSpinBox::editingFinished, this, [this]() { checkMargins(); });

    connect(mPreviewTimer, &QTimer::timeout, this, &PageSetupDialog::updatePreview);

    mPrinter->setOutputFormat(QPrinter::PdfFormat);
    mPrinter->setOutputFileName(QDir::temp().filePath("preview.pdf"));
    mPrinter->setPrinterName(QString());
    mPrinter->setPageOrientation((mUi->orientationComboBox->currentIndex() == 1) ? QPageLayout::Landscape : QPageLayout::Portrait);

    auto* layout = new QVBoxLayout(mUi->previewFrame);
    mUi->previewFrame->setLayout(layout);
    mPreview = new QPrintPreviewWidget(mPrinter->qprinter(), mUi->previewFrame);
    layout->addWidget(mPreview);

    auto* edit = Main::ref().ui()->textEdit;
    mPrinter->setImages(edit->internalImages());
    auto& images = edit->externalImageUrls();
    for (const auto& url: images) mPrinter->addImage(url);
    mPrinter->setIds(Main::ref().vectorOfIds(Main::ref().ui()->treeWidget->topLevelItem(0),
                                             { mPrefs->chapterTag(), mPrefs->sceneTag(), mPrefs->coverTag() }));

    connect(mPreview, &QPrintPreviewWidget::paintRequested, this,
            [this](QPrinter* printer) {
                if (mPrinter == nullptr) return;
                if (mPrinter->qprinter() == nullptr) return;
                Printer* savedPrinter = Main::ref().exchangePrinter(mPrinter);
                Main::ref().printNovel();
                mPrinter = Main::ref().exchangePrinter(savedPrinter);
            });

    mReady = true;
}

PageSetupDialog::~PageSetupDialog() {
    delete mPreviewTimer;
    delete mPrinter;
    delete mUi;
}

bool PageSetupDialog::event(QEvent *e) {
    if (e->type() == QEvent::ShortcutOverride) {
        auto *ke = static_cast<QKeyEvent*>(e);

        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            ke->accept();   // tell Qt: “don’t use this as a dialog shortcut”
            return true;    // swallow it
        }
    } else if (e->type() == QEvent::FocusOut) {
        auto* foe = dynamic_cast<QFocusEvent*>(e);
        if (mFocusWidget == mUi->footerCenterTextEdit
            || mFocusWidget == mUi->footerLeftTextEdit
            || mFocusWidget == mUi->footerRightTextEdit
            || mFocusWidget == mUi->headerCenterTextEdit
            || mFocusWidget == mUi->headerLeftTextEdit
            || mFocusWidget == mUi->headerRightTextEdit) updatePreview();
    }

    return QDialog::event(e);
}

QString PageSetupDialog::footer() {
    return mUi->footerLeftTextEdit->toHtml() + Preferences::sep + mUi->footerCenterTextEdit->toHtml() + Preferences::sep + mUi->footerRightTextEdit->toHtml();
}

QString PageSetupDialog::header() {
    return mUi->headerLeftTextEdit->toHtml() + Preferences::sep + mUi->headerCenterTextEdit->toHtml() + Preferences::sep + mUi->headerRightTextEdit->toHtml();
}

void PageSetupDialog::justify(QTextEdit* edit, Qt::AlignmentFlag which) {
    auto cursor = edit->textCursor();
    auto block = cursor.blockFormat();
    block.setAlignment(which);
    cursor.setBlockFormat(block);
    edit->setTextCursor(cursor);
}

double PageSetupDialog::bottomMargin() { return mUi->bottomMarginDoubleSpinBox->value(); }
double PageSetupDialog::leftMargin()   { return mUi->leftMarginDoubleSpinBox->value(); }
double PageSetupDialog::rightMargin()  { return mUi->rightMarginDoubleSpinBox->value(); }
double PageSetupDialog::topMargin()    { return mUi->topMarginDoubleSpinBox->value(); }

void PageSetupDialog::marginChanged() {
    auto* edit = dynamic_cast<QDoubleSpinBox*>(sender());
    if (edit == nullptr) return;
    auto x = edit->value();
    edit->setValue(x);
    setMargins();
}

template <typename T>
concept LessThanComparable =
    requires (const T& a, const T& b) {
        { a < b } -> std::convertible_to<bool>;
    };
template <LessThanComparable T>
T max(T& val, const T& min) { return (val < min) ? min : val; }

void PageSetupDialog::checkMargins() {
    if (mPrinter == nullptr) return;

    QList<qreal> margins;
    margins.fill(1.0, 4);
    qreal left =   mUi->leftMarginDoubleSpinBox->value();
    qreal right =  mUi->rightMarginDoubleSpinBox->value();
    qreal top =    mUi->topMarginDoubleSpinBox->value();
    qreal bottom = mUi->bottomMarginDoubleSpinBox->value();

    max(left,   LeftMarginMin);
    max(right,  RightMarginMin);
    max(top,    TopMarginMin);
    max(bottom, BottomMarginMin);

    auto minLR = LeftMarginMin + RightMarginMin;
    if (left + right > mPrinter->pageSize(QPrinter::Inch).width() - minLR) {
        left =  LeftMarginMin;
        right = RightMarginMin;
    }
    auto minTB = TopMarginMin + BottomMarginMin;
    if (top + bottom > mPrinter->pageSize(QPrinter::Inch).height() - minTB) {
        top =    TopMarginMin;
        bottom = BottomMarginMin;
    }

    mUi->leftMarginDoubleSpinBox->setValue(left);
    mUi->rightMarginDoubleSpinBox->setValue(right);
    mUi->topMarginDoubleSpinBox->setValue(top);
    mUi->bottomMarginDoubleSpinBox->setValue(bottom);

    setMargins();
}

void PageSetupDialog::setFooter() {
    mPrefs->setFooter(footer());
}

void PageSetupDialog::setHeader() {
    mPrefs->setHeader(header());
}

void PageSetupDialog::setMargins() {
    qreal left =   PdfExporter::toInches(value(mUi->leftMarginDoubleSpinBox));
    qreal top =    PdfExporter::toInches(value(mUi->topMarginDoubleSpinBox));
    qreal right =  PdfExporter::toInches(value(mUi->rightMarginDoubleSpinBox));
    qreal bottom = PdfExporter::toInches(value(mUi->bottomMarginDoubleSpinBox));
    mPrefs->setMargins({ left, top, right, bottom });
    updatePreview();
}

void PageSetupDialog::updateFootersAndHeaders() {
    mPrefs->setFooter(footer());
    mPrefs->setHeader(header());
    updatePreview();
}

void PageSetupDialog::updatePreview() {
    if (mPreview && mReady) mPreview->updatePreview();
}

QString PageSetupDialog::value(QDoubleSpinBox* edit) {
    return QString::number(edit->value());
}
