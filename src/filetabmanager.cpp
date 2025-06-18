#include "filetabmanager.h"
#include "sqleditor.h"
#include "resultstablemodel.h"
#include "chartmanager.h"
#include "sqlexecutor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTableView>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QAction>
#include <QKeySequence>
#include <QTabWidget>
#include <QFont>
#include <QHeaderView>

FileTabManager::FileTabManager(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_tabWidget(nullptr)
{
    setupUI();
    setupConnections();
}

FileTabManager::~FileTabManager()
{
    // Clean up tab data
    qDeleteAll(m_tabData);
    m_tabData.clear();
}

void FileTabManager::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    
    m_mainLayout->addWidget(m_tabWidget);
}

void FileTabManager::setupConnections()
{
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &FileTabManager::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &FileTabManager::onTabCloseRequested);
}

void FileTabManager::addFileTab(const QString &filePath)
{
    // Check if file is already open
    for (int i = 0; i < m_tabData.size(); ++i) {
        if (m_tabData[i]->filePath == filePath) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }
    
    // Create new tab data
    FileTabData *tabData = new FileTabData();
    tabData->filePath = filePath;
    tabData->fileName = QFileInfo(filePath).fileName();
    tabData->dbManager = std::make_unique<DuckDBManager>();
    tabData->sqlExecutor = std::make_unique<SQLExecutor>(tabData->dbManager.get());
    tabData->resultsModel = std::make_unique<ResultsTableModel>();
    tabData->chartManager = nullptr; // Will be created in createFileTabWidget with proper parent
    
    // Load the file
    if (!tabData->dbManager->loadFile(filePath)) {
        QMessageBox::critical(this, tr("Error"), 
                             tr("Failed to load file: %1").arg(filePath));
        delete tabData;
        return;
    }
    
    // Create the tab widget
    QWidget *tabWidget = createFileTabWidget(tabData);
    
    // Add tab to tab widget
    QString tabTitle = generateTabTitle(filePath);
    int tabIndex = m_tabWidget->addTab(tabWidget, tabTitle);
    
    // Store tab data
    m_tabData.append(tabData);
    
    // Switch to new tab
    m_tabWidget->setCurrentIndex(tabIndex);
    
    emit fileLoaded(filePath);
}

