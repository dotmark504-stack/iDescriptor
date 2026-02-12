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

#include "appinstalldialog.h"
#include "appcontext.h"
#include "appdownloadbasedialog.h"
#include "iDescriptor.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

AppInstallDialog::AppInstallDialog(const QString &appName,
                                   const QString &description,
                                   const QString &bundleId,
                                   InstallMode mode, QWidget *parent)
    : AppDownloadBaseDialog(appName, bundleId, parent), m_bundleId(bundleId),
      m_statusLabel(nullptr), m_modeLabel(nullptr), m_localPathLabel(nullptr),
      m_storeModeButton(nullptr), m_localModeButton(nullptr),
      m_chooseIpaButton(nullptr), m_installWatcher(nullptr),
      m_installMode(mode)
{
    setWindowTitle("Install " + appName + " - iDescriptor");
    setModal(true);
    setFixedWidth(500);
    m_manager = new QNetworkAccessManager(this);
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(this->layout());
    // App info section
    QHBoxLayout *appInfoLayout = new QHBoxLayout();
    QLabel *iconLabel = new QLabel();
    ::fetchAppIconFromApple(
        m_manager, bundleId, [iconLabel](const QPixmap &pixmap) {
            if (!pixmap.isNull()) {
                QPixmap scaled =
                    pixmap.scaled(64, 64, Qt::KeepAspectRatioByExpanding,
                                  Qt::SmoothTransformation);
                QPixmap rounded(64, 64);
                rounded.fill(Qt::transparent);

                QPainter painter(&rounded);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addRoundedRect(QRectF(0, 0, 64, 64), 16, 16);
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, scaled);
                painter.end();

                iconLabel->setPixmap(rounded);
            }
        });
    QPixmap icon = QApplication::style()
                       ->standardIcon(QStyle::SP_ComputerIcon)
                       .pixmap(64, 64);
    iconLabel->setPixmap(icon);
    iconLabel->setFixedSize(64, 64);
    appInfoLayout->addWidget(iconLabel);

    QVBoxLayout *detailsLayout = new QVBoxLayout();
    QLabel *nameLabel = new QLabel(appName);
    nameLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
    detailsLayout->addWidget(nameLabel);

    QLabel *descLabel = new QLabel(description);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("font-size: 14px;");
    detailsLayout->addWidget(descLabel);

    appInfoLayout->addLayout(detailsLayout);
    appInfoLayout->addStretch();
    layout->insertLayout(0, appInfoLayout);

    m_modeLabel = new QLabel("Install source:");
    m_modeLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    layout->insertWidget(1, m_modeLabel);

    QHBoxLayout *modeButtonsLayout = new QHBoxLayout();
    m_storeModeButton = new QPushButton("Install from App Store");
    m_storeModeButton->setCheckable(true);
    m_localModeButton = new QPushButton("Install local IPA file");
    m_localModeButton->setCheckable(true);
    modeButtonsLayout->addWidget(m_storeModeButton);
    modeButtonsLayout->addWidget(m_localModeButton);
    layout->insertLayout(2, modeButtonsLayout);

    connect(m_storeModeButton, &QPushButton::clicked, this,
            &AppInstallDialog::onModeChanged);
    connect(m_localModeButton, &QPushButton::clicked, this,
            &AppInstallDialog::onModeChanged);

    m_chooseIpaButton = new QPushButton("Choose IPA file...");
    connect(m_chooseIpaButton, &QPushButton::clicked, this,
            &AppInstallDialog::onChooseIpaClicked);
    layout->insertWidget(3, m_chooseIpaButton);

    m_localPathLabel = new QLabel("No IPA file selected");
    m_localPathLabel->setWordWrap(true);
    m_localPathLabel->setStyleSheet("font-size: 12px; color: #666;");
    layout->insertWidget(4, m_localPathLabel);

    QLabel *deviceLabel = new QLabel("Choose Device:");
    deviceLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    layout->insertWidget(5, deviceLabel);

    m_deviceCombo = new QComboBox();
    layout->insertWidget(6, m_deviceCombo);

    m_statusLabel = new QLabel("Ready to install");
    m_statusLabel->setStyleSheet("font-size: 14px; padding: 5px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    layout->insertWidget(7, m_statusLabel);

    layout->addStretch();

    m_actionButton = new QPushButton("Install");
    m_actionButton->setFixedHeight(40);

    connect(m_actionButton, &QPushButton::clicked, this,
            &AppInstallDialog::onInstallClicked);
    layout->addWidget(m_actionButton);

    QPushButton *cancelButton = new QPushButton("Cancel");
    cancelButton->setFixedHeight(40);
    cancelButton->setStyleSheet(
        "background-color: #f0f0f0; color: #333; border: 1px solid #ddd; "
        "border-radius: 6px; font-size: 16px;");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(cancelButton);

    connect(AppContext::sharedInstance(), &AppContext::deviceChange, this,
            &AppInstallDialog::updateDeviceList);

    updateDeviceList();
    onModeChanged();
}

