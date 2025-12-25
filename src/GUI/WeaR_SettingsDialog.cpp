#include "WeaR_SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>

namespace WeaR {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("WeaR Configuration");
    resize(600, 450);
    
    // Set up Deep Kernel Dark styling
    setStyleSheet(R"(
        QDialog {
            background-color: #1E1E1E;
            color: #E0E0E0;
        }
        QTabWidget::pane {
            border: 1px solid #3E3E42;
            background-color: #252526;
        }
        QTabBar::tab {
            background-color: #2D2D30;
            color: #A0A0A0;
            padding: 8px 16px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background-color: #1E1E1E;
            color: #0078D4;
            border-bottom: 2px solid #0078D4;
        }
        QGroupBox {
            border: 1px solid #3E3E42;
            margin-top: 1em;
            color: #0078D4;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 3px 0 3px;
        }
        QLabel {
            color: #E0E0E0;
        }
        QLineEdit {
            background-color: #333337;
            border: 1px solid #3E3E42;
            color: #FFFFFF;
            padding: 4px;
            selection-background-color: #0078D4;
        }
        QPushButton {
            background-color: #3E3E42;
            border: none;
            color: #FFFFFF;
            padding: 6px 16px;
        }
        QPushButton:hover {
            background-color: #505050;
        }
        QPushButton:pressed {
            background-color: #0078D4;
        }
        QComboBox {
            background-color: #333337;
            border: 1px solid #3E3E42;
            color: #FFFFFF;
            padding: 4px;
        }
        QCheckBox {
            color: #E0E0E0;
        }
        QSlider::groove:horizontal {
            border: 1px solid #3E3E42;
            height: 8px;
            background: #2D2D30;
            margin: 2px 0;
        }
        QSlider::handle:horizontal {
            background: #0078D4;
            border: 1px solid #0078D4;
            width: 18px;
            height: 18px;
            margin: -6px 0; 
            border-radius: 9px;
        }
    )");

