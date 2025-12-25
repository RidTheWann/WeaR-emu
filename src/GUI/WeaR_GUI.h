#pragma once

/**
 * @file WeaR_GUI.h
 * @brief Custom Qt GUI with full emulator integration
 */

#include "Hardware/HardwareDetector.h"

#include <QMainWindow>
#include <QPropertyAnimation>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QFrame;
class QWidget;
class QDockWidget;
class QTextEdit;
QT_END_NAMESPACE

namespace WeaR {

class WeaR_RenderEngine;
class WeaR_System;
class WeaR_Logger;

enum class WeaRGenUIStatus : uint8_t { Active, Unsupported, Disabled };
enum class GameState : uint8_t { NoGame, Loading, Loaded, Running, Paused, Error };

class WeaR_GUI : public QMainWindow {
    Q_OBJECT
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

public:
    explicit WeaR_GUI(const WeaR_Specs& specs, QWidget* parent = nullptr);
    ~WeaR_GUI() override;

    [[nodiscard]] QColor accentColor() const { return m_accentColor; }
    void setAccentColor(const QColor& color);
    void setFrameGenStatus(WeaRGenUIStatus status);

signals:
    void accentColorChanged(const QColor& color);
    void launchGameRequested();
    void gameLoaded(uint64_t entryPoint);

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();
    void onLaunchGameClicked();
    void onRenderFrame();
    void onFrameGenToggled(bool checked);
    void onCpuMonitorUpdate();
    void onKernelLogUpdate(const QString& message, int level);

private:
    void setupUI();
    void setupTitleBar();
    void setupDashboard();
    void setupRenderWidget();
    void setupStatusFooter();
    void setupCpuMonitorDock();
    void setupKernelLogDock();
    void loadStylesheet();
    void enableWindowsAcrylicBlur();
    void updateStatusIndicator();
    void initializeRenderEngine();
    void initializeSystem();
    void startRenderLoop();
    void stopRenderLoop();
    void updateFPSCounter();
    void updateGameState(GameState state, const QString& message = "");
    void openGameFileDialog();
    void loadGameFile(const QString& filepath);
    void bootGame();

    WeaR_Specs m_specs;
    std::unique_ptr<WeaR_RenderEngine> m_engine;
    std::unique_ptr<WeaR_System> m_system;
    
    GameState m_gameState = GameState::NoGame;
    uint64_t m_entryPoint = 0;
    QString m_loadedGamePath;

    QTimer* m_renderTimer = nullptr;
    QTimer* m_cpuMonitorTimer = nullptr;
    QElapsedTimer m_fpsTimer;
    uint32_t m_frameCount = 0;
    float m_currentFPS = 0.0f;
    bool m_engineInitialized = false;
    bool m_systemInitialized = false;

    QWidget* m_centralWidget = nullptr;
    QFrame* m_titleBar = nullptr;
    QFrame* m_mainContent = nullptr;
    QWidget* m_renderWidget = nullptr;
    QFrame* m_statusFooter = nullptr;
    QDockWidget* m_cpuMonitorDock = nullptr;
    QDockWidget* m_kernelLogDock = nullptr;
    QTextEdit* m_kernelLogText = nullptr;

    QLabel* m_logoLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_minimizeBtn = nullptr;
    QPushButton* m_maximizeBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;

    QLabel* m_gpuNameLabel = nullptr;
    QLabel* m_gpuInfoLabel = nullptr;
    QLabel* m_frameGenStatusLabel = nullptr;
    QFrame* m_frameGenIndicator = nullptr;
    QPushButton* m_launchGameBtn = nullptr;
    QPushButton* m_frameGenToggle = nullptr;

    QLabel* m_cpuRIPLabel = nullptr;
    QLabel* m_cpuRAXLabel = nullptr;
    QLabel* m_cpuOpcodeLabel = nullptr;
    QLabel* m_cpuInsnCountLabel = nullptr;
    QLabel* m_cpuStateLabel = nullptr;

    QLabel* m_fpsLabel = nullptr;
    QLabel* m_gameStatusLabel = nullptr;
    QLabel* m_frameGenModeLabel = nullptr;

    WeaRGenUIStatus m_frameGenStatus = WeaRGenUIStatus::Unsupported;
    QColor m_accentColor{0, 255, 157};
    QPoint m_dragPosition;
    bool m_isDragging = false;
    std::unique_ptr<QPropertyAnimation> m_accentAnimation;
};

} // namespace WeaR
