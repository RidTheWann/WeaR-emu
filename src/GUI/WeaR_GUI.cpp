#include "WeaR_GUI.h"
#include "WeaR_Logger.h"
#include "Graphics/WeaR_RenderEngine.h"
#include "Core/WeaR_System.h"
#include "HLE/WeaR_Syscalls.h"
#include "HLE/FileSystem/WeaR_VFS.h"
#include "Input/WeaR_Input.h"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDockWidget>
#include <QTextEdit>
#include <QScrollBar>
#include <QShowEvent>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QKeyEvent>

#include <format>
#include <iostream>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
// DWM_SYSTEMBACKDROP_TYPE values are in Windows 11 SDK - no custom enum needed
#endif

namespace WeaR {

namespace Colors {
    constexpr QColor NeonGreen{0, 255, 157};
    constexpr QColor Grey{136, 136, 136};
    constexpr QColor Red{255, 68, 68};
}

WeaR_GUI::WeaR_GUI(const WeaR_Specs& specs, QWidget* parent)
    : QMainWindow(parent), m_specs(specs)
    , m_engine(std::make_unique<WeaR_RenderEngine>())
    , m_system(std::make_unique<WeaR_System>())
{
    setWindowTitle("WeaR-emu");
    setMinimumSize(1100, 700);
    resize(1400, 850);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);

    m_frameGenStatus = m_specs.canRunFrameGen ? WeaRGenUIStatus::Active : WeaRGenUIStatus::Unsupported;
    m_accentColor = m_specs.canRunFrameGen ? Colors::NeonGreen : Colors::Grey;

    setupUI();
    setupCpuMonitorDock();
    setupKernelLogDock();
    loadStylesheet();
    enableWindowsAcrylicBlur();

    m_renderTimer = new QTimer(this);
    m_renderTimer->setTimerType(Qt::PreciseTimer);
    connect(m_renderTimer, &QTimer::timeout, this, &WeaR_GUI::onRenderFrame);

    m_cpuMonitorTimer = new QTimer(this);
    m_cpuMonitorTimer->setInterval(60);
    connect(m_cpuMonitorTimer, &QTimer::timeout, this, &WeaR_GUI::onCpuMonitorUpdate);

    // Connect logger to UI
    connect(&getLogger(), &WeaR_Logger::logAdded, this, &WeaR_GUI::onKernelLogUpdate, Qt::QueuedConnection);
}

WeaR_GUI::~WeaR_GUI() {
    stopRenderLoop();
    m_cpuMonitorTimer->stop();
    if (m_system) m_system->shutdown();
    if (m_engine) m_engine->shutdown();
}

void WeaR_GUI::initializeSystem() {
    if (m_systemInitialized) return;
    if (m_system->initialize(m_specs)) {
        m_systemInitialized = true;
        m_system->setRenderer(m_engine.get());
        getSyscallDispatcher().setLogger(&getLogger());
        qDebug() << "[GUI] System initialized";
    }
}

void WeaR_GUI::openGameFileDialog() {
    QString filter = "PS4 Executables (*.elf *.bin *.self);;All Files (*.*)";
    QString filepath = QFileDialog::getOpenFileName(this, "Select Game", QString(), filter);
    if (!filepath.isEmpty()) loadGameFile(filepath);
}

void WeaR_GUI::loadGameFile(const QString& filepath) {
    if (!m_systemInitialized) initializeSystem();
    if (!m_systemInitialized) { updateGameState(GameState::Error, "System init failed"); return; }
    updateGameState(GameState::Loading, "Loading...");
    
    // Auto-mount /app0 to the game's directory
    QFileInfo fileInfo(filepath);
    QString gameDir = fileInfo.absolutePath();
    WeaR_VFS::get().mount("/app0", gameDir.toStdString());
    WeaR_VFS::get().mount("/hostapp", gameDir.toStdString());
    getLogger().log(QString("VFS: Mounted /app0 -> %1").arg(gameDir).toStdString(), LogLevel::Info);
    
    m_entryPoint = m_system->loadGame(filepath.toStdString());
    if (m_entryPoint == 0) { updateGameState(GameState::Error, "Load failed"); return; }
    m_loadedGamePath = filepath;
    updateGameState(GameState::Loaded, QString::fromStdString(std::format("Entry 0x{:X}", m_entryPoint)));
    emit gameLoaded(m_entryPoint);
}