void AppInstallDialog::updateDeviceList()
{
    m_deviceCombo->clear();
    auto devices = AppContext::sharedInstance()->getAllDevices();
    if (devices.empty()) {
        m_deviceCombo->addItem("No devices connected");
        m_deviceCombo->setEnabled(false);
        m_actionButton->setDefault(false);
        m_actionButton->setEnabled(false);
        setStatusMessage("No devices connected", true);
    } else {
        m_deviceCombo->setEnabled(true);
        for (const auto &device : devices) {
            QString deviceName =
                QString::fromStdString(device->deviceInfo.productType);
            QString deviceId = QString::fromStdString(device->udid);
            m_deviceCombo->addItem(
                deviceName + " / " + deviceId.left(8) + "...", deviceId);
        }
        m_actionButton->setDefault(true);
        m_actionButton->setEnabled(true);
        setStatusMessage(m_installMode == InstallMode::AppStore
                             ? "Ready to download from App Store"
                             : "Ready to install local IPA");
    }
}

bool AppInstallDialog::hasValidDeviceSelection() const
{
    if (!m_deviceCombo->isEnabled()) {
        return false;
    }

    const QString selectedDevice = m_deviceCombo->currentData().toString();
    return !selectedDevice.isEmpty();
}

bool AppInstallDialog::validateInstallationInputs(QString *errorText) const
{
    if (!hasValidDeviceSelection()) {
        if (errorText) {
            *errorText = "Please select a connected device before installing.";
        }
        return false;
    }

    if (m_installMode == InstallMode::LocalFile) {
        if (m_selectedIpaPath.trimmed().isEmpty()) {
            if (errorText) {
                *errorText = "Please select a local IPA file first.";
            }
            return false;
        }

        QFileInfo ipaInfo(m_selectedIpaPath);
        if (!ipaInfo.exists() || !ipaInfo.isFile()) {
            if (errorText) {
                *errorText = "The selected IPA file does not exist.";
            }
            return false;
        }

        if (!ipaInfo.isReadable()) {
            if (errorText) {
                *errorText =
                    "The selected IPA file is not readable. Check file "
                    "permissions.";
            }
            return false;
        }

        if (ipaInfo.suffix().compare("ipa", Qt::CaseInsensitive) != 0) {
            if (errorText) {
                *errorText = "The selected file must have a .ipa extension.";
            }
            return false;
        }
    }

    return true;
}

void AppInstallDialog::setStatusMessage(const QString &message, bool isError)
{
    if (!m_statusLabel) {
        return;
    }

    if (isError) {
        m_statusLabel->setStyleSheet(
            "font-size: 14px; color: #FF3B30; padding: 5px;");
    } else {
        m_statusLabel->setStyleSheet("font-size: 14px; padding: 5px;");
    }
    m_statusLabel->setText(message);
}

void AppInstallDialog::onModeChanged()
{
    if (sender() == m_storeModeButton) {
        m_installMode = InstallMode::AppStore;
    } else if (sender() == m_localModeButton) {
        m_installMode = InstallMode::LocalFile;
    }

    m_storeModeButton->setChecked(m_installMode == InstallMode::AppStore);
    m_localModeButton->setChecked(m_installMode == InstallMode::LocalFile);

    const bool isLocal = m_installMode == InstallMode::LocalFile;
    m_chooseIpaButton->setVisible(isLocal);
    m_localPathLabel->setVisible(isLocal);
    setStatusMessage(isLocal ? "Ready to install local IPA"
                             : "Ready to download from App Store");
}

void AppInstallDialog::onChooseIpaClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Select IPA file", QString(), "IPA files (*.ipa)");
    if (path.isEmpty()) {
        return;
    }

    m_selectedIpaPath = path;
    m_localPathLabel->setText(m_selectedIpaPath);
    setStatusMessage("Local IPA selected. Ready to install.");
}

