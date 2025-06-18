#ifndef CHARTMANAGER_H
#define CHARTMANAGER_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QVariantList>
#include <QStringList>
#include <QPair>
#include <QDateTime>
#include <memory>
#include "duckdbmanager.h"

class ChartWidget;

class ChartManager : public QWidget
{
    Q_OBJECT

public:
    enum AggregationType {
        NoAggregation,
        Count,
        Sum,
        Average,
        Minimum,
        Maximum,
        StandardDeviation
    };

    enum DataType {
        NumericType,
        StringType,
        DateTimeType,
        BooleanType
    };

    struct ColumnInfo {
        QString name;
        DataType type;
        int index;
        double minValue;
        double maxValue;
        QStringList uniqueValues;  // For categorical data
    };

    struct ChartData {
        QStringList xLabels;
        QList<double> xValues;
        QList<double> yValues;
        QStringList categories;
        QString xAxisTitle;
        QString yAxisTitle;
        QString chartTitle;
    };

    explicit ChartManager(QWidget *parent = nullptr);

    // Data analysis
    QList<ColumnInfo> analyzeColumns(const DuckDBManager::QueryResult &results);
    DataType detectColumnType(const QVariantList &columnData);
    
    // Data processing for charts
    ChartData prepareBarChartData(const DuckDBManager::QueryResult &results, 
                                  const QString &xColumn, const QString &yColumn,
                                  const QString &groupBy = QString(),
                                  AggregationType aggregation = NoAggregation);
    
    ChartData prepareLineChartData(const DuckDBManager::QueryResult &results,
                                   const QString &xColumn, const QString &yColumn,
                                   const QString &groupBy = QString());
    
    ChartData prepareScatterData(const DuckDBManager::QueryResult &results,
                                 const QString &xColumn, const QString &yColumn,
                                 const QString &colorBy = QString());
    
    ChartData preparePieChartData(const DuckDBManager::QueryResult &results,
                                  const QString &labelColumn, const QString &valueColumn,
                                  AggregationType aggregation = Count);
    
    ChartData prepareHistogramData(const DuckDBManager::QueryResult &results,
                                   const QString &column, int bins = 20);

    // Statistical functions
    double calculateStatistic(const QList<double> &values, AggregationType type);
    QList<double> createBins(const QList<double> &values, int binCount);
    QStringList createCategoricalGroups(const QVariantList &values);

    // Utility functions
    QString aggregationTypeToString(AggregationType type);
    AggregationType stringToAggregationType(const QString &str);
    QString dataTypeToString(DataType type);
    
    // Data conversion helpers
    double variantToDouble(const QVariant &value, bool *ok = nullptr);
    QDateTime variantToDateTime(const QVariant &value, bool *ok = nullptr);
    QString formatValue(const QVariant &value, DataType type);

    // UI Management methods
    void setData(const DuckDBManager::QueryResult &results, const QString &fileName = QString());
    void clearCharts();
    void setVisible(bool visible);
    bool isVisible() const;

public slots:
    void onAddChart();
    void onCloseChart(int index);
    void onClosePanel();
    void onTabChanged(int index);

signals:
    void panelClosed();
    void chartAdded();
    void chartRemoved();

private:
    // UI setup methods
    void setupUI();
    void setupConnections();
    void updateTabText(int index, const QString &title);
    void createNewChart(const QString &title = QString());
    QString generateChartTitle(int chartNumber = 0);
    void saveCurrentFileCharts();
    void restoreFileCharts(const QString &fileName);

    // Helper methods
    int findColumnIndex(const QStringList &columnNames, const QString &columnName);
    QVariantList getColumnData(const DuckDBManager::QueryResult &results, int columnIndex);
    QMap<QString, QList<double>> groupNumericData(const DuckDBManager::QueryResult &results,
                                                   const QString &groupColumn, const QString &valueColumn,
                                                   AggregationType aggregation);

    // UI Components
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_headerLayout;
    QLabel *m_titleLabel;
    QPushButton *m_addChartButton;
    QPushButton *m_closePanelButton;
    QTabWidget *m_tabWidget;
    
    // Data management
    DuckDBManager::QueryResult m_currentData;
    QString m_currentFileName;
    QMap<QString, QList<ChartWidget*>> m_fileCharts; // Charts per file
    int m_chartCounter;
    bool m_isVisible;
};

#endif // CHARTMANAGER_H