void WeaR_GUI::bootGame() {
    if (!m_systemInitialized || m_entryPoint == 0) return;
    getLogger().log(std::string("Booting game..."), LogLevel::Info);
    m_system->boot();
    updateGameState(GameState::Running, "Running");
    m_cpuMonitorTimer->start();
    if (m_kernelLogDock) m_kernelLogDock->show();
}

void WeaR_GUI::updateGameState(GameState state, const QString& message) {
    m_gameState = state;
    if (m_gameStatusLabel) {
        QString text;
        switch (state) {
            case GameState::NoGame:  text = "No game"; break;
            case GameState::Loading: text = "Loading..."; break;
            case GameState::Loaded:  text = message.isEmpty() ? "Ready" : message; break;
            case GameState::Running: text = "Running"; break;
            case GameState::Paused:  text = "Paused"; break;
            case GameState::Error:   text = "Error: " + message; break;
        }
        m_gameStatusLabel->setText(text);
    }
    if (m_launchGameBtn) {
        switch (state) {
            case GameState::NoGame:  m_launchGameBtn->setText("Load Game"); m_launchGameBtn->setEnabled(true); break;
            case GameState::Loading: m_launchGameBtn->setText("Loading..."); m_launchGameBtn->setEnabled(false); break;
            case GameState::Loaded:  m_launchGameBtn->setText("Boot"); m_launchGameBtn->setEnabled(true); break;
            case GameState::Running: m_launchGameBtn->setText("Pause"); m_launchGameBtn->setEnabled(true); break;
            case GameState::Paused:  m_launchGameBtn->setText("Resume"); break;
            case GameState::Error:   m_launchGameBtn->setText("Retry"); m_launchGameBtn->setEnabled(true); break;
        }
    }
}

void WeaR_GUI::onCpuMonitorUpdate() {
    if (!m_system || !m_systemInitialized) return;
    auto ctx = m_system->getCpuSnapshot();
    auto cpu = m_system->getCpu();
    if (m_cpuRIPLabel) m_cpuRIPLabel->setText(QString("RIP: 0x%1").arg(ctx.RIP, 16, 16, QChar('0')).toUpper());
    if (m_cpuRAXLabel) m_cpuRAXLabel->setText(QString("RAX: 0x%1").arg(ctx.RAX, 16, 16, QChar('0')).toUpper());
    if (m_cpuOpcodeLabel && cpu) m_cpuOpcodeLabel->setText(QString("Opcode: 0x%1").arg(cpu->getLastOpcode(), 2, 16, QChar('0')).toUpper());
    if (m_cpuInsnCountLabel) m_cpuInsnCountLabel->setText(QString("Insns: %1").arg(m_system->getInstructionCount()));
    if (m_cpuStateLabel && cpu) {
        QString s;
        switch (cpu->getState()) {
            case CpuState::Stopped: s = "STOPPED"; break;
            case CpuState::Running: s = "RUNNING"; break;
            case CpuState::Paused:  s = "PAUSED"; break;
            case CpuState::Halted:  s = "HALTED"; break;
            case CpuState::Faulted: s = "FAULT"; break;
        }
        m_cpuStateLabel->setText("State: " + s);
    }
}

void WeaR_GUI::onKernelLogUpdate(const QString& message, int level) {
    (void)level;
    if (m_kernelLogText) {
        m_kernelLogText->append(message);
        QScrollBar* sb = m_kernelLogText->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
}

void WeaR_GUI::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (!m_engineInitialized) QTimer::singleShot(100, this, &WeaR_GUI::initializeRenderEngine);
}

void WeaR_GUI::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (m_engineInitialized && m_engine && m_renderWidget) {
        QSize s = m_renderWidget->size();
        if (s.width() > 0 && s.height() > 0) m_engine->onWindowResize(s.width(), s.height());
    }
}

void WeaR_GUI::closeEvent(QCloseEvent* event) {
    stopRenderLoop();
    m_cpuMonitorTimer->stop();
    if (m_system) m_system->shutdown();
    if (m_engine) { m_engine->shutdown(); m_engineInitialized = false; }
    QMainWindow::closeEvent(event);
}

