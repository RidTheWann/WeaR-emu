/**
 * @file main.cpp
 * @brief WeaR-emu entry point
 * 
 * Initialization sequence:
 * 1. Hardware capability detection (standalone Vulkan probe)
 * 2. Qt application bootstrap
 * 3. GUI initialization with detected specs
 */

#include "Hardware/HardwareDetector.h"
#include "GUI/WeaR_GUI.h"

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QFontDatabase>

#include <iostream>
#include <format>

namespace {

void printBanner() {
    std::cout << R"(
 ╔═══════════════════════════════════════════════════════════════╗
 ║                                                               ║
 ║   ██╗    ██╗███████╗ █████╗ ██████╗       ███████╗███╗   ███╗ ║
 ║   ██║    ██║██╔════╝██╔══██╗██╔══██╗      ██╔════╝████╗ ████║ ║
 ║   ██║ █╗ ██║█████╗  ███████║██████╔╝█████╗█████╗  ██╔████╔██║ ║
 ║   ██║███╗██║██╔══╝  ██╔══██║██╔══██╗╚════╝██╔══╝  ██║╚██╔╝██║ ║
 ║   ╚███╔███╔╝███████╗██║  ██║██║  ██║      ███████╗██║ ╚═╝ ██║ ║
 ║    ╚══╝╚══╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝      ╚══════╝╚═╝     ╚═╝ ║
 ║                                                               ║
 ║           Next-Generation PlayStation 4 Emulator              ║
 ║                      Version 0.1.0-alpha                      ║
 ║                                                               ║
 ╚═══════════════════════════════════════════════════════════════╝
)" << std::endl;
}

void printHardwareReport(const WeaR::WeaR_Specs& specs) {
    std::cout << "\n┌─────────────────────────────────────────────────────────────┐\n";
    std::cout << "│                    HARDWARE DETECTION                       │\n";
    std::cout << "├─────────────────────────────────────────────────────────────┤\n";
    std::cout << std::format("│  GPU:       {:<48} │\n", specs.gpuName.substr(0, 48));
    std::cout << std::format("│  Driver:    {:<48} │\n", specs.driverVersion);
    std::cout << std::format("│  VRAM:      {:<48} │\n", specs.vramString());
    std::cout << std::format("│  Tier:      {:<48} │\n", specs.tierString());
    std::cout << std::format("│  TFLOPs:    {:<48.1f} │\n", specs.estimatedTFLOPs);
    std::cout << std::format("│  FP16:      {:<48} │\n", specs.supportsFloat16 ? "Supported" : "Not Supported");
    std::cout << "├─────────────────────────────────────────────────────────────┤\n";
    
    if (specs.canRunFrameGen) {
        std::cout << "│  WeaR-Gen:  \033[32mACTIVE\033[0m - Frame Generation Ready               │\n";
    } else {
        std::cout << std::format("│  WeaR-Gen:  \033[33mDISABLED\033[0m                                       │\n");
        std::cout << std::format("│  Reason:    {:<48} │\n", specs.frameGenDisableReason.substr(0, 48));
    }
    
    std::cout << "└─────────────────────────────────────────────────────────────┘\n\n";
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    printBanner();

    // =========================================================================
    // PHASE 1: Hardware Detection (Before Qt)
    // =========================================================================
    std::cout << "[WeaR] Phase 1: Detecting hardware capabilities...\n";

    auto specsResult = WeaR::HardwareDetector::detectCapabilities();

    if (!specsResult) {
        // Vulkan detection failed - show error dialog
        std::cerr << "[WeaR] ERROR: " << specsResult.error() << "\n";

        QApplication tempApp(argc, argv);
        QMessageBox::critical(nullptr, "WeaR-emu - Hardware Detection Failed",
            QString::fromStdString(std::format(
                "Failed to detect GPU capabilities:\n\n{}\n\n"
                "Please ensure you have:\n"
                "• A Vulkan 1.3 compatible GPU\n"
                "• Up-to-date graphics drivers\n"
                "• Vulkan Runtime installed",
                specsResult.error()
            )));
        return 1;
    }

    WeaR::WeaR_Specs specs = specsResult.value();
    printHardwareReport(specs);

    // === CRITICAL: Low-spec device warning ===
    if (!specs.canRunFrameGen) {
        std::cout << "\033[33m[WeaR] Low-Spec Device Detected: WeaR-Gen Pipeline Disabled.\033[0m\n";
        std::cout << "[WeaR] Reason: " << specs.frameGenDisableReason << "\n";
        std::cout << "[WeaR] The emulator will run in compatibility mode.\n\n";
    }

    // =========================================================================
    // PHASE 2: Qt Application Initialization
    // =========================================================================
    std::cout << "[WeaR] Phase 2: Initializing Qt application...\n";

    QApplication app(argc, argv);
    
    // Application metadata
    app.setApplicationName("WeaR-emu");
    app.setApplicationVersion("0.1.0-alpha");
    app.setOrganizationName("WeaR Team");
    
    // Use Fusion as base (will be heavily styled by QSS)
    app.setStyle("Fusion");

    // High DPI support
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // =========================================================================
    // PHASE 3: Create Main Window
    // =========================================================================
    std::cout << "[WeaR] Phase 3: Creating main window...\n";

    WeaR::WeaR_GUI mainWindow(specs);
    mainWindow.show();

    std::cout << "[WeaR] Initialization complete. Entering event loop.\n\n";

    // =========================================================================
    // PHASE 4: Event Loop
    // =========================================================================
    int result = app.exec();

    // =========================================================================
    // PHASE 5: Cleanup
    // =========================================================================
    std::cout << "\n[WeaR] Shutting down...\n";
    std::cout << "[WeaR] Goodbye!\n";

    return result;
}
