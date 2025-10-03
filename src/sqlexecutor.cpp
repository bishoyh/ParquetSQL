#include "sqlexecutor.h"
#include <QDebug>
#include <QMutexLocker>

SQLExecutorWorker::SQLExecutorWorker(DuckDBManager *dbManager)
    : QObject(nullptr)
    , m_dbManager(dbManager)
{
}

void SQLExecutorWorker::executeQuery(const QString &query)
{
    try {
        if (!m_dbManager) {
            emit queryFinished(false, "Database manager not available", DuckDBManager::QueryResult());
            return;
        }

        DuckDBManager::QueryResult result = m_dbManager->executeQuery(query);
        emit queryFinished(result.success, result.error, result);
    } catch (const std::exception &e) {
        qCritical() << "SQLExecutorWorker exception:" << e.what();
        emit queryFinished(false, QString("Worker exception: %1").arg(e.what()), DuckDBManager::QueryResult());
    } catch (...) {
        qCritical() << "SQLExecutorWorker unknown exception";
        emit queryFinished(false, "Worker unknown exception", DuckDBManager::QueryResult());
    }
}

SQLExecutor::SQLExecutor(DuckDBManager *dbManager, QObject *parent)
    : QObject(parent)
    , m_dbManager(dbManager)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_isExecuting(false)
    , m_shouldCancel(false)
{
    startWorkerThread();
}

SQLExecutor::~SQLExecutor()
{
    stopWorkerThread();
}

void SQLExecutor::startWorkerThread()
{
    if (m_workerThread) {
        return;
    }
    
    m_workerThread = new QThread(this);
    m_worker = new SQLExecutorWorker(m_dbManager);
    m_worker->moveToThread(m_workerThread);
    
    connect(m_workerThread, &QThread::started,
            m_worker, [this]() {
                emit executionProgress("Worker thread started");
            });
    
    connect(m_worker, &SQLExecutorWorker::queryFinished,
            this, &SQLExecutor::onQueryFinished);
    
    connect(m_workerThread, &QThread::finished,
            m_worker, &QObject::deleteLater);
    
    m_workerThread->start();
}

void SQLExecutor::stopWorkerThread()
{
    if (!m_workerThread) {
        return;
    }
    
    m_workerThread->quit();
    if (!m_workerThread->wait(5000)) {
        qWarning() << "Worker thread did not finish within timeout, terminating";
        m_workerThread->terminate();
        m_workerThread->wait(2000);
    }
    
    m_workerThread = nullptr;
    m_worker = nullptr;
}

void SQLExecutor::executeQuery(const QString &query)
{
    try {
        if (m_isExecuting) {
            qWarning() << "Query execution already in progress";
            return;
        }

        if (!m_worker || !m_workerThread || !m_workerThread->isRunning()) {
            emit queryExecuted(false, "Worker thread not available");
            return;
        }

        m_isExecuting = true;
        m_shouldCancel = false;

        emit executionProgress("Executing query...");

        QMetaObject::invokeMethod(m_worker, "executeQuery", Qt::QueuedConnection,
                                  Q_ARG(QString, query));
    } catch (const std::exception &e) {
        m_isExecuting = false;
        qCritical() << "SQLExecutor::executeQuery exception:" << e.what();
        emit queryExecuted(false, QString("Execution exception: %1").arg(e.what()));
    } catch (...) {
        m_isExecuting = false;
        qCritical() << "SQLExecutor::executeQuery unknown exception";
        emit queryExecuted(false, "Execution unknown exception");
    }
}

bool SQLExecutor::isExecuting() const
{
    return m_isExecuting;
}

DuckDBManager::QueryResult SQLExecutor::getResults() const
{
    QMutexLocker locker(&m_resultsMutex);
    return m_lastResults;
}

void SQLExecutor::cancelExecution()
{
    m_shouldCancel = true;
    emit executionProgress("Cancelling query...");
}

void SQLExecutor::onQueryFinished(bool success, const QString &error, const DuckDBManager::QueryResult &result)
{
    try {
        m_isExecuting = false;

        if (m_shouldCancel) {
            emit queryExecuted(false, "Query cancelled by user");
            emit executionProgress("Query cancelled");
            return;
        }

        {
            QMutexLocker locker(&m_resultsMutex);
            m_lastResults = result;
        }

        emit queryExecuted(success, error);

        if (success) {
            emit executionProgress(QString("Query completed in %1ms, %2 rows returned")
                                  .arg(result.executionTimeMs)
                                  .arg(result.totalRows));
            emit resultsReady();
        } else {
            emit executionProgress("Query failed");
        }
    } catch (const std::exception &e) {
        qCritical() << "SQLExecutor::onQueryFinished exception:" << e.what();
        emit queryExecuted(false, QString("Result processing exception: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "SQLExecutor::onQueryFinished unknown exception";
        emit queryExecuted(false, "Result processing unknown exception");
    }
}