void WeaR_GUI::initializeRenderEngine() {
    if (m_engineInitialized || !m_engine || !m_renderWidget) return;
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(m_renderWidget->winId());
    if (!hwnd) return;
    QSize s = m_renderWidget->size();
    RenderEngineConfig cfg{}; cfg.windowWidth = s.width(); cfg.windowHeight = s.height();
    auto r = m_engine->initVulkan(m_specs, hwnd, cfg);
    if (!r) { qWarning() << "[GUI] Engine fail:" << QString::fromStdString(r.error()); return; }
    m_engineInitialized = true;
    if (m_engine->isFrameGenActive()) {
        setFrameGenStatus(WeaRGenUIStatus::Active);
        if (m_frameGenToggle) { m_frameGenToggle->setChecked(true); m_frameGenToggle->setText("Disable"); }
    }
    initializeSystem();
    startRenderLoop();
#endif
}

void WeaR_GUI::startRenderLoop() { if (!m_engineInitialized) return; m_fpsTimer.start(); m_frameCount = 0; m_renderTimer->start(0); }
void WeaR_GUI::stopRenderLoop() { if (m_renderTimer) m_renderTimer->stop(); }

void WeaR_GUI::onRenderFrame() {
    if (!m_engineInitialized || !m_engine) return;
    auto r = m_engine->renderFrame();
    if (!r && r.error().find("out of date") != std::string::npos) {
        QSize s = m_renderWidget->size();
        m_engine->onWindowResize(s.width(), s.height());
    }
    m_frameCount++;
    if (m_fpsTimer.elapsed() >= 500) updateFPSCounter();
}

void WeaR_GUI::updateFPSCounter() {
    qint64 e = m_fpsTimer.elapsed();
    if (e > 0) m_currentFPS = m_frameCount * 1000.0f / e;
    m_frameCount = 0; m_fpsTimer.restart();
    if (m_fpsLabel) {
        QString t = QString::fromStdString(std::format("FPS: {:.1f}", m_currentFPS));
        if (m_engine && m_engine->isFrameGenActive()) t += " (WeaR-Gen)";
        m_fpsLabel->setText(t);
    }
}

void WeaR_GUI::setupUI() {
    m_centralWidget = new QWidget(this); m_centralWidget->setObjectName("centralWidget");
    setCentralWidget(m_centralWidget);
    auto* ml = new QVBoxLayout(m_centralWidget); ml->setContentsMargins(0,0,0,0); ml->setSpacing(0);
    setupTitleBar(); ml->addWidget(m_titleBar);

    m_mainContent = new QFrame(); m_mainContent->setObjectName("mainContent");
    auto* cl = new QHBoxLayout(m_mainContent); cl->setContentsMargins(0,0,0,0); cl->setSpacing(0);

    auto* dash = new QFrame(); dash->setObjectName("dashboardPanel"); dash->setFixedWidth(300);
    auto* dl = new QVBoxLayout(dash); dl->setContentsMargins(20,20,20,20); dl->setSpacing(16);

    auto* gpu = new QFrame(); gpu->setObjectName("infoCard");
    auto* gl = new QVBoxLayout(gpu); gl->setContentsMargins(16,14,16,14);
    auto* gh = new QLabel("GPU"); gh->setObjectName("cardHeader"); gl->addWidget(gh);
    m_gpuNameLabel = new QLabel(QString::fromStdString(m_specs.gpuName).left(30));
    m_gpuNameLabel->setObjectName("gpuName"); m_gpuNameLabel->setWordWrap(true); gl->addWidget(m_gpuNameLabel);
    m_gpuInfoLabel = new QLabel(QString::fromStdString(std::format("{} • {:.1f} TFLOPs", m_specs.vramString(), m_specs.estimatedTFLOPs)));
    m_gpuInfoLabel->setObjectName("gpuInfo"); gl->addWidget(m_gpuInfoLabel);
    dl->addWidget(gpu);

    auto* fg = new QFrame(); fg->setObjectName("frameGenCard");
    auto* fl = new QVBoxLayout(fg); fl->setContentsMargins(16,14,16,14);
    auto* fr = new QHBoxLayout();
    m_frameGenIndicator = new QFrame(); m_frameGenIndicator->setObjectName("statusIndicator"); m_frameGenIndicator->setFixedSize(12,12);
    fr->addWidget(m_frameGenIndicator);
    auto* ft = new QLabel("WEAR-GEN"); ft->setObjectName("cardHeader"); fr->addWidget(ft); fr->addStretch();
    fl->addLayout(fr);
    m_frameGenStatusLabel = new QLabel(m_specs.canRunFrameGen ? "Ready" : QString::fromStdString(m_specs.frameGenDisableReason).left(40));
    m_frameGenStatusLabel->setObjectName("frameGenStatus"); m_frameGenStatusLabel->setWordWrap(true); fl->addWidget(m_frameGenStatusLabel);
    m_frameGenToggle = new QPushButton(m_specs.canRunFrameGen ? "Enable" : "N/A");
    m_frameGenToggle->setObjectName("frameGenToggle"); m_frameGenToggle->setCheckable(true); m_frameGenToggle->setEnabled(m_specs.canRunFrameGen);
    connect(m_frameGenToggle, &QPushButton::toggled, this, &WeaR_GUI::onFrameGenToggled);
    fl->addWidget(m_frameGenToggle);
    dl->addWidget(fg);

    dl->addStretch();
    m_launchGameBtn = new QPushButton("Load Game"); m_launchGameBtn->setObjectName("launchBtn"); m_launchGameBtn->setMinimumHeight(44);
    connect(m_launchGameBtn, &QPushButton::clicked, this, &WeaR_GUI::onLaunchGameClicked);
    dl->addWidget(m_launchGameBtn);

    cl->addWidget(dash);
    setupRenderWidget(); cl->addWidget(m_renderWidget, 1);
    ml->addWidget(m_mainContent, 1);
    setupStatusFooter(); ml->addWidget(m_statusFooter);
    updateStatusIndicator();
}