QWidget* FileTabManager::createFileTabWidget(FileTabData *tabData)
{
    QWidget *tabWidget = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout(tabWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // Create main content splitter (left side for query+results, right side for charts)
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Create left side splitter (query + results)
    QSplitter *leftSplitter = new QSplitter(Qt::Vertical);
    
    // Query panel
    QWidget *queryPanel = new QWidget();
    QVBoxLayout *queryLayout = new QVBoxLayout(queryPanel);
    
    QLabel *queryLabel = new QLabel("SQL Query");
    queryLabel->setStyleSheet("font-weight: bold;");
    queryLayout->addWidget(queryLabel);
    
    tabData->sqlEditor = new QTextEdit();
    tabData->sqlEditor->setMaximumHeight(200);
    tabData->sqlEditor->setFont(QFont("Monaco", 11));
    queryLayout->addWidget(tabData->sqlEditor);
    
    // Query buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *executeButton = new QPushButton("Execute Query");
    QPushButton *clearButton = new QPushButton("Clear");
    
    buttonLayout->addWidget(executeButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();
    queryLayout->addLayout(buttonLayout);
    
    // Results panel
    QWidget *resultsPanel = new QWidget();
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsPanel);
    
    QLabel *resultsLabel = new QLabel("Results");
    resultsLabel->setStyleSheet("font-weight: bold;");
    resultsLayout->addWidget(resultsLabel);
    
    // Table filter
    tabData->tableFilterEdit = new QLineEdit();
    tabData->tableFilterEdit->setPlaceholderText("Filter table data...");
    resultsLayout->addWidget(tabData->tableFilterEdit);
    
    // Set up proxy model for sorting and filtering
    tabData->proxyModel = new QSortFilterProxyModel(tabWidget);
    tabData->proxyModel->setSourceModel(tabData->resultsModel.get());
    tabData->proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    tabData->proxyModel->setFilterKeyColumn(-1);
    
    tabData->resultsTableView = new QTableView();
    tabData->resultsTableView->setModel(tabData->proxyModel);
    tabData->resultsTableView->setSortingEnabled(true);
    tabData->resultsTableView->setAlternatingRowColors(true);
    tabData->resultsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tabData->resultsTableView->horizontalHeader()->setStretchLastSection(true);
    resultsLayout->addWidget(tabData->resultsTableView);
    
    // Pagination controls
    QHBoxLayout *paginationLayout = new QHBoxLayout();
    tabData->firstPageButton = new QPushButton("First");
    tabData->prevPageButton = new QPushButton("Previous");
    tabData->pageInfoLabel = new QLabel("Page 0 of 0");
    tabData->nextPageButton = new QPushButton("Next");
    tabData->lastPageButton = new QPushButton("Last");
    tabData->rowCountLabel = new QLabel("0 rows");
    
    paginationLayout->addWidget(tabData->firstPageButton);
    paginationLayout->addWidget(tabData->prevPageButton);
    paginationLayout->addWidget(tabData->pageInfoLabel);
    paginationLayout->addWidget(tabData->nextPageButton);
    paginationLayout->addWidget(tabData->lastPageButton);
    paginationLayout->addStretch();
    paginationLayout->addWidget(tabData->rowCountLabel);
    resultsLayout->addLayout(paginationLayout);
    
    // Add panels to left splitter
    leftSplitter->addWidget(queryPanel);
    leftSplitter->addWidget(resultsPanel);
    leftSplitter->setSizes({200, 400});
    
    // Create chart manager with proper parent (will be deleted when tabWidget is deleted)
    tabData->chartManager = new ChartManager(tabWidget);
    
    // Add left splitter and chart manager to main splitter
    mainSplitter->addWidget(leftSplitter);
    mainSplitter->addWidget(tabData->chartManager);
    mainSplitter->setSizes({600, 400});
    
    mainLayout->addWidget(mainSplitter);
    
    // Connect tab-specific signals
    connect(executeButton, &QPushButton::clicked, [this, tabData]() {
        QString query = tabData->sqlEditor->toPlainText();
        if (query.trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please enter a SQL query."));
            return;
        }
        executeQuery(query);
    });
    
    connect(clearButton, &QPushButton::clicked, [this, tabData]() {
        tabData->sqlEditor->clear();
        tabData->resultsModel->clear();
        tabData->chartManager->clearCharts();
        updatePaginationControls(tabData);
    });
    
    connect(tabData->tableFilterEdit, &QLineEdit::textChanged, 
            tabData->proxyModel, &QSortFilterProxyModel::setFilterFixedString);
    
    connect(tabData->firstPageButton, &QPushButton::clicked, this, &FileTabManager::onFirstPage);
    connect(tabData->prevPageButton, &QPushButton::clicked, this, &FileTabManager::onPreviousPage);
    connect(tabData->nextPageButton, &QPushButton::clicked, this, &FileTabManager::onNextPage);
    connect(tabData->lastPageButton, &QPushButton::clicked, this, &FileTabManager::onLastPage);
    
    // Connect SQLExecutor signals for this tab
    connect(tabData->sqlExecutor.get(), &SQLExecutor::queryExecuted, 
            [this](bool success, const QString &error) {
                emit queryExecuted(success, error);
            });
    
    connect(tabData->sqlExecutor.get(), &SQLExecutor::resultsReady, 
            [this, tabData]() {
                // Update the current tab's results
                auto results = tabData->sqlExecutor->getResults();
                tabData->resultsModel->setResults(results);
                tabData->chartManager->setData(results, tabData->filePath);
                updatePaginationControls(tabData);
                emit resultsReady();
            });
    
    connect(tabData->sqlExecutor.get(), &SQLExecutor::executionProgress, 
            [this](const QString &status) {
                emit executionProgress(status);
            });
    
    // Set up default query
    QString tableName = QFileInfo(tabData->filePath).baseName();
    QString defaultQuery = QString("SELECT * FROM \"%1\" LIMIT 1000;").arg(tableName);
    tabData->sqlEditor->setPlainText(defaultQuery);
    
    updatePaginationControls(tabData);
    
    return tabWidget;
}

void FileTabManager::closeFileTab(int index)
{
    if (index >= 0 && index < m_tabWidget->count() && index < m_tabData.size()) {
        QWidget *widget = m_tabWidget->widget(index);
        m_tabWidget->removeTab(index);
        delete widget;
        
        delete m_tabData[index];
        m_tabData.removeAt(index);
    }
}

FileTabData* FileTabManager::getCurrentTabData()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex >= 0 && currentIndex < m_tabData.size()) {
        return m_tabData[currentIndex];
    }
    return nullptr;
}

