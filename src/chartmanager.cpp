#include "chartmanager.h"
#include "chartwidget.h"
#include <QDebug>
#include <QRegularExpression>
#include <QtMath>
#include <QMap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QFileInfo>
#include <algorithm>
#include <numeric>

ChartManager::ChartManager(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_addChartButton(nullptr)
    , m_closePanelButton(nullptr)
    , m_tabWidget(nullptr)
    , m_chartCounter(0)
    , m_isVisible(true)
{
    setupUI();
    setupConnections();
}

QList<ChartManager::ColumnInfo> ChartManager::analyzeColumns(const DuckDBManager::QueryResult &results)
{
    QList<ColumnInfo> columnInfos;
    
    for (int i = 0; i < results.columnNames.size(); ++i) {
        ColumnInfo info;
        info.name = results.columnNames[i];
        info.index = i;
        
        QVariantList columnData = getColumnData(results, i);
        info.type = detectColumnType(columnData);
        
        if (info.type == NumericType) {
            QList<double> numericValues;
            for (const QVariant &value : columnData) {
                bool ok;
                double numValue = variantToDouble(value, &ok);
                if (ok) {
                    numericValues.append(numValue);
                }
            }
            
            if (!numericValues.isEmpty()) {
                info.minValue = *std::min_element(numericValues.begin(), numericValues.end());
                info.maxValue = *std::max_element(numericValues.begin(), numericValues.end());
            }
        } else if (info.type == StringType) {
            // Get unique values for categorical data (limit to reasonable number)
            QSet<QString> uniqueSet;
            for (const QVariant &value : columnData) {
                if (!value.isNull() && uniqueSet.size() < 100) {
                    uniqueSet.insert(value.toString());
                }
            }
            info.uniqueValues = uniqueSet.values();
        }
        
        columnInfos.append(info);
    }
    
    return columnInfos;
}

ChartManager::DataType ChartManager::detectColumnType(const QVariantList &columnData)
{
    if (columnData.isEmpty()) {
        return StringType;
    }
    
    int numericCount = 0;
    int dateTimeCount = 0;
    int booleanCount = 0;
    int totalCount = 0;
    
    for (const QVariant &value : columnData) {
        if (value.isNull()) continue;
        
        totalCount++;
        
        // Check if numeric
        bool ok;
        variantToDouble(value, &ok);
        if (ok) {
            numericCount++;
            continue;
        }
        
        // Check if boolean
        if (value.typeId() == QMetaType::Bool ||
            (value.typeId() == QMetaType::QString && 
             (value.toString().toLower() == "true" || value.toString().toLower() == "false"))) {
            booleanCount++;
            continue;
        }
        
        // Check if date/time
        QDateTime dateTime = variantToDateTime(value);
        if (dateTime.isValid()) {
            dateTimeCount++;
            continue;
        }
    }
    
    if (totalCount == 0) return StringType;
    
    // Determine type based on majority
    double numericRatio = double(numericCount) / totalCount;
    double dateTimeRatio = double(dateTimeCount) / totalCount;
    double booleanRatio = double(booleanCount) / totalCount;
    
    if (numericRatio > 0.8) return NumericType;
    if (dateTimeRatio > 0.8) return DateTimeType;
    if (booleanRatio > 0.8) return BooleanType;
    
    return StringType;
}

ChartManager::ChartData ChartManager::prepareBarChartData(const DuckDBManager::QueryResult &results,
                                                          const QString &xColumn, const QString &yColumn,
                                                          const QString &groupBy, AggregationType aggregation)
{
    ChartData data;
    data.xAxisTitle = xColumn;
    data.yAxisTitle = yColumn;
    data.chartTitle = QString("%1 by %2").arg(yColumn).arg(xColumn);
    
    int xIndex = findColumnIndex(results.columnNames, xColumn);
    int yIndex = findColumnIndex(results.columnNames, yColumn);
    
    if (xIndex == -1 || yIndex == -1) {
        return data;
    }
    
    if (groupBy.isEmpty() && aggregation == NoAggregation) {
        // Simple case: direct mapping
        for (const QVariantList &row : results.rows) {
            if (xIndex < row.size() && yIndex < row.size()) {
                data.xLabels.append(row[xIndex].toString());
                bool ok;
                double yValue = variantToDouble(row[yIndex], &ok);
                data.yValues.append(ok ? yValue : 0.0);
                data.xValues.append(data.xValues.size());
            }
        }
    } else {
        // Aggregation required
        QMap<QString, QList<double>> groupedData = groupNumericData(results, xColumn, yColumn, aggregation);
        
        for (auto it = groupedData.begin(); it != groupedData.end(); ++it) {
            data.xLabels.append(it.key());
            double aggregatedValue = calculateStatistic(it.value(), aggregation);
            data.yValues.append(aggregatedValue);
            data.xValues.append(data.xValues.size());
        }
    }
    
    return data;
}

ChartManager::ChartData ChartManager::prepareLineChartData(const DuckDBManager::QueryResult &results,
                                                           const QString &xColumn, const QString &yColumn,
                                                           const QString &groupBy)
{
    ChartData data;
    data.xAxisTitle = xColumn;
    data.yAxisTitle = yColumn;
    data.chartTitle = QString("%1 vs %2").arg(yColumn).arg(xColumn);
    
    int xIndex = findColumnIndex(results.columnNames, xColumn);
    int yIndex = findColumnIndex(results.columnNames, yColumn);
    
    if (xIndex == -1 || yIndex == -1) {
        return data;
    }
    
    // Collect data points
    QList<QPair<double, double>> points;
    
    for (const QVariantList &row : results.rows) {
        if (xIndex < row.size() && yIndex < row.size()) {
            bool xOk, yOk;
            double xValue = variantToDouble(row[xIndex], &xOk);
            double yValue = variantToDouble(row[yIndex], &yOk);
            
            if (xOk && yOk) {
                points.append(qMakePair(xValue, yValue));
            }
        }
    }
    
    // Sort by x-value for proper line drawing
    std::sort(points.begin(), points.end(), [](const QPair<double, double> &a, const QPair<double, double> &b) {
        return a.first < b.first;
    });
    
    // Extract sorted values
    for (const auto &point : points) {
        data.xValues.append(point.first);
        data.yValues.append(point.second);
    }
    
    return data;
}

ChartManager::ChartData ChartManager::prepareScatterData(const DuckDBManager::QueryResult &results,
                                                         const QString &xColumn, const QString &yColumn,
                                                         const QString &colorBy)
{
    // For now, scatter data is similar to line data without sorting
    ChartData data;
    data.xAxisTitle = xColumn;
    data.yAxisTitle = yColumn;
    data.chartTitle = QString("%1 vs %2").arg(yColumn).arg(xColumn);
    
    int xIndex = findColumnIndex(results.columnNames, xColumn);
    int yIndex = findColumnIndex(results.columnNames, yColumn);
    
    if (xIndex == -1 || yIndex == -1) {
        return data;
    }
    
    for (const QVariantList &row : results.rows) {
        if (xIndex < row.size() && yIndex < row.size()) {
            bool xOk, yOk;
            double xValue = variantToDouble(row[xIndex], &xOk);
            double yValue = variantToDouble(row[yIndex], &yOk);
            
            if (xOk && yOk) {
                data.xValues.append(xValue);
                data.yValues.append(yValue);
            }
        }
    }
    
    return data;
}

ChartManager::ChartData ChartManager::preparePieChartData(const DuckDBManager::QueryResult &results,
                                                          const QString &labelColumn, const QString &valueColumn,
                                                          AggregationType aggregation)
{
    ChartData data;
    data.chartTitle = QString("%1 Distribution").arg(labelColumn);
    
    int labelIndex = findColumnIndex(results.columnNames, labelColumn);
    int valueIndex = findColumnIndex(results.columnNames, valueColumn);
    
    if (labelIndex == -1) {
        return data;
    }
    
    QMap<QString, QList<double>> groupedData;
    
    for (const QVariantList &row : results.rows) {
        if (labelIndex < row.size()) {
            QString label = row[labelIndex].toString();
            
            if (aggregation == Count) {
                groupedData[label].append(1.0);
            } else if (valueIndex != -1 && valueIndex < row.size()) {
                bool ok;
                double value = variantToDouble(row[valueIndex], &ok);
                if (ok) {
                    groupedData[label].append(value);
                }
            }
        }
    }
    
    // Calculate aggregated values
    for (auto it = groupedData.begin(); it != groupedData.end(); ++it) {
        data.xLabels.append(it.key());
        double aggregatedValue = calculateStatistic(it.value(), aggregation);
        data.yValues.append(aggregatedValue);
    }
    
    return data;
}

ChartManager::ChartData ChartManager::prepareHistogramData(const DuckDBManager::QueryResult &results,
                                                           const QString &column, int bins)
{
    ChartData data;
    data.xAxisTitle = column;
    data.yAxisTitle = "Frequency";
    data.chartTitle = QString("Histogram of %1").arg(column);
    
    int columnIndex = findColumnIndex(results.columnNames, column);
    if (columnIndex == -1) {
        return data;
    }
    
    // Collect numeric values
    QList<double> values;
    for (const QVariantList &row : results.rows) {
        if (columnIndex < row.size()) {
            bool ok;
            double value = variantToDouble(row[columnIndex], &ok);
            if (ok) {
                values.append(value);
            }
        }
    }
    
    if (values.isEmpty()) {
        return data;
    }
    
    // Create bins
    double minVal = *std::min_element(values.begin(), values.end());
    double maxVal = *std::max_element(values.begin(), values.end());
    double binWidth = (maxVal - minVal) / bins;
    
    QList<int> binCounts(bins, 0);
    
    // Count values in each bin
    for (double value : values) {
        int binIndex = qMin(bins - 1, int((value - minVal) / binWidth));
        binCounts[binIndex]++;
    }
    
    // Create chart data
    for (int i = 0; i < bins; ++i) {
        double binStart = minVal + i * binWidth;
        double binEnd = minVal + (i + 1) * binWidth;
        data.xLabels.append(QString("[%1, %2)").arg(binStart, 0, 'f', 1).arg(binEnd, 0, 'f', 1));
        data.yValues.append(binCounts[i]);
        data.xValues.append(i);
    }
    
    return data;
}

double ChartManager::calculateStatistic(const QList<double> &values, AggregationType type)
{
    if (values.isEmpty()) {
        return 0.0;
    }
    
    switch (type) {
    case Count:
        return values.size();
    case Sum:
        return std::accumulate(values.begin(), values.end(), 0.0);
    case Average:
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    case Minimum:
        return *std::min_element(values.begin(), values.end());
    case Maximum:
        return *std::max_element(values.begin(), values.end());
    case StandardDeviation: {
        double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        double variance = std::accumulate(values.begin(), values.end(), 0.0,
                                        [mean](double sum, double value) {
                                            return sum + (value - mean) * (value - mean);
                                        }) / values.size();
        return qSqrt(variance);
    }
    default:
        return values.isEmpty() ? 0.0 : values.first();
    }
}

QString ChartManager::aggregationTypeToString(AggregationType type)
{
    switch (type) {
    case NoAggregation: return "None";
    case Count: return "Count";
    case Sum: return "Sum";
    case Average: return "Average";
    case Minimum: return "Min";
    case Maximum: return "Max";
    case StandardDeviation: return "Std Dev";
    }
    return "None";
}

ChartManager::AggregationType ChartManager::stringToAggregationType(const QString &str)
{
    if (str == "Count") return Count;
    if (str == "Sum") return Sum;
    if (str == "Average") return Average;
    if (str == "Min") return Minimum;
    if (str == "Max") return Maximum;
    if (str == "Std Dev") return StandardDeviation;
    return NoAggregation;
}

QString ChartManager::dataTypeToString(DataType type)
{
    switch (type) {
    case NumericType: return "Numeric";
    case StringType: return "String";
    case DateTimeType: return "DateTime";
    case BooleanType: return "Boolean";
    }
    return "Unknown";
}

double ChartManager::variantToDouble(const QVariant &value, bool *ok)
{
    if (ok) *ok = true;
    
    if (value.isNull()) {
        if (ok) *ok = false;
        return 0.0;
    }
    
    switch (value.typeId()) {
    case QMetaType::Double:
    case QMetaType::Float:
        return value.toDouble();
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return value.toLongLong();
    case QMetaType::QString: {
        bool conversionOk;
        double result = value.toString().toDouble(&conversionOk);
        if (ok) *ok = conversionOk;
        return result;
    }
    default:
        if (ok) *ok = false;
        return 0.0;
    }
}

QDateTime ChartManager::variantToDateTime(const QVariant &value, bool *ok)
{
    if (ok) *ok = true;
    
    if (value.isNull()) {
        if (ok) *ok = false;
        return QDateTime();
    }
    
    if (value.typeId() == QMetaType::QDateTime) {
        return value.toDateTime();
    }
    
    if (value.typeId() == QMetaType::QString) {
        QString str = value.toString();
        
        // Try various date formats
        QStringList formats = {
            "yyyy-MM-dd hh:mm:ss",
            "yyyy-MM-dd",
            "MM/dd/yyyy",
            "dd/MM/yyyy",
            "yyyy-MM-ddThh:mm:ss",
            "yyyy-MM-ddThh:mm:ssZ"
        };
        
        for (const QString &format : formats) {
            QDateTime dateTime = QDateTime::fromString(str, format);
            if (dateTime.isValid()) {
                return dateTime;
            }
        }
    }
    
    if (ok) *ok = false;
    return QDateTime();
}

QString ChartManager::formatValue(const QVariant &value, DataType type)
{
    if (value.isNull()) {
        return "NULL";
    }
    
    switch (type) {
    case NumericType: {
        bool ok;
        double numValue = variantToDouble(value, &ok);
        if (ok) {
            return QString::number(numValue, 'f', 2);
        }
        return value.toString();
    }
    case DateTimeType: {
        QDateTime dateTime = variantToDateTime(value);
        if (dateTime.isValid()) {
            return dateTime.toString("yyyy-MM-dd hh:mm:ss");
        }
        return value.toString();
    }
    default:
        return value.toString();
    }
}

int ChartManager::findColumnIndex(const QStringList &columnNames, const QString &columnName)
{
    return columnNames.indexOf(columnName);
}

QVariantList ChartManager::getColumnData(const DuckDBManager::QueryResult &results, int columnIndex)
{
    QVariantList columnData;
    
    for (const QVariantList &row : results.rows) {
        if (columnIndex < row.size()) {
            columnData.append(row[columnIndex]);
        }
    }
    
    return columnData;
}

QMap<QString, QList<double>> ChartManager::groupNumericData(const DuckDBManager::QueryResult &results,
                                                            const QString &groupColumn, const QString &valueColumn,
                                                            AggregationType aggregation)
{
    QMap<QString, QList<double>> groupedData;
    
    int groupIndex = findColumnIndex(results.columnNames, groupColumn);
    int valueIndex = findColumnIndex(results.columnNames, valueColumn);
    
    if (groupIndex == -1) {
        return groupedData;
    }
    
    for (const QVariantList &row : results.rows) {
        if (groupIndex < row.size()) {
            QString groupKey = row[groupIndex].toString();
            
            if (aggregation == Count) {
                groupedData[groupKey].append(1.0);
            } else if (valueIndex != -1 && valueIndex < row.size()) {
                bool ok;
                double value = variantToDouble(row[valueIndex], &ok);
                if (ok) {
                    groupedData[groupKey].append(value);
                }
            }
        }
    }
    
    return groupedData;
}

// UI Management Methods

void ChartManager::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);
    
    // Header with title and controls
    m_headerLayout = new QHBoxLayout();
    
    m_titleLabel = new QLabel("Charts");
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    m_addChartButton = new QPushButton("+ Add Chart");
    m_addChartButton->setMaximumWidth(100);
    
    m_closePanelButton = new QPushButton("×");
    m_closePanelButton->setMaximumSize(20, 20);
    m_closePanelButton->setStyleSheet("QPushButton { font-size: 16px; font-weight: bold; }");
    m_closePanelButton->setToolTip("Close Charts Panel");
    
    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();
    m_headerLayout->addWidget(m_addChartButton);
    m_headerLayout->addWidget(m_closePanelButton);
    
    m_mainLayout->addLayout(m_headerLayout);
    
    // Tab widget for multiple charts
    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    
    m_mainLayout->addWidget(m_tabWidget);
    
    // Start with one default chart
    createNewChart("Chart 1");
}

