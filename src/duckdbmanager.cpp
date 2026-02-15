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
    , m_isDiskBased(false)
{
    initialize();
}

DuckDBManager::~DuckDBManager()
{
    cleanup();
}

bool DuckDBManager::initialize(bool useDiskDatabase, const QString &dbPath)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_connected) {
        return true;
    }

    m_isDiskBased = useDiskDatabase;
    m_databasePath = dbPath;

    return setupDatabase();
}

bool DuckDBManager::setupDatabase()
{
    cleanup();

    m_database = new duckdb_database;
    m_connection = new duckdb_connection;

    QByteArray dbLocationBytes = (m_isDiskBased && !m_databasePath.isEmpty())
                               ? m_databasePath.toUtf8()
                               : QByteArray(":memory:");
    const char* dbLocation = dbLocationBytes.constData();

    if (duckdb_open(dbLocation, m_database) == DuckDBError) {
        m_lastError = QString("Failed to create %1 database").arg(m_isDiskBased ? "disk-based" : "in-memory");
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
        "SET memory_limit='8GB';",
        "SET threads TO 8;",
        "SET preserve_insertion_order=false;",
        "SET temp_directory='/tmp/duckdb_temp';"
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
    QString escapedPath = filePath;
    escapedPath.replace("'", "''");
    // Use CREATE VIEW for large files to avoid loading everything into memory
    // DuckDB will read from parquet file on-demand
    QString sql = QString("CREATE OR REPLACE VIEW \"%1\" AS SELECT * FROM parquet_scan('%2');")
                      .arg(tableName)
                      .arg(escapedPath);

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

    m_lastLoadedTable = tableName;
    return true;
}

bool DuckDBManager::loadCSVFile(const QString &filePath)
{
    QString tableName = generateTableName(filePath);
    QString delimiter = QFileInfo(filePath).suffix().toLower() == "tsv" ? "\t" : ",";
    QString escapedPath = filePath;
    escapedPath.replace("'", "''");

    QString sql = QString("CREATE OR REPLACE TABLE \"%1\" AS SELECT * FROM read_csv_auto('%2', delim='%3', header=true);")
                      .arg(tableName)
                      .arg(escapedPath)
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

    m_lastLoadedTable = tableName;
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

    try {
        if (!m_connected) {
            result.error = "Database not connected";
            return result;
        }

        if (query.trimmed().isEmpty()) {
            result.error = "Query is empty";
            return result;
        }

        QElapsedTimer timer;
        timer.start();

        duckdb_result duckResult;
        if (duckdb_query(*m_connection, query.toUtf8().constData(), &duckResult) == DuckDBError) {
            const char* errorMsg = duckdb_result_error(&duckResult);
            result.error = QString("Query error: %1").arg(errorMsg ? errorMsg : "Unknown error");
            duckdb_destroy_result(&duckResult);
            qWarning() << "DuckDB query failed:" << result.error;
            return result;
        }

        result.executionTimeMs = timer.elapsed();

        idx_t columnCount = duckdb_column_count(&duckResult);
        idx_t rowCount = duckdb_row_count(&duckResult);

        result.totalRows = static_cast<int>(rowCount);

        // Extract column names
        try {
            for (idx_t col = 0; col < columnCount; col++) {
                const char* colName = duckdb_column_name(&duckResult, col);
                result.columnNames.append(QString::fromUtf8(colName ? colName : ""));
            }
        } catch (const std::exception &e) {
            result.error = QString("Error extracting column names: %1").arg(e.what());
            duckdb_destroy_result(&duckResult);
            return result;
        } catch (...) {
            result.error = "Unknown error extracting column names";
            duckdb_destroy_result(&duckResult);
            return result;
        }

        // Extract row data
        try {
            for (idx_t row = 0; row < rowCount; row++) {
                QVariantList rowData;
                for (idx_t col = 0; col < columnCount; col++) {
                    try {
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
                                    rowData.append(QString::fromUtf8(str ? str : ""));
                                    if (str) duckdb_free(str);
                                    break;
                                }
                            }
                        }
                    } catch (const std::exception &e) {
                        qWarning() << "Error extracting cell value at row" << row << "col" << col << ":" << e.what();
                        rowData.append(QVariant(QString("[Error: %1]").arg(e.what())));
                    } catch (...) {
                        qWarning() << "Unknown error extracting cell value at row" << row << "col" << col;
                        rowData.append(QVariant("[Error: Unknown]"));
                    }
                }
                result.rows.append(rowData);
            }
        } catch (const std::exception &e) {
            result.error = QString("Error extracting row data: %1").arg(e.what());
            duckdb_destroy_result(&duckResult);
            return result;
        } catch (...) {
            result.error = "Unknown error extracting row data";
            duckdb_destroy_result(&duckResult);
            return result;
        }

        duckdb_destroy_result(&duckResult);
        result.success = true;
        return result;
    } catch (const std::exception &e) {
        result.error = QString("Exception in executeQuery: %1").arg(e.what());
        qCritical() << "DuckDBManager::executeQuery exception:" << result.error;
        return result;
    } catch (...) {
        result.error = "Unknown exception in executeQuery";
        qCritical() << "DuckDBManager::executeQuery unknown exception";
        return result;
    }
}

bool DuckDBManager::interruptQuery()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected || !m_connection) {
        return false;
    }
    duckdb_interrupt(*m_connection);
    return true;
}

QStringList DuckDBManager::getLoadedTables() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_loadedTables;
}

QStringList DuckDBManager::getAllTables() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    QStringList tables;

    if (!m_connected) {
        return tables;
    }

    // Query to get all tables and views from DuckDB
    const char* query = "SELECT table_name FROM information_schema.tables WHERE table_schema = 'main' ORDER BY table_name;";

    duckdb_result result;
    if (duckdb_query(*m_connection, query, &result) == DuckDBError) {
        qWarning() << "Failed to query tables:" << duckdb_result_error(&result);
        duckdb_destroy_result(&result);
        return tables;
    }

    idx_t rowCount = duckdb_row_count(&result);
    for (idx_t row = 0; row < rowCount; row++) {
        if (!duckdb_value_is_null(&result, 0, row)) {
            char* tableName = duckdb_value_varchar(&result, 0, row);
            tables.append(QString::fromUtf8(tableName));
            duckdb_free(tableName);
        }
    }

    duckdb_destroy_result(&result);
    return tables;
}
