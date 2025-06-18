#ifndef SQLEXECUTOR_H
#define SQLEXECUTOR_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <memory>
#include "duckdbmanager.h"

class SQLExecutorWorker : public QObject
{
    Q_OBJECT

public:
    explicit SQLExecutorWorker(DuckDBManager *dbManager);

public slots:
    void executeQuery(const QString &query);

signals:
    void queryFinished(bool success, const QString &error, const DuckDBManager::QueryResult &result);

private:
    DuckDBManager *m_dbManager;
};

class SQLExecutor : public QObject
{
    Q_OBJECT

public:
    explicit SQLExecutor(DuckDBManager *dbManager, QObject *parent = nullptr);
    ~SQLExecutor();

    void executeQuery(const QString &query);
    bool isExecuting() const;
    
    DuckDBManager::QueryResult getResults() const;
    void cancelExecution();

signals:
    void queryExecuted(bool success, const QString &error);
    void resultsReady();
    void executionProgress(const QString &status);

private slots:
    void onQueryFinished(bool success, const QString &error, const DuckDBManager::QueryResult &result);

private:
    void startWorkerThread();
    void stopWorkerThread();

    DuckDBManager *m_dbManager;
    QThread *m_workerThread;
    SQLExecutorWorker *m_worker;
    
    mutable QMutex m_resultsMutex;
    DuckDBManager::QueryResult m_lastResults;
    bool m_isExecuting;
    bool m_shouldCancel;
};

#endif // SQLEXECUTOR_H