void ChartManager::setupConnections()
{
    connect(m_addChartButton, &QPushButton::clicked, this, &ChartManager::onAddChart);
    connect(m_closePanelButton, &QPushButton::clicked, this, &ChartManager::onClosePanel);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &ChartManager::onCloseChart);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &ChartManager::onTabChanged);
}

void ChartManager::setData(const DuckDBManager::QueryResult &results, const QString &fileName)
{
    m_currentData = results;
    
    // If switching to a different file, save current charts and restore charts for new file
    if (!fileName.isEmpty() && fileName != m_currentFileName) {
        saveCurrentFileCharts();
        m_currentFileName = fileName;
        restoreFileCharts(fileName);
    } else {
        m_currentFileName = fileName;
        // Update all chart widgets with new data
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            ChartWidget *chartWidget = qobject_cast<ChartWidget*>(m_tabWidget->widget(i));
            if (chartWidget) {
                chartWidget->setData(results);
            }
        }
    }
    
    // Update title if we have a filename
    if (!fileName.isEmpty()) {
        m_titleLabel->setText(QString("Charts - %1").arg(QFileInfo(fileName).baseName()));
    } else {
        m_titleLabel->setText("Charts");
    }
}

void ChartManager::clearCharts()
{
    // Clear all charts but keep at least one tab
    while (m_tabWidget->count() > 1) {
        onCloseChart(m_tabWidget->count() - 1);
    }
    
    // Clear the remaining chart
    if (m_tabWidget->count() > 0) {
        ChartWidget *chartWidget = qobject_cast<ChartWidget*>(m_tabWidget->widget(0));
        if (chartWidget) {
            chartWidget->clearChart();
        }
    }
    
    m_currentData = DuckDBManager::QueryResult();
    m_currentFileName.clear();
    m_titleLabel->setText("Charts");
}

