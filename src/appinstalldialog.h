/*
 * iDescriptor: A free and open-source idevice management tool.
 *
 * Copyright (C) 2025 Uncore <https://github.com/uncor3>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef APPINSTALLDIALOG_H
#define APPINSTALLDIALOG_H

#include "appdownloadbasedialog.h"
#include <QComboBox>
#include <QDialog>
#include <QFutureWatcher>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QTemporaryDir>

class AppInstallDialog : public AppDownloadBaseDialog
{
    Q_OBJECT
public:
    enum class InstallMode { AppStore, LocalFile };

    explicit AppInstallDialog(const QString &appName,
                              const QString &description,
                              const QString &bundleId,
                              InstallMode mode = InstallMode::AppStore,
                              QWidget *parent = nullptr);
    ~AppInstallDialog();

protected:
    void reject() override;

private slots:
    void onInstallClicked();
    void onModeChanged();
    void onChooseIpaClicked();

private:
    bool validateInstallationInputs(QString *errorText = nullptr) const;
    bool hasValidDeviceSelection() const;
    void setStatusMessage(const QString &message, bool isError = false);

    QComboBox *m_deviceCombo;
    QString m_bundleId;
    QLabel *m_statusLabel;
    QLabel *m_modeLabel;
    QLabel *m_localPathLabel;
    QPushButton *m_storeModeButton;
    QPushButton *m_localModeButton;
    QPushButton *m_chooseIpaButton;
    QFutureWatcher<int> *m_installWatcher;
    QTemporaryDir *m_tempDir = nullptr;
    QNetworkAccessManager *m_manager = nullptr;
    InstallMode m_installMode;
    QString m_selectedIpaPath;
    void updateDeviceList();
    void performInstallation(const QString &ipaPath, const QString &deviceUdid);
};

#endif // APPINSTALLDIALOG_H
