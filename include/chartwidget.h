#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QChartView>
#include <QChart>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <memory>
#include "duckdbmanager.h"

QT_BEGIN_NAMESPACE
class QBarSeries;
class QLineSeries;
class QScatterSeries;
class QPieSeries;
class QValueAxis;
class QBarCategoryAxis;
class QDateTimeAxis;
QT_END_NAMESPACE

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    enum ChartType {
        BarChart,
        LineChart,
        ScatterChart,
        PieChart,
        Histogram
    };

    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget();

    void setData(const DuckDBManager::QueryResult &results);
    void clearChart();

public slots:
    void onChartTypeChanged();
    void onColumnSelectionChanged();
    void onAggregationChanged();
    void onRefreshChart();
    void onExportChart();

signals:
    void chartConfigChanged();

private:
    void setupUI();
    void setupConnections();
    void populateColumnSelectors();
    void createBarChart();
    void createLineChart();
    void createScatterChart();
    void createPieChart();
    void createHistogram();
    void updateChartTheme();
    void configureAxes();

    // UI Components
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_controlsLayout;
    QGroupBox *m_controlsGroup;
    
    // Chart controls
    QComboBox *m_chartTypeCombo;
    QLabel *m_xAxisLabel;
    QComboBox *m_xAxisCombo;
    QLabel *m_yAxisLabel;
    QComboBox *m_yAxisCombo;
    QComboBox *m_groupByCombo;
    QComboBox *m_aggregationCombo;
    QPushButton *m_refreshButton;
    QPushButton *m_exportButton;
    QCheckBox *m_showGridCheck;
    QCheckBox *m_showLegendCheck;
    QSpinBox *m_binsSpin;
    QLabel *m_statsLabel;

    // Chart components
    QChartView *m_chartView;
    QChart *m_chart;
    
    // Data
    DuckDBManager::QueryResult m_data;
    ChartType m_currentType;
    
    // Chart series (Qt manages these through parent-child ownership)
    QBarSeries *m_barSeries;
    QLineSeries *m_lineSeries;
    QScatterSeries *m_scatterSeries;
    QPieSeries *m_pieSeries;
    
    // Axes
    QValueAxis *m_xValueAxis;
    QValueAxis *m_yValueAxis;
    QBarCategoryAxis *m_xCategoryAxis;
    QDateTimeAxis *m_xDateTimeAxis;

    static constexpr int DEFAULT_HISTOGRAM_BINS = 20;
};

#endif // CHARTWIDGET_H