void WeaR_GUI::setupTitleBar() {
    m_titleBar = new QFrame(); m_titleBar->setObjectName("titleBar"); m_titleBar->setFixedHeight(44);
    auto* l = new QHBoxLayout(m_titleBar); l->setContentsMargins(16,0,8,0); l->setSpacing(12);
    m_logoLabel = new QLabel("◆"); m_logoLabel->setObjectName("logoLabel"); l->addWidget(m_logoLabel);
    m_titleLabel = new QLabel("WeaR-emu"); m_titleLabel->setObjectName("titleLabel"); l->addWidget(m_titleLabel);
    auto* sub = new QLabel("PlayStation 4 Emulator"); sub->setObjectName("subtitleLabel"); l->addWidget(sub);
    l->addStretch();
    m_minimizeBtn = new QPushButton("─"); m_minimizeBtn->setObjectName("windowBtn"); m_minimizeBtn->setFixedSize(46,32);
    connect(m_minimizeBtn, &QPushButton::clicked, this, &WeaR_GUI::onMinimizeClicked); l->addWidget(m_minimizeBtn);
    m_maximizeBtn = new QPushButton("□"); m_maximizeBtn->setObjectName("windowBtn"); m_maximizeBtn->setFixedSize(46,32);
    connect(m_maximizeBtn, &QPushButton::clicked, this, &WeaR_GUI::onMaximizeClicked); l->addWidget(m_maximizeBtn);
    m_closeBtn = new QPushButton("✕"); m_closeBtn->setObjectName("closeBtn"); m_closeBtn->setFixedSize(46,32);
    connect(m_closeBtn, &QPushButton::clicked, this, &WeaR_GUI::onCloseClicked); l->addWidget(m_closeBtn);
}

void WeaR_GUI::setupDashboard() {}

void WeaR_GUI::setupRenderWidget() {
    m_renderWidget = new QWidget(); m_renderWidget->setObjectName("renderWidget");
    m_renderWidget->setAttribute(Qt::WA_NativeWindow);
    m_renderWidget->setAttribute(Qt::WA_PaintOnScreen);
    m_renderWidget->setAttribute(Qt::WA_NoSystemBackground);
    m_renderWidget->setMinimumSize(640, 480);
    m_renderWidget->winId();
}

