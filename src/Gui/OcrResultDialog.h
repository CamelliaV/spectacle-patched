/* SPDX-License-Identifier: LGPL-2.0-or-later */
#pragma once

#include <QDialog>
#include <QImage>

class QLabel;
class QTextEdit;
class QPushButton;
class QComboBox;

class OcrResultDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OcrResultDialog(QWidget *parent = nullptr);
    void setResult(const QString &text, const QString &message);
    void setSourceImage(const QImage &image, const QString &languageCode);
    void setBusy(bool busy);

Q_SIGNALS:
    void recognitionRequested(const QImage &image, const QString &languageCode);

private:
    QLabel *mStatus;
    QTextEdit *mText;
    QPushButton *mCopy;
    QComboBox *mLanguages;
    QPushButton *mRetry;
    QImage mImage;
};
