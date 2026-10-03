/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "OcrResultDialog.h"

#include <KLocalizedString>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

OcrResultDialog::OcrResultDialog(QWidget *parent)
    : QDialog(parent)
    , mStatus(new QLabel(this))
    , mText(new QTextEdit(this))
    , mCopy(nullptr)
    , mLanguages(new QComboBox(this))
    , mRetry(new QPushButton(i18nc("@action:button", "Recognize Again"), this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(i18nc("@title:window", "Extracted Text"));
    resize(640, 420);
    auto layout = new QVBoxLayout(this);
    mRetry->setObjectName(QStringLiteral("ocrRetryButton"));
    mLanguages->setAccessibleName(i18n("Recognition language"));
    mLanguages->addItem(i18n("Chinese, English and Japanese (Automatic)"), QString());
    mLanguages->addItem(i18n("Simplified Chinese + English (Tesseract)"), QStringLiteral("chi_sim+eng"));
    mLanguages->addItem(i18n("Traditional Chinese + English (Tesseract)"), QStringLiteral("chi_tra+eng"));
    mLanguages->addItem(i18n("Japanese + English (Tesseract)"), QStringLiteral("jpn+eng"));
    mLanguages->addItem(i18n("English (Tesseract)"), QStringLiteral("eng"));
    auto languageRow = new QHBoxLayout;
    languageRow->addWidget(mLanguages, 1);
    languageRow->addWidget(mRetry);
    layout->addLayout(languageRow);
    connect(mRetry, &QPushButton::clicked, this, [this] {
        if (!mImage.isNull()) {
            Q_EMIT recognitionRequested(mImage, mLanguages->currentData().toString());
        }
    });
    mStatus->setWordWrap(true);
    mStatus->setTextFormat(Qt::PlainText);
    mText->setAcceptRichText(false);
    mText->setAccessibleName(i18n("Extracted text"));
    layout->addWidget(mStatus);
    layout->addWidget(mText);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    mCopy = buttons->addButton(i18nc("@action:button", "Copy Text"), QDialogButtonBox::ActionRole);
    connect(mCopy, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(mText->toPlainText());
    });
    connect(mText, &QTextEdit::textChanged, this, [this] {
        mCopy->setEnabled(!mText->toPlainText().isEmpty());
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);
    setResult({}, i18n("Extracting text…"));
}

void OcrResultDialog::setResult(const QString &text, const QString &message)
{
    mStatus->setText(message);
    mText->setPlainText(text);
    mCopy->setEnabled(!text.isEmpty());
    setBusy(false);
}

void OcrResultDialog::setSourceImage(const QImage &image, const QString &languageCode)
{
    mImage = image;
    int index = mLanguages->findData(languageCode);
    if (index < 0) {
        mLanguages->addItem(i18n("Custom languages: %1", languageCode), languageCode);
        index = mLanguages->count() - 1;
    }
    mLanguages->setCurrentIndex(index);
    mRetry->setEnabled(!mImage.isNull());
}

void OcrResultDialog::setBusy(bool busy)
{
    mLanguages->setEnabled(!busy);
    mRetry->setEnabled(!busy && !mImage.isNull());
    mCopy->setEnabled(!busy && !mText->toPlainText().isEmpty());
}

#include "moc_OcrResultDialog.cpp"