void ChartManager::setVisible(bool visible)
{
    m_isVisible = visible;
    QWidget::setVisible(visible);
}

bool ChartManager::isVisible() const
{
    return m_isVisible;
}

void ChartManager::onAddChart()
{
    QString title = generateChartTitle(m_tabWidget->count() + 1);
    createNewChart(title);
    emit chartAdded();
}

void ChartManager::onCloseChart(int index)
{
    if (m_tabWidget->count() <= 1) {
        // Don't close the last chart, just clear it
        ChartWidget *chartWidget = qobject_cast<ChartWidget*>(m_tabWidget->widget(index));
        if (chartWidget) {
            chartWidget->clearChart();
        }
        return;
    }
    
    QWidget *widget = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    delete widget;
    
    emit chartRemoved();
}

void ChartManager::onClosePanel()
{
    setVisible(false);
    emit panelClosed();
}

void ChartManager::onTabChanged(int index)
{
    Q_UNUSED(index)
    // Could add logic here to update specific chart when switching tabs
}

void ChartManager::updateTabText(int index, const QString &title)
{
    if (index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setTabText(index, title);
    }
}

void ChartManager::createNewChart(const QString &title)
{
    ChartWidget *chartWidget = new ChartWidget();
    
    // Set data if we have it
    if (!m_currentData.columnNames.isEmpty()) {
        chartWidget->setData(m_currentData);
    }
    
    int index = m_tabWidget->addTab(chartWidget, title);
    m_tabWidget->setCurrentIndex(index);
    
    // Connect chart signals if needed
    connect(chartWidget, &ChartWidget::chartConfigChanged, [this, index]() {
        // Could update tab title based on chart configuration
    });
}

