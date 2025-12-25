#pragma once

/**
 * @file WeaR_GUI.h
 * @brief RPCS3-Style GUI: Toolbar + Game Table + Log Dock
 */

#include "Hardware/HardwareDetector.h"

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QToolBar;
class QTableWidget;
class QDockWidget;
class QTextEdit;
class QAction;
QT_END_NAMESPACE

namespace WeaR {

class WeaR_RenderEngine;
class WeaR_System;
class WeaR_Input;

enum class GameState : uint8_t { NoGame, Loading, Loaded, Running, Paused, Error };

/**
 * @brief RPCS3-Style GUI: Toolbar on top, Game table center, Log dock bottom
 */
class WeaR_GUI : public QMainWindow {
    Q_OBJECT

public:
    explicit WeaR_GUI(const WeaR_Specs& specs, QWidget* parent = nullptr);
    ~WeaR_GUI() override;

signals:
    void launchGameRequested();
    void gameLoaded(uint64_t entryPoint);
    void logMessage(const QString& message, int level);

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onLoadGame();
    void onBootBios();
    void onOpenSettings();
    void onRefreshGames();
    void onGameDoubleClicked(int row, int column);
    void onRenderFrame();
    void onInputPoll();

private:
    // Setup
    void setupUI();
    void setupToolbar();
    void setupGameTable();
    void setupLogDock();
    void setupStatusBar();
    void applyStylesheet();

    // Game management
    void openGameFileDialog();
    void loadGameFile(const QString& filepath);
    void bootGame();
    void updateGameState(GameState state, const QString& message = "");
    void scanGameDirectory();

    // Logging
    void log(const QString& message, int level = 1);

    // Systems
    void initializeInputSystem();
    void initializeRenderEngine();
    void startRenderLoop();
    void stopRenderLoop();
    void updateControllerStatus();
    void updateFPSCounter();

    // Data
    WeaR_Specs m_specs;
    std::unique_ptr<WeaR_RenderEngine> m_engine;
    std::unique_ptr<WeaR_System> m_system;
    std::unique_ptr<WeaR_Input> m_input;

    GameState m_gameState = GameState::NoGame;
    uint64_t m_entryPoint = 0;
    QString m_loadedGamePath;

    // Timers
    QTimer* m_renderTimer = nullptr;
    QTimer* m_inputTimer = nullptr;
    QElapsedTimer m_fpsTimer;
    uint32_t m_frameCount = 0;
    float m_currentFPS = 0.0f;
    bool m_engineInitialized = false;
    bool m_controllerConnected = false;

    // UI Elements
    QToolBar* m_toolbar = nullptr;
    QTableWidget* m_gameTable = nullptr;
    QDockWidget* m_logDock = nullptr;
    QTextEdit* m_logConsole = nullptr;
    QWidget* m_renderWidget = nullptr;

    // Toolbar Actions
    QAction* m_loadAction = nullptr;
    QAction* m_biosAction = nullptr;
    QAction* m_settingsAction = nullptr;
    QAction* m_refreshAction = nullptr;

    // Status Bar
    QLabel* m_controllerLabel = nullptr;
    QLabel* m_fpsLabel = nullptr;
    QLabel* m_stateLabel = nullptr;
};

} // namespace WeaR
