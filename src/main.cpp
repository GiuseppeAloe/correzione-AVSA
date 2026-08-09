#include <QApplication>
#include "MainWindow.h"
#include <iostream>
#include <cstdio>
#include <windows.h>
#include <QStyleFactory>
#include <QPalette>

int main(int argc, char *argv[])
{
    // MEGA-EARLY LOGGING
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    
    setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);

    // --- DISABLE QUICK EDIT MODE (Prevents blocking on console click) ---
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prev_mode;
    GetConsoleMode(hInput, &prev_mode);
    SetConsoleMode(hInput, prev_mode & ~ENABLE_QUICK_EDIT_MODE);
    // ------------------------------------------------------------------

    std::cout << "DEBUG: main() ENTERED (Console Mode)" << std::endl;
    
    // Dump Env
    const char* path = std::getenv("PATH");
    std::cout << "DEBUG: PATH=" << (path ? path : "NULL") << std::endl;
    const char* qtPlugin = std::getenv("QT_PLUGIN_PATH");
    std::cout << "DEBUG: QT_PLUGIN_PATH=" << (qtPlugin ? qtPlugin : "NULL") << std::endl;

    // Force Unset
    _putenv("QT_PLUGIN_PATH=");
    std::cout << "DEBUG: Cleared QT_PLUGIN_PATH" << std::endl;

    QApplication app(argc, argv);
    std::cout << "DEBUG: QApplication created" << std::endl;

    // --- RESTORE MODERN DARK THEME (Fusion) ---
    app.setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
    // ------------------------------------------

    try {
        std::cout << "DEBUG: Creating MainWindow..." << std::endl;
        MainWindow w;
        std::cout << "DEBUG: MainWindow created. Showing..." << std::endl;
        w.show();
        std::cout << "DEBUG: MainWindow shown. Entering exec()..." << std::endl;
        int ret = app.exec();
        std::cout << "DEBUG: Application finished with code " << ret << std::endl;
        system("pause");
        return ret;
    } catch (const std::exception& e) {
        std::cout << "CRITICAL: Exception in main: " << e.what() << std::endl;
        system("pause");
        return -1;
    } catch (...) {
        std::cout << "CRITICAL: Unknown exception in main" << std::endl;
        system("pause");
        return -1;
    }
}

