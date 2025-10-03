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

    bool initialize(bool useDiskDatabase = false, const QString &dbPath = QString());
    bool loadFile(const QString &filePath);
    QueryResult executeQuery(const QString &query);
    bool isConnected() const { return m_connected; }

    QStringList getLoadedTables() const;
    QStringList getAllTables() const;
    QString getLastLoadedTableName() const { return m_lastLoadedTable; }
    QString getLastError() const { return m_lastError; }
    QString getCurrentDatabasePath() const { return m_databasePath; }
    bool isDiskBased() const { return m_isDiskBased; }

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
    bool m_isDiskBased;
    QString m_databasePath;
    QString m_lastError;
    QStringList m_loadedTables;
    QString m_lastLoadedTable;
    mutable std::mutex m_mutex;
};

#endif // DUCKDBMANAGER_H