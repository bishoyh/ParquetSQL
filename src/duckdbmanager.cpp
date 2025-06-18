#include "duckdbmanager.h"
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>
#include <QDir>
#include <QRegularExpression>

DuckDBManager::DuckDBManager(QObject *parent)
    : QObject(parent)
    , m_database(nullptr)
    , m_connection(nullptr)
    , m_connected(false)
{
    initialize();
}

DuckDBManager::~DuckDBManager()
{
    cleanup();
}

bool DuckDBManager::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_connected) {
        return true;
    }
    
    return setupDatabase();
}

bool DuckDBManager::setupDatabase()
{
    cleanup();
    
    m_database = new duckdb_database;
    m_connection = new duckdb_connection;
    
    if (duckdb_open(":memory:", m_database) == DuckDBError) {
        m_lastError = "Failed to create in-memory database";
        cleanup();
        return false;
    }
    
    if (duckdb_connect(*m_database, m_connection) == DuckDBError) {
        m_lastError = "Failed to connect to database";
        cleanup();
        return false;
    }
    
    duckdb_result result;
    const char* extensions[] = {
        "INSTALL parquet;",
        "LOAD parquet;",
        "SET memory_limit='2GB';",
        "SET threads=4;"
    };
    
    for (const char* sql : extensions) {
        if (duckdb_query(*m_connection, sql, &result) == DuckDBError) {
            qWarning() << "Warning: Failed to execute:" << sql;
        }
        duckdb_destroy_result(&result);
    }
    
    m_connected = true;
    m_lastError.clear();
    return true;
}

void DuckDBManager::cleanup()
{
    if (m_connection) {
        duckdb_disconnect(m_connection);
        delete m_connection;
        m_connection = nullptr;
    }
    
    if (m_database) {
        duckdb_close(m_database);
        delete m_database;
        m_database = nullptr;
    }
    
    m_connected = false;
    m_loadedTables.clear();
}

bool DuckDBManager::loadFile(const QString &filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_connected) {
        m_lastError = "Database not connected";
        return false;
    }
    
    if (!QFile::exists(filePath)) {
        m_lastError = "File does not exist: " + filePath;
        return false;
    }
    
    QString fileType = detectFileType(filePath);
    bool success = false;
    
    if (fileType == "parquet") {
        success = loadParquetFile(filePath);
    } else if (fileType == "csv") {
        success = loadCSVFile(filePath);
    } else {
        m_lastError = "Unsupported file type: " + fileType;
        return false;
    }
    
    return success;
}

QString DuckDBManager::detectFileType(const QString &filePath)
{
    QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == "parquet") {
        return "parquet";
    } else if (suffix == "csv" || suffix == "tsv") {
        return "csv";
    }
    return "unknown";
}

bool DuckDBManager::loadParquetFile(const QString &filePath)
{
    QString tableName = generateTableName(filePath);
    QString sql = QString("CREATE OR REPLACE TABLE %1 AS SELECT * FROM parquet_scan('%2');")
                      .arg(tableName)
                      .arg(filePath);
    
    duckdb_result result;
    if (duckdb_query(*m_connection, sql.toUtf8().constData(), &result) == DuckDBError) {
        m_lastError = QString("Failed to load Parquet file: %1").arg(duckdb_result_error(&result));
        duckdb_destroy_result(&result);
        return false;
    }
    
    duckdb_destroy_result(&result);
    
    if (!m_loadedTables.contains(tableName)) {
        m_loadedTables.append(tableName);
    }
    
    return true;
}

bool DuckDBManager::loadCSVFile(const QString &filePath)
{
    QString tableName = generateTableName(filePath);
    QString delimiter = QFileInfo(filePath).suffix().toLower() == "tsv" ? "\t" : ",";
    
    QString sql = QString("CREATE OR REPLACE TABLE %1 AS SELECT * FROM read_csv_auto('%2', delim='%3', header=true);")
                      .arg(tableName)
                      .arg(filePath)
                      .arg(delimiter);
    
    duckdb_result result;
    if (duckdb_query(*m_connection, sql.toUtf8().constData(), &result) == DuckDBError) {
        m_lastError = QString("Failed to load CSV file: %1").arg(duckdb_result_error(&result));
        duckdb_destroy_result(&result);
        return false;
    }
    
    duckdb_destroy_result(&result);
    
    if (!m_loadedTables.contains(tableName)) {
        m_loadedTables.append(tableName);
    }
    
    return true;
}

QString DuckDBManager::generateTableName(const QString &filePath)
{
    QString baseName = QFileInfo(filePath).baseName();
    baseName = baseName.replace(QRegularExpression("[^a-zA-Z0-9_]"), "_");
    if (baseName.isEmpty() || baseName[0].isDigit()) {
        baseName = "table_" + baseName;
    }
    return baseName;
}

DuckDBManager::QueryResult DuckDBManager::executeQuery(const QString &query)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    QueryResult result;
    result.success = false;
    
    if (!m_connected) {
        result.error = "Database not connected";
        return result;
    }
    
    QElapsedTimer timer;
    timer.start();
    
    duckdb_result duckResult;
    if (duckdb_query(*m_connection, query.toUtf8().constData(), &duckResult) == DuckDBError) {
        result.error = QString("Query error: %1").arg(duckdb_result_error(&duckResult));
        duckdb_destroy_result(&duckResult);
        return result;
    }
    
    result.executionTimeMs = timer.elapsed();
    
    idx_t columnCount = duckdb_column_count(&duckResult);
    idx_t rowCount = duckdb_row_count(&duckResult);
    
    result.totalRows = static_cast<int>(rowCount);
    
    for (idx_t col = 0; col < columnCount; col++) {
        result.columnNames.append(QString::fromUtf8(duckdb_column_name(&duckResult, col)));
    }
    
    for (idx_t row = 0; row < rowCount; row++) {
        QVariantList rowData;
        for (idx_t col = 0; col < columnCount; col++) {
            if (duckdb_value_is_null(&duckResult, col, row)) {
                rowData.append(QVariant());
            } else {
                switch (duckdb_column_type(&duckResult, col)) {
                    case DUCKDB_TYPE_BOOLEAN:
                        rowData.append(duckdb_value_boolean(&duckResult, col, row));
                        break;
                    case DUCKDB_TYPE_TINYINT:
                        rowData.append(duckdb_value_int8(&duckResult, col, row));
                        break;
                    case DUCKDB_TYPE_SMALLINT:
                        rowData.append(duckdb_value_int16(&duckResult, col, row));
                        break;
                    case DUCKDB_TYPE_INTEGER:
                        rowData.append(duckdb_value_int32(&duckResult, col, row));
                        break;
                    case DUCKDB_TYPE_BIGINT:
                        rowData.append(static_cast<qint64>(duckdb_value_int64(&duckResult, col, row)));
                        break;
                    case DUCKDB_TYPE_FLOAT:
                        rowData.append(duckdb_value_float(&duckResult, col, row));
                        break;
                    case DUCKDB_TYPE_DOUBLE:
                        rowData.append(duckdb_value_double(&duckResult, col, row));
                        break;
                    case DUCKDB_TYPE_VARCHAR:
                    default: {
                        char* str = duckdb_value_varchar(&duckResult, col, row);
                        rowData.append(QString::fromUtf8(str));
                        duckdb_free(str);
                        break;
                    }
                }
            }
        }
        result.rows.append(rowData);
    }
    
    duckdb_destroy_result(&duckResult);
    result.success = true;
    return result;
}

QStringList DuckDBManager::getLoadedTables() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_loadedTables;
}