void WeaR_GUI::setupStatusFooter() {
    m_statusFooter = new QFrame(); m_statusFooter->setObjectName("statusFooter"); m_statusFooter->setFixedHeight(32);
    auto* l = new QHBoxLayout(m_statusFooter); l->setContentsMargins(20,0,20,0); l->setSpacing(20);
    auto* v = new QLabel("v0.1.0-alpha"); v->setObjectName("footerLabel"); l->addWidget(v);
    m_gameStatusLabel = new QLabel("No game"); m_gameStatusLabel->setObjectName("gameStatusLabel"); l->addWidget(m_gameStatusLabel);
    l->addStretch();
    m_fpsLabel = new QLabel("FPS: --"); m_fpsLabel->setObjectName("fpsLabel"); l->addWidget(m_fpsLabel);
    m_frameGenModeLabel = new QLabel(m_specs.supportsFloat16 ? "FP16" : "FP32"); m_frameGenModeLabel->setObjectName("footerLabel"); l->addWidget(m_frameGenModeLabel);
}

void WeaR_GUI::setupCpuMonitorDock() {
    m_cpuMonitorDock = new QDockWidget("CPU Monitor", this);
    m_cpuMonitorDock->setObjectName("cpuMonitorDock");
    auto* c = new QFrame(); c->setObjectName("cpuMonitorContent");
    auto* l = new QVBoxLayout(c); l->setContentsMargins(12,12,12,12); l->setSpacing(8);
    auto mkLbl = [&](const QString& t) { auto* lb = new QLabel(t); lb->setStyleSheet("font-family:Consolas;font-size:11px;color:#00ff9d;"); l->addWidget(lb); return lb; };
    m_cpuStateLabel = mkLbl("State: STOPPED");
    m_cpuRIPLabel = mkLbl("RIP: 0x0000000000000000");
    m_cpuRAXLabel = mkLbl("RAX: 0x0000000000000000");
    m_cpuOpcodeLabel = mkLbl("Opcode: 0x00");
    m_cpuInsnCountLabel = mkLbl("Insns: 0");
    l->addStretch();
    m_cpuMonitorDock->setWidget(c);
    addDockWidget(Qt::RightDockWidgetArea, m_cpuMonitorDock);
    m_cpuMonitorDock->hide();
}

void WeaR_GUI::setupKernelLogDock() {
    m_kernelLogDock = new QDockWidget("Kernel Log", this);
    m_kernelLogDock->setObjectName("kernelLogDock");
    m_kernelLogText = new QTextEdit();
    m_kernelLogText->setReadOnly(true);
    m_kernelLogText->setStyleSheet(
        "QTextEdit {"
        "  background-color: #0a0a0a;"
        "  color: #00ff9d;"
        "  font-family: 'Consolas', 'Fira Code', monospace;"
        "  font-size: 11px;"
        "  border: none;"
        "  padding: 8px;"
        "}"
    );
    m_kernelLogText->setPlaceholderText("Kernel output will appear here...");
    m_kernelLogDock->setWidget(m_kernelLogText);
    addDockWidget(Qt::BottomDockWidgetArea, m_kernelLogDock);
    m_kernelLogDock->hide();
}

void WeaR_GUI::updateStatusIndicator() {
    if (!m_frameGenIndicator) return;
    QString c = (m_frameGenStatus == WeaRGenUIStatus::Active) ? "#00ff9d" : (m_frameGenStatus == WeaRGenUIStatus::Unsupported) ? "#888" : "#f44";
    m_frameGenIndicator->setStyleSheet(QString("background-color:%1;border-radius:6px;").arg(c));
    if (m_logoLabel) m_logoLabel->setStyleSheet(QString("color:%1;font-size:22px;font-weight:bold;").arg(c));
}

void WeaR_GUI::setAccentColor(const QColor& c) { if (m_accentColor == c) return; m_accentColor = c; updateStatusIndicator(); emit accentColorChanged(c); }

void WeaR_GUI::setFrameGenStatus(WeaRGenUIStatus s) {
    m_frameGenStatus = s;
    m_accentColor = (s == WeaRGenUIStatus::Active) ? Colors::NeonGreen : (s == WeaRGenUIStatus::Unsupported) ? Colors::Grey : Colors::Red;
    if (m_frameGenStatusLabel) m_frameGenStatusLabel->setText((s == WeaRGenUIStatus::Active) ? "Active" : "Disabled");
    updateStatusIndicator();
}

