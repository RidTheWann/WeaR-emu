#include "WeaR_GUI.h"
#include "WeaR_Style.h"
#include "WeaR_SettingsDialog.h"
#include "WeaR_Logger.h"

#include "Graphics/WeaR_RenderEngine.h"
#include "Core/WeaR_System.h"
#include "Core/WeaR_EmulatorCore.h"
#include "Input/WeaR_Input.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QDockWidget>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QAction>
#include <QKeyEvent>
#include <QShowEvent>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDir>

#include <iostream>

namespace WeaR {

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

WeaR_GUI::WeaR_GUI(const WeaR_Specs& specs, QWidget* parent)
    : QMainWindow(parent)
    , m_specs(specs)
{
    setWindowTitle("WeaR-emu - PlayStation 4 Emulator");
    setMinimumSize(1000, 650);
    resize(1200, 750);

    applyStylesheet();
    setupUI();
    initializeInputSystem();

    // Connect global logger to GUI log console
    connect(&getLogger(), &WeaR_Logger::logAdded, this, [this](const QString& message, int level) {
        if (m_logConsole) {
            QString color;
            switch (level) {
                case 0: color = "#5A5A5A"; break;  // Debug
                case 1: color = "#CCCCCC"; break;  // Info
                case 2: color = "#E67E22"; break;  // Warning
                case 3: color = "#E74C3C"; break;  // Error
                default: color = "#CCCCCC";
            }
            m_logConsole->append(QString("<span style='color:%1'>%2</span>").arg(color, message));
        }
    });

    // Set application icon (for Windows taskbar only)
    QApplication::setWindowIcon(QIcon(":/resources/wear_logo.png"));
    // Remove icon from window title bar (keep only in taskbar)
    setWindowIcon(QIcon());

    log("[CORE] WeaR-emu initialized", 1);
    log(QString("[GPU] %1").arg(QString::fromStdString(m_specs.gpuName)), 1);
    log(QString("[WEAR-GEN] %1").arg(m_specs.canRunFrameGen ? "Available" : "Not Supported"), 1);
}

WeaR_GUI::~WeaR_GUI() {
    stopRenderLoop();
}

// =============================================================================
// STYLESHEET
// =============================================================================

void WeaR_GUI::applyStylesheet() {
    setStyleSheet(getStyleSheet());
}

// =============================================================================
// MAIN SETUP
// =============================================================================

void WeaR_GUI::setupUI() {
    // Toolbar at top
    setupToolbar();

    // Game table as central widget
    setupGameTable();
    setCentralWidget(m_gameTable);

    // Log dock at bottom
    setupLogDock();

    // Status bar
    setupStatusBar();
}

// =============================================================================
// TOOLBAR (RPCS3 Style)
// =============================================================================

void WeaR_GUI::setupToolbar() {
    m_toolbar = new QToolBar("Main Toolbar", this);
    m_toolbar->setObjectName("mainToolbar");
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setIconSize(QSize(24, 24));
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // Actions with text labels (no emojis)
    m_loadAction = m_toolbar->addAction("Load Game");
    m_loadAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(m_loadAction, &QAction::triggered, this, &WeaR_GUI::onLoadGame);

    m_biosAction = m_toolbar->addAction("Boot BIOS");
    connect(m_biosAction, &QAction::triggered, this, &WeaR_GUI::onBootBios);

    m_toolbar->addSeparator();

    m_settingsAction = m_toolbar->addAction("Settings");
    m_settingsAction->setShortcut(QKeySequence("Ctrl+P"));
    connect(m_settingsAction, &QAction::triggered, this, &WeaR_GUI::onOpenSettings);

    m_refreshAction = m_toolbar->addAction("Refresh");
    m_refreshAction->setShortcut(QKeySequence("F5"));
    connect(m_refreshAction, &QAction::triggered, this, &WeaR_GUI::onRefreshGames);

    addToolBar(Qt::TopToolBarArea, m_toolbar);
}

// =============================================================================
// GAME TABLE
// =============================================================================

void WeaR_GUI::setupGameTable() {
    m_gameTable = new QTableWidget(0, 4, this);
    m_gameTable->setObjectName("gameTable");

    // Headers
    QStringList headers = {"Title", "Serial", "Status", "Path"};
    m_gameTable->setHorizontalHeaderLabels(headers);

    // Styling
    m_gameTable->setShowGrid(false);
    m_gameTable->setAlternatingRowColors(true);
    m_gameTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_gameTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_gameTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gameTable->verticalHeader()->setVisible(false);
    m_gameTable->horizontalHeader()->setStretchLastSection(true);
    m_gameTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_gameTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_gameTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_gameTable->setColumnWidth(1, 100);
    m_gameTable->setColumnWidth(2, 80);

    // Double-click to load
    connect(m_gameTable, &QTableWidget::cellDoubleClicked, 
            this, &WeaR_GUI::onGameDoubleClicked);

    // Add placeholder row
    m_gameTable->insertRow(0);
    m_gameTable->setItem(0, 0, new QTableWidgetItem("No games found"));
    m_gameTable->setItem(0, 1, new QTableWidgetItem("-"));
    m_gameTable->setItem(0, 2, new QTableWidgetItem("-"));
    m_gameTable->setItem(0, 3, new QTableWidgetItem("Use 'Load Game' or add games to directory"));
}

// =============================================================================
// LOG DOCK
// =============================================================================

void WeaR_GUI::setupLogDock() {
    m_logDock = new QDockWidget("Log", this);
    m_logDock->setObjectName("logDock");
    m_logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_logDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

    m_logConsole = new QTextEdit();
    m_logConsole->setObjectName("logConsole");
    m_logConsole->setReadOnly(true);
    m_logConsole->setPlaceholderText("System logs...");
    m_logConsole->setMinimumHeight(120);

    m_logDock->setWidget(m_logConsole);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
}

// =============================================================================
// STATUS BAR
// =============================================================================

void WeaR_GUI::setupStatusBar() {
    QStatusBar* status = statusBar();
    status->setObjectName("statusBar");

    m_stateLabel = new QLabel("Ready");
    m_stateLabel->setObjectName("statusLabel");
    status->addWidget(m_stateLabel);

    status->addPermanentWidget(new QLabel(" | "));

    // Start with Disconnected (will update on first poll)
    m_controllerLabel = new QLabel("Controller: Disconnected");
    m_controllerLabel->setObjectName("statusLabel");
    m_controllerLabel->setStyleSheet("color: #E74C3C;");
    status->addPermanentWidget(m_controllerLabel);

    status->addPermanentWidget(new QLabel(" | "));

    m_fpsLabel = new QLabel("FPS: --");
    m_fpsLabel->setObjectName("statusLabel");
    status->addPermanentWidget(m_fpsLabel);
}

// =============================================================================
// TOOLBAR ACTIONS
// =============================================================================

void WeaR_GUI::onLoadGame() {
    openGameFileDialog();
}

void WeaR_GUI::onBootBios() {
    try {
        log("[BIOS] === BOOT BIOS CLICKED ===", 1);
        
        log("[BIOS] Step 1: Getting emulator core...", 1);
        auto& core = getEmulatorCore();
        
        log("[BIOS] Step 2: Checking initialization...", 1);
        if (!core.isInitialized()) {
            log("[BIOS] Core not initialized, initializing now...", 1);
            if (!core.initialize(m_specs)) {
                log("[ERROR] Failed to initialize emulator core", 3);
                return;
            }
            log("[BIOS] ✓ Core initialized successfully", 1);
        } else {
            log("[BIOS] ✓ Core already initialized", 1);
        }

        log("[BIOS] Step 3: Loading internal BIOS...", 1);
        uint64_t entry = core.loadInternalBios();
        if (entry == 0) {
            log("[ERROR] Failed to load internal BIOS (returned 0)", 3);
            return;
        }
        log(QString("[BIOS] ✓ BIOS loaded, entry point: 0x%1").arg(entry, 0, 16), 1);

        log("[BIOS] Step 4: Setting up game state...", 1);
        m_entryPoint = entry;
        m_loadedGamePath = "[Internal BIOS]";
        updateGameState(GameState::Loaded, "Internal BIOS");
        log("[BIOS] ✓ Game state updated", 1);

        log("[BIOS] Step 5: Starting emulation...", 1);
        if (core.run()) {
            log("[BIOS] ✓ Emulation started successfully!", 1);
            updateGameState(GameState::Running, "Running");
            startRenderLoop();
        } else {
            log("[ERROR] Failed to start emulation", 3);
            updateGameState(GameState::Error, "Start failed");
        }
        
    } catch (const std::exception& e) {
        log(QString("[EXCEPTION] onBootBios crashed: %1").arg(e.what()), 3);
        updateGameState(GameState::Error, "Exception");
    } catch (...) {
        log("[EXCEPTION] onBootBios crashed with unknown exception!", 3);
        updateGameState(GameState::Error, "Unknown exception");
    }
}

void WeaR_GUI::onOpenSettings() {
    SettingsDialog dialog(this);
    dialog.exec();
}

void WeaR_GUI::onRefreshGames() {
    log("[SCAN] Refreshing game list...", 1);
    scanGameDirectory();
}

void WeaR_GUI::onGameDoubleClicked(int row, int column) {
    (void)column;
    QTableWidgetItem* pathItem = m_gameTable->item(row, 3);
    if (pathItem && !pathItem->text().isEmpty() && !pathItem->text().startsWith("Use ")) {
        loadGameFile(pathItem->text());
    }
}

// =============================================================================
// GAME LOADING
// =============================================================================

void WeaR_GUI::openGameFileDialog() {
    QString filter = "PS4 Executables (*.pkg *.bin *.self *.elf);;All Files (*.*)";
    QString filepath = QFileDialog::getOpenFileName(this, "Select Game", QString(), filter);
    if (!filepath.isEmpty()) {
        loadGameFile(filepath);
    }
}

void WeaR_GUI::loadGameFile(const QString& filepath) {
    log(QString("[LOAD] Loading: %1").arg(filepath), 1);
    updateGameState(GameState::Loading, "Loading...");

    auto& core = getEmulatorCore();
    if (!core.isInitialized()) {
        if (!core.initialize(m_specs)) {
            log("[ERROR] Failed to initialize emulator core", 3);
            updateGameState(GameState::Error, "Init failed");
            return;
        }
    }

    uint64_t entry = core.loadGame(filepath.toStdString());
    if (entry == 0) {
        log("[ERROR] Failed to load game file", 3);
        updateGameState(GameState::Error, "Load failed");
        return;
    }

    m_entryPoint = entry;
    m_loadedGamePath = filepath;
    log(QString("[LOAD] Entry point: 0x%1").arg(entry, 16, 16, QChar('0')).toUpper(), 1);
    updateGameState(GameState::Loaded, QFileInfo(filepath).fileName());

    bootGame();
}

void WeaR_GUI::bootGame() {
    if (m_gameState != GameState::Loaded) return;

    log("[BOOT] Starting emulation...", 1);
    initializeRenderEngine();
    startRenderLoop();

    updateGameState(GameState::Running, QFileInfo(m_loadedGamePath).fileName());
}

void WeaR_GUI::updateGameState(GameState state, const QString& message) {
    m_gameState = state;

    QString stateText;
    switch (state) {
        case GameState::NoGame:   stateText = "Ready"; break;
        case GameState::Loading:  stateText = "Loading..."; break;
        case GameState::Loaded:   stateText = "Loaded"; break;
        case GameState::Running:  stateText = "Running"; break;
        case GameState::Paused:   stateText = "Paused"; break;
        case GameState::Error:    stateText = "Error"; break;
    }

    if (!message.isEmpty()) {
        stateText = message;
    }

    m_stateLabel->setText(stateText);
}

void WeaR_GUI::scanGameDirectory() {
    // Placeholder - scan a games directory
    log("[SCAN] Directory scanning not yet implemented", 2);
}

// =============================================================================
// LOGGING
// =============================================================================

void WeaR_GUI::log(const QString& message, int level) {
    if (!m_logConsole) return;

    QString color;
    switch (level) {
        case 0: color = "#5A5A5A"; break;  // Debug
        case 1: color = "#CCCCCC"; break;  // Info
        case 2: color = "#E67E22"; break;  // Warning
        case 3: color = "#E74C3C"; break;  // Error
        default: color = "#CCCCCC";
    }

    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    m_logConsole->append(QString("<span style='color:#5A5A5A'>[%1]</span> <span style='color:%2'>%3</span>")
        .arg(timestamp, color, message));

    emit logMessage(message, level);
}

// =============================================================================
// INPUT SYSTEM
// =============================================================================

void WeaR_GUI::initializeInputSystem() {
    m_input = std::make_unique<WeaR_Input>();

    // Poll at ~60Hz (16ms) for real-time response
    m_inputTimer = new QTimer(this);
    connect(m_inputTimer, &QTimer::timeout, this, &WeaR_GUI::onInputPoll);
    m_inputTimer->start(16);

    // Immediate first poll to set initial status
    onInputPoll();
}

void WeaR_GUI::onInputPoll() {
    if (!m_input) return;

    // Poll updates controller state internally
    ScePadData padState = m_input->poll();

    bool currentlyConnected = m_input->isControllerConnected();
    
    // Update status bar on EVERY state change
    if (m_controllerConnected != currentlyConnected) {
        m_controllerConnected = currentlyConnected;
        updateControllerStatus();
        if (m_controllerConnected) {
            log("[INPUT] Controller connected", 1);
        } else {
            log("[INPUT] Controller disconnected", 2);
        }
    }
}

void WeaR_GUI::updateControllerStatus() {
    if (m_controllerConnected) {
        m_controllerLabel->setText("Controller: Connected");
        m_controllerLabel->setStyleSheet("color: #4EC9B0;");
    } else {
        m_controllerLabel->setText("Controller: Disconnected");
        m_controllerLabel->setStyleSheet("color: #E74C3C;");
    }
}

// =============================================================================
// RENDER ENGINE
// =============================================================================

void WeaR_GUI::initializeRenderEngine() {
    if (m_engineInitialized) return;

    // Create render widget
    if (!m_renderWidget) {
        m_renderWidget = new QWidget(this);
        m_renderWidget->setMinimumSize(640, 480);
    }

    m_engine = std::make_unique<WeaR_RenderEngine>();

    RenderEngineConfig config;
    config.appName = "WeaR-emu";
    config.windowWidth = static_cast<uint32_t>(m_renderWidget->width());
    config.windowHeight = static_cast<uint32_t>(m_renderWidget->height());
    config.enableValidation = false;

    auto hwnd = reinterpret_cast<HWND>(m_renderWidget->winId());
    auto result = m_engine->initVulkan(m_specs, hwnd, config);

    if (!result) {
        log(QString("[VULKAN] Init failed: %1").arg(QString::fromStdString(result.error())), 3);
        return;
    }

    m_engineInitialized = true;
    log("[VULKAN] Render engine initialized", 1);
}

void WeaR_GUI::startRenderLoop() {
    if (m_renderTimer) return;

    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &WeaR_GUI::onRenderFrame);
    m_renderTimer->start(16);