    setupUI();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

QVariant SettingsDialog::getSetting(const QString& key, const QVariant& defaultValue)
{
    QSettings settings("WeaR-Team", "WeaR-emu");
    return settings.value(key, defaultValue);
}

void SettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);

    // ====================================================================
    // 1. General Tab
    // ====================================================================
    QWidget* generalTab = new QWidget;
    QVBoxLayout* generalLayout = new QVBoxLayout(generalTab);
    
    QGroupBox* firmwareGroup = new QGroupBox("Firmware", generalTab);
    QVBoxLayout* firmwareLayout = new QVBoxLayout(firmwareGroup);
    
    QHBoxLayout* pathLayout = new QHBoxLayout;
    m_firmwarePathEdit = new QLineEdit;
    QPushButton* browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, this, &SettingsDialog::selectFirmwarePath);
    
    pathLayout->addWidget(new QLabel("Path:"));
    pathLayout->addWidget(m_firmwarePathEdit);
    pathLayout->addWidget(browseButton);
    firmwareLayout->addLayout(pathLayout);
    
    QGroupBox* debugGroup = new QGroupBox("Debugging", generalTab);
    QVBoxLayout* debugLayout = new QVBoxLayout(debugGroup);
    m_enableDebugConsole = new QCheckBox("Enable Debug Console / Log Window");
    debugLayout->addWidget(m_enableDebugConsole);

    generalLayout->addWidget(firmwareGroup);
    generalLayout->addWidget(debugGroup);
    generalLayout->addStretch();
    m_tabs->addTab(generalTab, "General");

    // ====================================================================
    // 2. Graphics Tab
    // ====================================================================
    QWidget* graphicsTab = new QWidget;
    QVBoxLayout* graphicsLayout = new QVBoxLayout(graphicsTab);
    
    QGroupBox* displayGroup = new QGroupBox("Display Settings", graphicsTab);
    QGridLayout* displayGrid = new QGridLayout(displayGroup);
    
    displayGrid->addWidget(new QLabel("Resolution Scale:"), 0, 0);
    m_resolutionScaleCombo = new QComboBox;
    m_resolutionScaleCombo->addItem("720p (1x)", 1);
    m_resolutionScaleCombo->addItem("1080p (1.5x)", 1.5);
    m_resolutionScaleCombo->addItem("1440p (2x)", 2);
    m_resolutionScaleCombo->addItem("4K (3x)", 3);
    displayGrid->addWidget(m_resolutionScaleCombo, 0, 1);
    
    displayGrid->addWidget(new QLabel("Aspect Ratio:"), 1, 0);
    m_aspectRatioCombo = new QComboBox;
    m_aspectRatioCombo->addItem("16:9 (Default)");
    m_aspectRatioCombo->addItem("21:9 (Ultrawide)");
    m_aspectRatioCombo->addItem("4:3 (Legacy)");
    displayGrid->addWidget(m_aspectRatioCombo, 1, 1);
    
    m_enableVSync = new QCheckBox("Enable VSync / Vertical Sync");
    displayGrid->addWidget(m_enableVSync, 2, 0, 1, 2);
    
    // WeaR-Gen Frame Generation
    m_enableWeaRGen = new QCheckBox("Enable WeaR-Gen (AI Frame Generation)");
    m_enableWeaRGen->setToolTip("Use compute shaders to generate intermediate frames");
    displayGrid->addWidget(m_enableWeaRGen, 3, 0, 1, 2);
    
    graphicsLayout->addWidget(displayGroup);
    graphicsLayout->addStretch();
    m_tabs->addTab(graphicsTab, "Graphics");

    // ====================================================================
    // 3. Audio Tab
    // ====================================================================
    QWidget* audioTab = new QWidget;
    QVBoxLayout* audioLayout = new QVBoxLayout(audioTab);
    
    QGroupBox* volumeGroup = new QGroupBox("Volume Control", audioTab);
    QHBoxLayout* volumeLayout = new QHBoxLayout(volumeGroup);
    
    m_masterVolumeSlider = new QSlider(Qt::Horizontal);
    m_masterVolumeSlider->setRange(0, 100);
    m_volumeLabel = new QLabel("100%");
    m_volumeLabel->setFixedWidth(40);
    
    connect(m_masterVolumeSlider, &QSlider::valueChanged, [this](int val){
        m_volumeLabel->setText(QString("%1%").arg(val));
    });
    
    volumeLayout->addWidget(new QLabel("Master Volume:"));
    volumeLayout->addWidget(m_masterVolumeSlider);
    volumeLayout->addWidget(m_volumeLabel);
    
    audioLayout->addWidget(volumeGroup);
    audioLayout->addStretch();
    m_tabs->addTab(audioTab, "Audio");

    // ====================================================================
    // 4. System Tab
    // ====================================================================
    QWidget* systemTab = new QWidget;
    QVBoxLayout* systemLayout = new QVBoxLayout(systemTab);
    
    QGroupBox* regionGroup = new QGroupBox("Region & Language", systemTab);
    QGridLayout* regionGrid = new QGridLayout(regionGroup);
    
    regionGrid->addWidget(new QLabel("System Language:"), 0, 0);
    m_languageCombo = new QComboBox;
    m_languageCombo->addItems({"English (US)", "Japanese", "French", "German", "Spanish"});
    regionGrid->addWidget(m_languageCombo, 0, 1);
    
    regionGrid->addWidget(new QLabel("Console Region:"), 1, 0);
    m_regionCombo = new QComboBox;
    m_regionCombo->addItems({"NTSC-U (USA)", "NTSC-J (Japan)", "PAL (Europe)"});
    regionGrid->addWidget(m_regionCombo, 1, 1);
    
    systemLayout->addWidget(regionGroup);
    systemLayout->addStretch();
    m_tabs->addTab(systemTab, "System");

    // ====================================================================
    // 5. Input Tab (NEW)
    // ====================================================================
    QWidget* inputTab = new QWidget;
    QVBoxLayout* inputLayout = new QVBoxLayout(inputTab);
    
    QGroupBox* inputGroup = new QGroupBox("Input Backend", inputTab);
    QVBoxLayout* inputGroupLayout = new QVBoxLayout(inputGroup);
    
    QHBoxLayout* backendLayout = new QHBoxLayout;
    backendLayout->addWidget(new QLabel("Backend:"));
    m_inputBackendCombo = new QComboBox;
    m_inputBackendCombo->addItem("Keyboard Only");
    m_inputBackendCombo->addItem("XInput Gamepad (Xbox Controller)");
    m_inputBackendCombo->addItem("Auto-Detect");
    backendLayout->addWidget(m_inputBackendCombo);
    inputGroupLayout->addLayout(backendLayout);
    
    QLabel* inputNote = new QLabel("Note: XInput requires a compatible Xbox controller connected.");
    inputNote->setStyleSheet("color: #888888; font-style: italic;");
    inputGroupLayout->addWidget(inputNote);
    
    inputLayout->addWidget(inputGroup);
    inputLayout->addStretch();
    m_tabs->addTab(inputTab, "Input");

    // ====================================================================
    // 6. Experimental Tab (NEW)
    // ====================================================================
    QWidget* experimentalTab = new QWidget;
    QVBoxLayout* experimentalLayout = new QVBoxLayout(experimentalTab);
    
    QGroupBox* dangerGroup = new QGroupBox("⚠️ Experimental Features", experimentalTab);
    dangerGroup->setStyleSheet("QGroupBox { border-color: #FF6600; color: #FF6600; }");
    QVBoxLayout* dangerLayout = new QVBoxLayout(dangerGroup);
    
    m_unlockFpsLimit = new QCheckBox("Unlock FPS Limit (May cause instability)");
    m_enableValidationLayers = new QCheckBox("Enable Vulkan Validation Layers (Slower)");
    m_aggressiveRecompiler = new QCheckBox("Aggressive Shader Recompiler (Experimental)");
    
    dangerLayout->addWidget(m_unlockFpsLimit);
    dangerLayout->addWidget(m_enableValidationLayers);
    dangerLayout->addWidget(m_aggressiveRecompiler);
    
    QLabel* warningLabel = new QLabel("These settings may cause crashes or unexpected behavior.");
    warningLabel->setStyleSheet("color: #FF4444; font-weight: bold;");
    dangerLayout->addWidget(warningLabel);
    
    experimentalLayout->addWidget(dangerGroup);
    experimentalLayout->addStretch();
    m_tabs->addTab(experimentalTab, "Experimental");

    // ====================================================================
    // Main Layout Assembly
    // ====================================================================
    mainLayout->addWidget(m_tabs);

    // Dialog Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    
    QPushButton* applyBtn = new QPushButton("Apply");
    QPushButton* cancelBtn = new QPushButton("Cancel");
    
    connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::applySettings);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsDialog::reject);
    
    buttonLayout->addWidget(applyBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::selectFirmwarePath()
{
    QString path = QFileDialog::getOpenFileName(this, "Select PS4 Firmware", "", "Firmware Files (*.pup *.bin)");
    if (!path.isEmpty()) {
        m_firmwarePathEdit->setText(path);
    }
}

void SettingsDialog::loadSettings()
{
    QSettings settings("WeaR-Team", "WeaR-emu");

    // General
    m_firmwarePathEdit->setText(settings.value("General/FirmwarePath", "").toString());
    m_enableDebugConsole->setChecked(settings.value("General/DebugConsole", true).toBool());

    // Graphics
    int resIndex = m_resolutionScaleCombo->findData(settings.value("Graphics/ResolutionScale", 1).toDouble());
    if (resIndex != -1) m_resolutionScaleCombo->setCurrentIndex(resIndex);
    
    m_enableVSync->setChecked(settings.value("Graphics/VSync", true).toBool());
    
    QString aspect = settings.value("Graphics/AspectRatio", "16:9 (Default)").toString();
    m_aspectRatioCombo->setCurrentText(aspect);
    
    m_enableWeaRGen->setChecked(settings.value("Graphics/WeaRGen", false).toBool());

    // Audio
    int vol = settings.value("Audio/MasterVolume", 100).toInt();
    m_masterVolumeSlider->setValue(vol);

    // System
    m_languageCombo->setCurrentText(settings.value("System/Language", "English (US)").toString());
    m_regionCombo->setCurrentText(settings.value("System/Region", "NTSC-U (USA)").toString());

    // Input
    m_inputBackendCombo->setCurrentText(settings.value("Input/Backend", "Auto-Detect").toString());

    // Experimental
    m_unlockFpsLimit->setChecked(settings.value("Experimental/UnlockFPS", false).toBool());
    m_enableValidationLayers->setChecked(settings.value("Experimental/ValidationLayers", false).toBool());
    m_aggressiveRecompiler->setChecked(settings.value("Experimental/AggressiveRecompiler", false).toBool());
}

void SettingsDialog::saveSettings()
{
    QSettings settings("WeaR-Team", "WeaR-emu");

    // General
    settings.setValue("General/FirmwarePath", m_firmwarePathEdit->text());
    settings.setValue("General/DebugConsole", m_enableDebugConsole->isChecked());

    // Graphics
    settings.setValue("Graphics/ResolutionScale", m_resolutionScaleCombo->currentData());
    settings.setValue("Graphics/VSync", m_enableVSync->isChecked());
    settings.setValue("Graphics/AspectRatio", m_aspectRatioCombo->currentText());
    settings.setValue("Graphics/WeaRGen", m_enableWeaRGen->isChecked());

    // Audio
    settings.setValue("Audio/MasterVolume", m_masterVolumeSlider->value());

    // System
    settings.setValue("System/Language", m_languageCombo->currentText());
    settings.setValue("System/Region", m_regionCombo->currentText());

    // Input
    settings.setValue("Input/Backend", m_inputBackendCombo->currentText());

    // Experimental
    settings.setValue("Experimental/UnlockFPS", m_unlockFpsLimit->isChecked());
    settings.setValue("Experimental/ValidationLayers", m_enableValidationLayers->isChecked());
    settings.setValue("Experimental/AggressiveRecompiler", m_aggressiveRecompiler->isChecked());

    accept(); // Close the dialog
}

void SettingsDialog::applySettings()
{
    saveSettings();
    // In a real app, you might want a signal here to notify other parts of the app
    // e.g. emit settingsChanged();
}

} // namespace WeaR
