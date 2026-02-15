#ifndef FILETABMANAGER_H
#define FILETABMANAGER_H

#include <QWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <memory>
#include "duckdbmanager.h"

class SQLEditor;
class ResultsTableModel;
class ChartManager;
class QSortFilterProxyModel;

class SQLExecutor;

struct FileTabData {
    QString filePath;
    QString fileName;
    std::unique_ptr<DuckDBManager> dbManager;
    std::unique_ptr<SQLExecutor> sqlExecutor;
    std::unique_ptr<ResultsTableModel> resultsModel;
    ChartManager *chartManager; // Qt widget - managed by Qt parent/child system
    QSortFilterProxyModel *proxyModel;
    QTextEdit *sqlEditor;
    QTableView *resultsTableView;
    QLineEdit *tableFilterEdit;
    QPushButton *cancelQueryButton;
    QPushButton *firstPageButton;
    QPushButton *prevPageButton;
    QPushButton *nextPageButton;
    QPushButton *lastPageButton;
    QLabel *pageInfoLabel;
    QLabel *rowCountLabel;
    
    // Destructor: chartManager will be deleted by Qt's parent-child system
    // when the tab widget is deleted, so we don't need to manually delete it
    ~FileTabData() = default;
};

class FileTabManager : public QWidget
{
    Q_OBJECT

public:
    explicit FileTabManager(QWidget *parent = nullptr);
    ~FileTabManager();

    void addFileTab(const QString &filePath);
    void closeFileTab(int index);
    FileTabData* getCurrentTabData();
    int getCurrentTabIndex() const;
    int getTabCount() const;
    void setCurrentTabIndex(int index);
    
    void executeQuery(const QString &query);
    void cancelCurrentQuery();
    void clearCurrentTab();

public slots:
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onFirstPage();
    void onPreviousPage();
    void onNextPage();
    void onLastPage();

signals:
    void tabChanged(const QString &filePath);
    void fileLoaded(const QString &filePath);
    void queryExecuted(bool success, const QString &error);
    void resultsReady();
    void executionProgress(const QString &status);

private:
    void setupUI();
    void setupConnections();
    QWidget* createFileTabWidget(FileTabData *tabData);
    void updatePaginationControls(FileTabData *tabData);
    void updateStatusForTab(FileTabData *tabData);
    QString generateTabTitle(const QString &filePath);

    QVBoxLayout *m_mainLayout;
    QTabWidget *m_tabWidget;
    QList<FileTabData*> m_tabData;

    static constexpr int ROWS_PER_PAGE = 1000;
};

#endif // FILETABMANAGER_H
