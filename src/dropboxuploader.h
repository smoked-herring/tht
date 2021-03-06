/*
 * This file is part of THT.
 *
 * THT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * THT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with THT.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DROPBOXUPLOADER_H
#define DROPBOXUPLOADER_H

#include <QByteArray>
#include <QDialog>

#include "qdropbox.h"

namespace Ui
{
    class DropBoxUploader;
}

class DropBoxUploader : public QDialog
{
    Q_OBJECT

public:
    explicit DropBoxUploader(const QString &fileName, const QByteArray &binary, QWidget *parent = 0);
    ~DropBoxUploader();

    enum Status { Break, NeedRestart, Error, Done };
    Status status() const;

private:
    QString errorToString(QDropbox::Error) const;
    void message(const QString &text);
    void showError(const QString &text);
    void stopProgress();
    void changeCancelToClose();

private slots:
    void slotRequestTokenFinished(const QString &token, const QString &secret);
    void slotAccessTokenFinished(const QString &token, const QString &secret);
    void slotSharedLinkReceived(const QString &sharedLinkJson);
    void slotTokenExpired();
    void slotErrorOccured(QDropbox::Error);
    void slotDelayedTokenRequest();
    void slotDelayedAccessTokenRequest();
    void slotDelayedWork();

private:
    Ui::DropBoxUploader *ui;
    QString m_fileName;
    QByteArray m_binary;
    enum State { NotInitialized, RequestingToken, RequestingAccessToken, RequestingSharedLink, Work };
    State m_state;
    QString m_token, m_tokenSecret, m_accessToken, m_accessTokenSecret;
    QDropbox *m_api;
    Status m_status;
};

inline
DropBoxUploader::Status DropBoxUploader::status() const
{
    return m_status;
}

#endif // DROPBOXUPLOADER_H