    m_fpsTimer.start();
    m_frameCount = 0;
}

void WeaR_GUI::stopRenderLoop() {
    if (m_renderTimer) {
        m_renderTimer->stop();
        delete m_renderTimer;
        m_renderTimer = nullptr;
    }
}

void WeaR_GUI::onRenderFrame() {
    if (!m_engineInitialized || !m_engine) return;

    auto result = m_engine->renderFrame();
    if (!result) {
        log(QString("[RENDER] Error: %1").arg(QString::fromStdString(result.error())), 3);
    }

    m_frameCount++;
    updateFPSCounter();
}

void WeaR_GUI::updateFPSCounter() {
    qint64 elapsed = m_fpsTimer.elapsed();
    if (elapsed >= 1000) {
        m_currentFPS = static_cast<float>(m_frameCount) * 1000.0f / elapsed;
        m_fpsLabel->setText(QString("FPS: %1").arg(m_currentFPS, 0, 'f', 1));
        m_frameCount = 0;
        m_fpsTimer.restart();
    }
}

// =============================================================================
// EVENTS
// =============================================================================

void WeaR_GUI::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    log("[GUI] Window ready", 0);
}

void WeaR_GUI::closeEvent(QCloseEvent* event) {
    stopRenderLoop();
    QMainWindow::closeEvent(event);
}

void WeaR_GUI::keyPressEvent(QKeyEvent* event) {
    WeaR_InputManager::get().handleKeyPress(event->key(), true);
    QMainWindow::keyPressEvent(event);
}

void WeaR_GUI::keyReleaseEvent(QKeyEvent* event) {
    WeaR_InputManager::get().handleKeyPress(event->key(), false);
    QMainWindow::keyReleaseEvent(event);
}

} // namespace WeaR
