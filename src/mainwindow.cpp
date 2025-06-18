#include "mainwindow.h"
#include "filebrowser.h"
#include "filetabmanager.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QSplitter>
#include <QHeaderView>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QPushButton>
#include <QTableView>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_fileBrowser(std::make_unique<FileBrowser>())
    , m_fileTabManager(std::make_unique<FileTabManager>())
    , m_updateTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    setupKeyboardShortcuts();
    
    statusLabel->setText("Ready - Select a file to begin");
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("ParquetSQL - Multi-File Data Browser");
    resize(1400, 900);
    
    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main horizontal splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    
    // Create left panel for file browser
    QWidget *leftPanel = new QWidget();
    leftPanel->setMinimumWidth(250);
    leftPanel->setMaximumWidth(400);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    QLabel *filesLabel = new QLabel("File Browser");
    filesLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(filesLabel);
    
    // Add file filter
    fileFilterEdit = new QLineEdit();
    fileFilterEdit->setPlaceholderText("Filter files...");
    leftLayout->addWidget(fileFilterEdit);
    
    fileTreeView = new QTreeView();
    fileTreeView->setModel(m_fileBrowser->getModel());
    fileTreeView->setRootIndex(m_fileBrowser->getRootIndex());
    fileTreeView->hideColumn(1); // Size
    fileTreeView->hideColumn(2); // Type  
    fileTreeView->hideColumn(3); // Date Modified
    fileTreeView->setExpandsOnDoubleClick(false); // Prevent expansion on double-click
    leftLayout->addWidget(fileTreeView);
    
    loadFileButton = new QPushButton("Load Selected File");
    loadFileButton->setStyleSheet("QPushButton { padding: 8px; font-weight: bold; }");
    leftLayout->addWidget(loadFileButton);
    
    // Status label
    statusLabel = new QLabel("Ready - Select a file to begin");
    statusLabel->setStyleSheet("QLabel { padding: 5px; background-color: #f0f0f0; border: 1px solid #ccc; border-radius: 3px; }");
    statusLabel->setWordWrap(true);
    leftLayout->addWidget(statusLabel);
    
    // Add left panel and file tab manager to main splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(m_fileTabManager.get());
    
    // Set initial splitter proportions (25% for file browser, 75% for tabs)
    mainSplitter->setSizes({300, 1100});
    mainSplitter->setCollapsible(0, false); // Don't allow file browser to be collapsed completely
    mainSplitter->setCollapsible(1, false); // Don't allow main area to be collapsed completely
    
    // Set up main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(mainSplitter);
}

void MainWindow::setupConnections()
{
    // File browser connections
    connect(loadFileButton, &QPushButton::clicked, this, &MainWindow::onLoadFileClicked);
    connect(fileFilterEdit, &QLineEdit::textChanged, this, &MainWindow::onFileFilterChanged);
    
    connect(fileTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            [this]() {
                auto indexes = fileTreeView->selectionModel()->selectedIndexes();
                if (!indexes.isEmpty()) {
                    QString filePath = m_fileBrowser->getModel()->filePath(indexes.first());
                    onFileSelected(filePath);
                }
            });
    
    // File tab manager connections
    connect(m_fileTabManager.get(), &FileTabManager::tabChanged, this, &MainWindow::onFileTabChanged);
    connect(m_fileTabManager.get(), &FileTabManager::queryExecuted, this, &MainWindow::onQueryExecuted);
    connect(m_fileTabManager.get(), &FileTabManager::resultsReady, this, &MainWindow::onResultsReady);
    connect(m_fileTabManager.get(), &FileTabManager::executionProgress, this, &MainWindow::onExecutionProgress);
}