QString ChartManager::generateChartTitle(int chartNumber)
{
    return QString("Chart %1").arg(chartNumber);
}

void ChartManager::saveCurrentFileCharts()
{
    if (m_currentFileName.isEmpty()) {
        return;
    }
    
    // Clear existing charts for this file
    m_fileCharts.remove(m_currentFileName);
    
    // Save current chart configurations
    QList<ChartWidget*> charts;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        ChartWidget *chartWidget = qobject_cast<ChartWidget*>(m_tabWidget->widget(i));
        if (chartWidget) {
            // Create a copy of the chart widget to store
            ChartWidget *chartCopy = new ChartWidget();
            chartCopy->setData(m_currentData);
            // Note: In a full implementation, we'd save/restore chart configurations
            // For now, we just store the widget itself
            charts.append(chartCopy);
        }
    }
    
    if (!charts.isEmpty()) {
        m_fileCharts[m_currentFileName] = charts;
    }
}

void ChartManager::restoreFileCharts(const QString &fileName)
{
    // Clear current tabs
    while (m_tabWidget->count() > 0) {
        QWidget *widget = m_tabWidget->widget(0);
        m_tabWidget->removeTab(0);
        delete widget;
    }
    
    // Restore charts for this file if they exist
    if (m_fileCharts.contains(fileName)) {
        const QList<ChartWidget*> &charts = m_fileCharts[fileName];
        for (int i = 0; i < charts.size(); ++i) {
            ChartWidget *chartWidget = new ChartWidget();
            chartWidget->setData(m_currentData);
            
            QString title = QString("Chart %1").arg(i + 1);
            m_tabWidget->addTab(chartWidget, title);
            
            // Connect chart signals
            connect(chartWidget, &ChartWidget::chartConfigChanged, [this, i]() {
                // Could update tab title based on chart configuration
            });
        }
        
        // Clean up the stored charts as we've created new ones
        qDeleteAll(charts);
        m_fileCharts.remove(fileName);
    } else {
        // No existing charts for this file, create a default one
        createNewChart("Chart 1");
    }
}