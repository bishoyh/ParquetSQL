# ParquetSQL

A high-performance Qt5-based application for browsing and querying Parquet and CSV files using DuckDB as the SQL engine.

## Features

- **File Browser**: Navigate and select Parquet (.parquet) and CSV (.csv, .tsv) files
- **SQL Editor**: Syntax-highlighted SQL editor with auto-completion
- **Fast Queries**: Powered by DuckDB for optimized analytical queries
- **Pagination**: Handle large result sets with built-in pagination (1000 rows per page)
- **Threading**: Non-blocking SQL execution using background threads
- **Dark Theme**: Modern dark UI theme optimized for data analysis
- **Performance**: Optimized for large datasets with memory-efficient operations

## Requirements

- Qt5 (Core, Widgets, Sql)
- DuckDB library
- CMake 3.16+
- C++17 compiler
- pthread support

## Building

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install qt5-default libqt5widgets5 libqt5sql5 libduckdb-dev cmake build-essential
```

**macOS (with Homebrew):**
```bash
brew install qt5 duckdb cmake
```

**Windows:**
- Install Qt5 from https://www.qt.io/download
- Download DuckDB from https://duckdb.org/docs/installation

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## Usage

1. **Launch the application:**
   ```bash
   ./ParquetSQL
   ```

2. **Load a file:**
   - Use the file browser on the left to navigate to your data files
   - Select a .parquet, .csv, or .tsv file
   - Click "Load Selected File"

3. **Query your data:**
   - The SQL editor will populate with a default SELECT query
   - Modify the query as needed
   - Click "Execute Query" to run
   - Results appear in the table below with pagination controls

4. **Navigate results:**
   - Use pagination buttons (First, Previous, Next, Last)
   - View up to 1000 rows per page for optimal performance

## Performance Features

- **Memory Management**: 2GB memory limit with efficient data streaming
- **Multi-threading**: 4-thread execution for parallel query processing
- **Vectorized Operations**: DuckDB's columnar processing for fast analytics
- **Lazy Loading**: Results loaded on-demand with pagination
- **Query Optimization**: Automatic query planning and optimization

## SQL Examples

**Basic queries:**
```sql
SELECT * FROM your_table LIMIT 100;
SELECT column1, COUNT(*) FROM your_table GROUP BY column1;
SELECT * FROM your_table WHERE column1 > 1000 ORDER BY column2 DESC;
```

**Analytical queries:**
```sql
SELECT 
    column1,
    AVG(column2) as avg_value,
    MIN(column2) as min_value,
    MAX(column2) as max_value
FROM your_table 
GROUP BY column1 
HAVING COUNT(*) > 10
ORDER BY avg_value DESC;
```

## Keyboard Shortcuts

- **Ctrl+O**: Open file dialog
- **Ctrl+Space**: SQL auto-completion
- **Ctrl+Enter**: Execute query
- **Ctrl+Q**: Quit application

## Architecture

- **MainWindow**: Primary UI coordinator
- **DuckDBManager**: Database connection and query execution
- **SQLExecutor**: Threaded query execution
- **ResultsTableModel**: Paginated data model
- **SQLEditor**: Syntax-highlighted editor with completion
- **FileBrowser**: File system navigation

## License

This project is licensed under the MIT License.