void MainWindow::setupKeyboardShortcuts()
{
    // File operations
    QAction *openFileAction = new QAction(this);
    openFileAction->setShortcut(QKeySequence::Open);
    connect(openFileAction, &QAction::triggered, this, &MainWindow::onLoadFileClicked);
    addAction(openFileAction);
    
    // Tab operations
    QAction *closeTabAction = new QAction(this);
    closeTabAction->setShortcut(QKeySequence::Close);
    connect(closeTabAction, &QAction::triggered, [this]() {
        int currentIndex = m_fileTabManager->getCurrentTabIndex();
        if (currentIndex >= 0) {
            m_fileTabManager->closeFileTab(currentIndex);
        }
    });
    addAction(closeTabAction);
    
    // Tab navigation
    QAction *nextTabAction = new QAction(this);
    nextTabAction->setShortcut(QKeySequence::NextChild);
    connect(nextTabAction, &QAction::triggered, [this]() {
        int current = m_fileTabManager->getCurrentTabIndex();
        int count = m_fileTabManager->getTabCount();
        if (count > 1) {
            int next = (current + 1) % count;
            // Note: QTabWidget doesn't expose setCurrentIndex directly, 
            // but FileTabManager can handle this internally
        }
    });
    addAction(nextTabAction);
    
    QAction *prevTabAction = new QAction(this);
    prevTabAction->setShortcut(QKeySequence::PreviousChild);
    connect(prevTabAction, &QAction::triggered, [this]() {
        int current = m_fileTabManager->getCurrentTabIndex();
        int count = m_fileTabManager->getTabCount();
        if (count > 1) {
            int prev = (current - 1 + count) % count;
            // Note: QTabWidget doesn't expose setCurrentIndex directly,
            // but FileTabManager can handle this internally
        }
    });
    addAction(prevTabAction);
    
    // Query execution
    QAction *executeQueryAction = new QAction(this);
    executeQueryAction->setShortcut(QKeySequence("Ctrl+Return"));
    connect(executeQueryAction, &QAction::triggered, [this]() {
        // Execute query in current tab
        auto *tabData = m_fileTabManager->getCurrentTabData();
        if (tabData && tabData->sqlEditor) {
            QString query = tabData->sqlEditor->toPlainText();
            if (!query.trimmed().isEmpty()) {
                m_fileTabManager->executeQuery(query);
            }
        }
    });
    addAction(executeQueryAction);
    
    // Clear current editor
    QAction *clearAction = new QAction(this);
    clearAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    connect(clearAction, &QAction::triggered, [this]() {
        m_fileTabManager->clearCurrentTab();
    });
    addAction(clearAction);
    
    // Focus file filter
    QAction *focusFilterAction = new QAction(this);
    focusFilterAction->setShortcut(QKeySequence("Ctrl+F"));
    connect(focusFilterAction, &QAction::triggered, [this]() {
        fileFilterEdit->setFocus();
        fileFilterEdit->selectAll();
    });
    addAction(focusFilterAction);
    
    // Refresh file tree
    QAction *refreshAction = new QAction(this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, [this]() {
        QString currentPath = m_fileBrowser->getCurrentPath();
        m_fileBrowser->setRootPath(currentPath);
    });
    addAction(refreshAction);
}

void MainWindow::onLoadFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Parquet or CSV File"),
        QString(),
        tr("Data Files (*.parquet *.csv *.tsv);;All Files (*)")
    );
    
    if (!fileName.isEmpty()) {
        m_fileTabManager->addFileTab(fileName);
        statusLabel->setText(tr("Loaded file: %1").arg(QFileInfo(fileName).fileName()));
    }
}

void MainWindow::onFileSelected(const QString &filePath)
{
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        return;
    }
    
    // Check if it's a directory - if so, ignore the selection
    QFileInfo fileInfo(filePath);
    if (fileInfo.isDir()) {
        return;
    }
    
    // Check if it's a supported file type
    QString suffix = fileInfo.suffix().toLower();
    if (suffix != "parquet" && suffix != "csv" && suffix != "tsv") {
        statusLabel->setText(tr("Unsupported file type: %1").arg(suffix.toUpper()));
        return;
    }
    
    // Add file to tab manager
    m_fileTabManager->addFileTab(filePath);
    
    QString fileName = fileInfo.fileName();
    QString fileType = suffix.toUpper();
    statusLabel->setText(tr("%1 file selected: %2").arg(fileType).arg(fileName));
}

void MainWindow::onFileTabChanged(const QString &filePath)
{
    QString fileName = QFileInfo(filePath).fileName();
    statusLabel->setText(tr("Active file: %1").arg(fileName));
    m_currentFilePath = filePath;
}

void MainWindow::onQueryExecuted(bool success, const QString &error)
{
    if (!success) {
        statusLabel->setText("Query failed");
        QMessageBox::critical(this, tr("Query Error"), error);
        return;
    }
    
    statusLabel->setText("Query completed successfully");
}

void MainWindow::onResultsReady()
{
    statusLabel->setText("Results updated");
}

void MainWindow::onExecutionProgress(const QString &status)
{
    statusLabel->setText(status);
}

void MainWindow::onFileFilterChanged(const QString &filter)
{
    QStringList nameFilters;
    if (filter.isEmpty()) {
        nameFilters << "*.parquet" << "*.csv" << "*.tsv";
    } else {
        // Create filters that include the user's filter text
        nameFilters << QString("*%1*.parquet").arg(filter)
                    << QString("*%1*.csv").arg(filter)
                    << QString("*%1*.tsv").arg(filter);
    }
    m_fileBrowser->getModel()->setNameFilters(nameFilters);
}