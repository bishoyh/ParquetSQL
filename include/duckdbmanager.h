#ifndef DUCKDBMANAGER_H
#define DUCKDBMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <memory>
#include <mutex>

extern "C" {
    #include <duckdb.h>
}

class DuckDBManager : public QObject
{
    Q_OBJECT

public:
    struct QueryResult {
        QStringList columnNames;
        QList<QVariantList> rows;
        QString error;
        bool success;
        qint64 executionTimeMs;
        int totalRows;
    };

    explicit DuckDBManager(QObject *parent = nullptr);
    ~DuckDBManager();

    bool initialize();
    bool loadFile(const QString &filePath);
    QueryResult executeQuery(const QString &query);
    bool isConnected() const { return m_connected; }
    
    QStringList getLoadedTables() const;
    QString getLastError() const { return m_lastError; }

private:
    bool setupDatabase();
    void cleanup();
    QString detectFileType(const QString &filePath);
    bool loadParquetFile(const QString &filePath);
    bool loadCSVFile(const QString &filePath);
    QString generateTableName(const QString &filePath);
    
    duckdb_database *m_database;
    duckdb_connection *m_connection;
    bool m_connected;
    QString m_lastError;
    QStringList m_loadedTables;
    mutable std::mutex m_mutex;
};

#endif // DUCKDBMANAGER_H