#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QThread>
#include "mainwindow.h"

void setupApplication()
{
    QApplication::setApplicationName("ParquetSQL");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("ParquetSQL");
    QApplication::setOrganizationDomain("parquetsql.org");
    
    // High DPI scaling is enabled by default in Qt6
    
    QThread::idealThreadCount();
}

void setupStyle(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));
    
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
    
    app.setStyleSheet(R"(
        QMainWindow {
            background-color: #353535;
        }
        QTextEdit {
            background-color: #2b2b2b;
            color: white;
            border: 1px solid #555;
            selection-background-color: #2a82da;
        }
        QTableView {
            background-color: #2b2b2b;
            alternate-background-color: #404040;
            color: white;
            gridline-color: #555;
            selection-background-color: #2a82da;
        }
        QHeaderView::section {
            background-color: #404040;
            color: white;
            border: 1px solid #555;
            padding: 4px;
        }
        QTreeView {
            background-color: #2b2b2b;
            color: white;
            selection-background-color: #2a82da;
        }
        QPushButton {
            background-color: #404040;
            color: white;
            border: 1px solid #555;
            padding: 6px 12px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
        }
        QPushButton:pressed {
            background-color: #2a82da;
        }
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #666;
        }
        QLabel {
            color: white;
        }
        QSplitter::handle {
            background-color: #555;
        }
        QScrollBar:vertical {
            background: #2b2b2b;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background: #555;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover {
            background: #666;
        }
    )");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    setupApplication();
    setupStyle(app);
    
    QSplashScreen splash;
    splash.showMessage("Loading ParquetSQL...", Qt::AlignBottom | Qt::AlignCenter, Qt::white);
    splash.show();
    app.processEvents();
    
    MainWindow window;
    
    QTimer::singleShot(1000, [&]() {
        splash.close();
        window.show();
        window.raise();
        window.activateWindow();
    });
    
    return app.exec();
}