void WeaR_GUI::loadStylesheet() {
    QFile f(":/styles/DeepKernelDark.qss");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) { setStyleSheet(QString::fromUtf8(f.readAll())); f.close(); }
    updateStatusIndicator();
}

void WeaR_GUI::enableWindowsAcrylicBlur() {
#ifdef Q_OS_WIN
    HWND h = reinterpret_cast<HWND>(winId());
    BOOL d = TRUE; DwmSetWindowAttribute(h, DWMWA_USE_IMMERSIVE_DARK_MODE, &d, sizeof(d));
    auto b = DWMSBT_TRANSIENTWINDOW; DwmSetWindowAttribute(h, DWMWA_SYSTEMBACKDROP_TYPE, &b, sizeof(b));
    MARGINS m = {-1,-1,-1,-1}; DwmExtendFrameIntoClientArea(h, &m);
#endif
}

void WeaR_GUI::mousePressEvent(QMouseEvent* e) { if (e->button() == Qt::LeftButton && m_titleBar->geometry().contains(e->pos())) { m_isDragging = true; m_dragPosition = e->globalPosition().toPoint() - frameGeometry().topLeft(); e->accept(); return; } QMainWindow::mousePressEvent(e); }
void WeaR_GUI::mouseMoveEvent(QMouseEvent* e) { if (m_isDragging && (e->buttons() & Qt::LeftButton)) { move(e->globalPosition().toPoint() - m_dragPosition); e->accept(); return; } QMainWindow::mouseMoveEvent(e); }
void WeaR_GUI::mouseReleaseEvent(QMouseEvent* e) { if (e->button() == Qt::LeftButton) m_isDragging = false; QMainWindow::mouseReleaseEvent(e); }

void WeaR_GUI::keyPressEvent(QKeyEvent* event) {
    // Ignore auto-repeat events
    if (event->isAutoRepeat()) {
        QMainWindow::keyPressEvent(event);
        return;
    }
    
    // Forward to input manager
    WeaR_InputManager::get().handleKeyPress(event->key(), true);
    
    // Handle special keys
    if (event->key() == Qt::Key_Escape) {
        // Toggle pause
        if (m_gameState == GameState::Running) {
            m_system->pause();
            updateGameState(GameState::Paused);
        } else if (m_gameState == GameState::Paused) {
            m_system->resume();
            updateGameState(GameState::Running);
        }
    } else if (event->key() == Qt::Key_F1) {
        // Toggle CPU monitor
        if (m_cpuMonitorDock) m_cpuMonitorDock->setVisible(!m_cpuMonitorDock->isVisible());
    } else if (event->key() == Qt::Key_F2) {
        // Toggle kernel log
        if (m_kernelLogDock) m_kernelLogDock->setVisible(!m_kernelLogDock->isVisible());
    }
    
    QMainWindow::keyPressEvent(event);
}

void WeaR_GUI::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        QMainWindow::keyReleaseEvent(event);
        return;
    }
    
    WeaR_InputManager::get().handleKeyPress(event->key(), false);
    QMainWindow::keyReleaseEvent(event);
}

void WeaR_GUI::onMinimizeClicked() { showMinimized(); }
void WeaR_GUI::onMaximizeClicked() { if (isMaximized()) { showNormal(); m_maximizeBtn->setText("□"); } else { showMaximized(); m_maximizeBtn->setText("❐"); } }
void WeaR_GUI::onCloseClicked() { close(); }

void WeaR_GUI::onLaunchGameClicked() {
    switch (m_gameState) {
        case GameState::NoGame: case GameState::Error: openGameFileDialog(); break;
        case GameState::Loaded: bootGame(); break;
        case GameState::Running: m_system->pause(); updateGameState(GameState::Paused); break;
        case GameState::Paused: m_system->resume(); updateGameState(GameState::Running); break;
        default: break;
    }
}

void WeaR_GUI::onFrameGenToggled(bool c) {
    if (!m_engine || !m_engineInitialized) return;
    if (m_engine->setFrameGenEnabled(c)) { setFrameGenStatus(c ? WeaRGenUIStatus::Active : WeaRGenUIStatus::Disabled); m_frameGenToggle->setText(c ? "Disable" : "Enable"); }
}

} // namespace WeaR