void AppInstallDialog::performInstallation(const QString &ipaPath,
                                           const QString &deviceUdid)
{
    setStatusMessage(m_installMode == InstallMode::AppStore
                         ? "Installing downloaded app..."
                         : "Installing local IPA...");

    // Setup install watcher
    m_installWatcher = new QFutureWatcher<int>(this);
    connect(m_installWatcher, &QFutureWatcher<int>::finished, this, [this]() {
        int result = m_installWatcher->result();
        m_installWatcher->deleteLater();
        m_installWatcher = nullptr;

        if (result == 0) {
            setStatusMessage("Installation completed successfully!");
            m_statusLabel->setStyleSheet(
                "font-size: 14px; color: #34C759; padding: 5px;");
            QMessageBox::information(this, "Success",
                                     m_installMode == InstallMode::AppStore
                                         ? "App downloaded and installed "
                                           "successfully!"
                                         : "Local IPA installed successfully!");
            accept();
        } else {
            const QString failureText =
                m_installMode == InstallMode::AppStore
                    ? "App Store installation failed"
                    : "Local IPA installation failed";
            setStatusMessage(failureText, true);
            QMessageBox::critical(
                this, "Installation Error",
                QString("%1 with error code: %2").arg(failureText).arg(result));
        }
    });

    // Run installation in background thread
    QFuture<int> future = QtConcurrent::run([ipaPath, deviceUdid]() -> int {
        iDescriptorDevice *device =
            AppContext::sharedInstance()->getDevice(deviceUdid.toStdString());
        if (!device) {
            return -1;
        }

        instproxy_error_t ret = install_IPA(device->device, device->afcClient,
                                            ipaPath.toStdString().c_str());
        return static_cast<int>(ret);
    });

    m_installWatcher->setFuture(future);
}
void AppInstallDialog::onInstallClicked()
{
    QString errorText;
    if (!validateInstallationInputs(&errorText)) {
        setStatusMessage(errorText, true);
        QMessageBox::warning(this, "Cannot Install", errorText);
        return;
    }

    m_deviceCombo->setEnabled(false);
    m_actionButton->setEnabled(false);

    const QString selectedDevice = m_deviceCombo->currentData().toString();

    if (m_installMode == InstallMode::LocalFile) {
        performInstallation(m_selectedIpaPath, selectedDevice);
        return;
    }

    setStatusMessage("Downloading app from App Store...");

    int buttonIndex = m_layout->indexOf(m_actionButton);
    layout()->removeWidget(m_actionButton);
    m_actionButton->deleteLater();
    m_actionButton = nullptr;

    if (m_tempDir) {
        delete m_tempDir;
        m_tempDir = nullptr;
    }
    // Create a new temporary directory for each installation
    m_tempDir = new QTemporaryDir();
    if (!m_tempDir->isValid()) {
        setStatusMessage("Failed to create temporary directory", true);
        QMessageBox::critical(
            this, "Error",
            "Could not create temporary directory for App Store download.");
        return;
    }

    startDownloadProcess(m_bundleId, m_tempDir->path(), buttonIndex, false);
    connect(this, &AppDownloadBaseDialog::downloadFinished, this,
            [this, selectedDevice](bool success, const QString &) {
                if (success) {
                    qDebug() << "Download finished, starting installation...";
                    /*
                        FIXME: libipatool generates random id and appends that
                       to the downloaded IPA filename, so we need to search for
                       it.
                    */
                    // Find the actual downloaded IPA file
                    QDir outDir(m_tempDir->path());
                    QStringList filters;
                    filters << m_bundleId + "*.ipa";
                    QStringList matches =
                        outDir.entryList(filters, QDir::Files, QDir::Time);
                    if (matches.isEmpty()) {
                        setStatusMessage(
                            "App Store download failed - IPA not found", true);
                        QMessageBox::critical(
                            this, "Download Error",
                            QString("Downloaded IPA not found in %1")
                                .arg(outDir.absolutePath()));
                        return;
                    }

                    QString ipaFile = outDir.filePath(matches.first());
                    performInstallation(ipaFile, selectedDevice);

                } else {
                    setStatusMessage("App Store download failed", true);
                    QMessageBox::critical(
                        this, "Download Error",
                        "Failed to download app from App Store.");
                }
            });
}

void AppInstallDialog::reject()
{
    // Cancel installation if it's running
    if (m_installWatcher && !m_installWatcher->isFinished()) {
        m_installWatcher->cancel();
        m_installWatcher->deleteLater();
        m_installWatcher = nullptr;
        if (m_statusLabel) {
            setStatusMessage("Installation cancelled", true);
        }
    }

    AppDownloadBaseDialog::reject();
}

AppInstallDialog::~AppInstallDialog()
{
    if (m_tempDir) {
        delete m_tempDir;
        m_tempDir = nullptr;
    }
}
