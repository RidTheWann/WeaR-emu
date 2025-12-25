#pragma once

#include <QDialog>
#include <QSettings>
#include <QScopedPointer>

class QTabWidget;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QSlider;
class QLabel;

namespace WeaR {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

    // Static helper to get a setting value anywhere in the app
    static QVariant getSetting(const QString& key, const QVariant& defaultValue = QVariant());

private slots:
    void applySettings();
    void selectFirmwarePath();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    // UI Components
    QTabWidget* m_tabs;

    // General Tab
    QLineEdit* m_firmwarePathEdit;
    QCheckBox* m_enableDebugConsole;

    // Graphics Tab
    QComboBox* m_resolutionScaleCombo;
    QCheckBox* m_enableVSync;
    QComboBox* m_aspectRatioCombo;
    QCheckBox* m_enableWeaRGen;     // NEW: WeaR-Gen AI Frame Generation

    // Audio Tab
    QSlider* m_masterVolumeSlider;
    QLabel* m_volumeLabel;

    // System Tab
    QComboBox* m_languageCombo;
    QComboBox* m_regionCombo;

    // Input Tab (NEW)
    QComboBox* m_inputBackendCombo;

    // Experimental Tab (NEW)
    QCheckBox* m_unlockFpsLimit;
    QCheckBox* m_enableValidationLayers;
    QCheckBox* m_aggressiveRecompiler;
};

} // namespace WeaR
