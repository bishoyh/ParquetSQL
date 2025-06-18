#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QTreeView;
class QTableView;
class QTextEdit;
class QPushButton;
class QLineEdit;
class QSortFilterProxyModel;
QT_END_NAMESPACE

class DuckDBManager;
class SQLExecutor;
class ResultsTableModel;
class FileBrowser;
class SQLEditor;
class ChartManager;
class FileTabManager;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoadFileClicked();
    void onFileSelected(const QString &filePath);
    void onFileFilterChanged(const QString &filter);
    void onFileTabChanged(const QString &filePath);
    void onQueryExecuted(bool success, const QString &error);
    void onResultsReady();
    void onExecutionProgress(const QString &status);

private:
    void setupUI();
    void setupConnections();
    void setupKeyboardShortcuts();

    // UI elements - we'll create them manually instead of using .ui file
    QLineEdit *fileFilterEdit;
    QTreeView *fileTreeView;
    QPushButton *loadFileButton;
    QLabel *statusLabel;
    std::unique_ptr<FileBrowser> m_fileBrowser;
    std::unique_ptr<FileTabManager> m_fileTabManager;
    
    QString m_currentFilePath;
    QTimer *m_updateTimer;
    
    static constexpr int ROWS_PER_PAGE = 1000;
};

#endif // MAINWINDOW_H