int FileTabManager::getCurrentTabIndex() const
{
    return m_tabWidget->currentIndex();
}

int FileTabManager::getTabCount() const
{
    return m_tabWidget->count();
}

void FileTabManager::executeQuery(const QString &query)
{
    FileTabData *tabData = getCurrentTabData();
    if (!tabData) return;
    
    // Use the tab's SQLExecutor to execute the query
    tabData->sqlExecutor->executeQuery(query);
}

void FileTabManager::clearCurrentTab()
{
    FileTabData *tabData = getCurrentTabData();
    if (!tabData) return;
    
    tabData->sqlEditor->clear();
    tabData->resultsModel->clear();
    tabData->chartManager->clearCharts();
    updatePaginationControls(tabData);
}

void FileTabManager::onTabChanged(int index)
{
    if (index >= 0 && index < m_tabData.size()) {
        emit tabChanged(m_tabData[index]->filePath);
    }
}

void FileTabManager::onTabCloseRequested(int index)
{
    closeFileTab(index);
}

void FileTabManager::onFirstPage()
{
    FileTabData *tabData = getCurrentTabData();
    if (tabData) {
        tabData->resultsModel->setCurrentPage(0);
        updatePaginationControls(tabData);
    }
}

void FileTabManager::onPreviousPage()
{
    FileTabData *tabData = getCurrentTabData();
    if (tabData) {
        int currentPage = tabData->resultsModel->getCurrentPage();
        if (currentPage > 0) {
            tabData->resultsModel->setCurrentPage(currentPage - 1);
            updatePaginationControls(tabData);
        }
    }
}

void FileTabManager::onNextPage()
{
    FileTabData *tabData = getCurrentTabData();
    if (tabData) {
        int currentPage = tabData->resultsModel->getCurrentPage();
        int totalPages = tabData->resultsModel->getTotalPages();
        if (currentPage < totalPages - 1) {
            tabData->resultsModel->setCurrentPage(currentPage + 1);
            updatePaginationControls(tabData);
        }
    }
}

void FileTabManager::onLastPage()
{
    FileTabData *tabData = getCurrentTabData();
    if (tabData) {
        int totalPages = tabData->resultsModel->getTotalPages();
        if (totalPages > 0) {
            tabData->resultsModel->setCurrentPage(totalPages - 1);
            updatePaginationControls(tabData);
        }
    }
}

void FileTabManager::updatePaginationControls(FileTabData *tabData)
{
    if (!tabData) return;
    
    int currentPage = tabData->resultsModel->getCurrentPage();
    int totalPages = tabData->resultsModel->getTotalPages();
    int totalRows = tabData->resultsModel->getTotalRows();
    
    tabData->firstPageButton->setEnabled(currentPage > 0);
    tabData->prevPageButton->setEnabled(currentPage > 0);
    tabData->nextPageButton->setEnabled(currentPage < totalPages - 1);
    tabData->lastPageButton->setEnabled(currentPage < totalPages - 1);
    
    if (totalPages > 0) {
        tabData->pageInfoLabel->setText(tr("Page %1 of %2").arg(currentPage + 1).arg(totalPages));
    } else {
        tabData->pageInfoLabel->setText("Page 0 of 0");
    }
    
    tabData->rowCountLabel->setText(tr("%1 rows").arg(totalRows));
}

QString FileTabManager::generateTabTitle(const QString &filePath)
{
    QString fileName = QFileInfo(filePath).baseName();
    return fileName.length() > 15 ? fileName.left(12) + "..." : fileName;
}