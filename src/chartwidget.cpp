#include "chartwidget.h"
#include "chartmanager.h"
#include <QChart>
#include <QChartView>
#include <QBarSeries>
#include <QLineSeries>
#include <QScatterSeries>
#include <QPieSeries>
#include <QBarSet>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QDateTimeAxis>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QSvgGenerator>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_controlsLayout(nullptr)
    , m_controlsGroup(nullptr)
    , m_chartTypeCombo(nullptr)
    , m_xAxisLabel(nullptr)
    , m_xAxisCombo(nullptr)
    , m_yAxisLabel(nullptr)
    , m_yAxisCombo(nullptr)
    , m_groupByCombo(nullptr)
    , m_aggregationCombo(nullptr)
    , m_refreshButton(nullptr)
    , m_exportButton(nullptr)
    , m_showGridCheck(nullptr)
    , m_showLegendCheck(nullptr)
    , m_binsSpin(nullptr)
    , m_statsLabel(nullptr)
    , m_chartView(nullptr)
    , m_chart(nullptr)
    , m_currentType(BarChart)
    , m_barSeries(nullptr)
    , m_lineSeries(nullptr)
    , m_scatterSeries(nullptr)
    , m_pieSeries(nullptr)
    , m_xValueAxis(nullptr)
    , m_yValueAxis(nullptr)
    , m_xCategoryAxis(nullptr)
    , m_xDateTimeAxis(nullptr)
{
    setupUI();
    setupConnections();
    clearChart();
}

ChartWidget::~ChartWidget()
{
    // Qt handles cleanup through parent-child relationships
}

void ChartWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Create controls group with more compact layout
    m_controlsGroup = new QGroupBox("Chart Configuration");
    m_controlsGroup->setMaximumHeight(100);
    QVBoxLayout *groupLayout = new QVBoxLayout(m_controlsGroup);
    groupLayout->setContentsMargins(5, 5, 5, 5);
    groupLayout->setSpacing(3);

    // First row: chart type and columns
    QHBoxLayout *row1 = new QHBoxLayout();
    m_controlsLayout = row1; // Keep reference for compatibility
    
    // Chart type selector
    m_controlsLayout->addWidget(new QLabel("Type:"));
    m_chartTypeCombo = new QComboBox();
    m_chartTypeCombo->addItems({"Bar Chart", "Line Chart", "Scatter Plot", "Pie Chart", "Histogram"});
    m_chartTypeCombo->setToolTip("Select the type of chart to display");
    m_controlsLayout->addWidget(m_chartTypeCombo);

    // X-axis selector (label will be updated based on chart type)
    m_xAxisLabel = new QLabel("X-Axis:");
    m_xAxisLabel->setToolTip("Column to use for X axis / categories");
    m_controlsLayout->addWidget(m_xAxisLabel);
    m_xAxisCombo = new QComboBox();
    m_xAxisCombo->setToolTip("Select column for X axis (or labels for Pie/Histogram)");
    m_controlsLayout->addWidget(m_xAxisCombo);

    // Y-axis selector (label will be updated based on chart type)
    m_yAxisLabel = new QLabel("Y-Axis:");
    m_yAxisLabel->setToolTip("Column to use for Y axis / values");
    m_controlsLayout->addWidget(m_yAxisLabel);
    m_yAxisCombo = new QComboBox();
    m_yAxisCombo->setToolTip("Select column for Y axis values");
    m_controlsLayout->addWidget(m_yAxisCombo);

    // Group by selector
    QLabel *groupLabel = new QLabel("Group:");
    groupLabel->setToolTip("Group data by this column (optional)");
    m_controlsLayout->addWidget(groupLabel);
    m_groupByCombo = new QComboBox();
    m_groupByCombo->addItem("(None)");
    m_groupByCombo->setToolTip("Optionally group data by another column");
    m_controlsLayout->addWidget(m_groupByCombo);

    // Aggregation selector
    QLabel *aggLabel = new QLabel("Aggregation:");
    aggLabel->setToolTip("How to aggregate grouped data");
    row1->addWidget(aggLabel);
    m_aggregationCombo = new QComboBox();
    m_aggregationCombo->addItems({"None", "Count", "Sum", "Average", "Min", "Max", "StdDev"});
    m_aggregationCombo->setToolTip("Select aggregation function (Count, Sum, Average, etc.)");
    row1->addWidget(m_aggregationCombo);

    // Histogram bins (initially hidden)
    QLabel *binsLabel = new QLabel("Bins:");
    binsLabel->setToolTip("Number of bins for histogram");
    row1->addWidget(binsLabel);
    m_binsSpin = new QSpinBox();
    m_binsSpin->setRange(5, 100);
    m_binsSpin->setValue(DEFAULT_HISTOGRAM_BINS);
    m_binsSpin->setToolTip("Number of bins to group data into (5-100)");
    m_binsSpin->setVisible(false);
    binsLabel->setVisible(false);
    row1->addWidget(m_binsSpin);

    row1->addStretch();
    groupLayout->addLayout(row1);

    // Second row: buttons and options
    QHBoxLayout *row2 = new QHBoxLayout();

    // Control buttons
    m_refreshButton = new QPushButton("Refresh Chart");
    m_refreshButton->setToolTip("Refresh the chart with current settings");
    m_exportButton = new QPushButton("Export Image");
    m_exportButton->setToolTip("Export chart as PNG or SVG image");
    row2->addWidget(m_refreshButton);
    row2->addWidget(m_exportButton);

    // Options
    m_showGridCheck = new QCheckBox("Show Grid");
    m_showGridCheck->setChecked(true);
    m_showGridCheck->setToolTip("Toggle grid lines on/off");
    m_showLegendCheck = new QCheckBox("Show Legend");
    m_showLegendCheck->setChecked(true);
    m_showLegendCheck->setToolTip("Toggle chart legend on/off");
    row2->addWidget(m_showGridCheck);
    row2->addWidget(m_showLegendCheck);

    row2->addStretch();

    // Stats label
    m_statsLabel = new QLabel("No data");
    row2->addWidget(m_statsLabel);

    groupLayout->addLayout(row2);

    m_mainLayout->addWidget(m_controlsGroup);
    
    // Create chart
    m_chart = new QChart();
    m_chart->setBackgroundBrush(QBrush(QColor(25, 25, 25)));
    m_chart->setTitleBrush(QBrush(Qt::white));
    m_chart->legend()->setLabelColor(Qt::white);
    
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setStyleSheet("background-color: #2b2b2b; border: 1px solid #555;");
    
    m_mainLayout->addWidget(m_chartView);
}

void ChartWidget::setupConnections()
{
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartWidget::onChartTypeChanged);
    connect(m_xAxisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartWidget::onColumnSelectionChanged);
    connect(m_yAxisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartWidget::onColumnSelectionChanged);
    connect(m_groupByCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartWidget::onColumnSelectionChanged);
    connect(m_aggregationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartWidget::onAggregationChanged);
    connect(m_binsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ChartWidget::onColumnSelectionChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, &ChartWidget::onRefreshChart);
    connect(m_exportButton, &QPushButton::clicked, this, &ChartWidget::onExportChart);
    connect(m_showGridCheck, &QCheckBox::toggled, this, &ChartWidget::onColumnSelectionChanged);
    connect(m_showLegendCheck, &QCheckBox::toggled, this, &ChartWidget::onColumnSelectionChanged);
}

void ChartWidget::setData(const DuckDBManager::QueryResult &results)
{
    m_data = results;
    populateColumnSelectors();
    onRefreshChart();
}

void ChartWidget::populateColumnSelectors()
{
    // Block signals during population to prevent premature chart updates
    m_xAxisCombo->blockSignals(true);
    m_yAxisCombo->blockSignals(true);
    m_groupByCombo->blockSignals(true);
    
    m_xAxisCombo->clear();
    m_yAxisCombo->clear();
    m_groupByCombo->clear();
    m_groupByCombo->addItem("(None)");
    
    for (const QString &column : m_data.columnNames) {
        m_xAxisCombo->addItem(column);
        m_yAxisCombo->addItem(column);
        m_groupByCombo->addItem(column);
    }
    
    // Set sensible defaults
    if (m_data.columnNames.size() >= 2) {
        m_xAxisCombo->setCurrentIndex(0);
        m_yAxisCombo->setCurrentIndex(1);
    }
    
    m_statsLabel->setText(QString("%1 rows, %2 columns")
                         .arg(m_data.totalRows)
                         .arg(m_data.columnNames.size()));
    
    // Re-enable signals
    m_xAxisCombo->blockSignals(false);
    m_yAxisCombo->blockSignals(false);
    m_groupByCombo->blockSignals(false);
}

void ChartWidget::onChartTypeChanged()
{
    try {
        qDebug() << "onChartTypeChanged: Starting";

        if (!m_chartTypeCombo) {
            qWarning() << "onChartTypeChanged: m_chartTypeCombo is null";
            return;
        }

        m_currentType = static_cast<ChartType>(m_chartTypeCombo->currentIndex());
        qDebug() << "Chart type changed to:" << m_currentType;

        // Show/hide bins spinner for histogram
        bool isHistogram = (m_currentType == Histogram);
        if (m_binsSpin) {
            m_binsSpin->setVisible(isHistogram);
        }

        // Find and toggle bins label visibility
        if (m_controlsLayout) {
            int binsIndex = m_controlsLayout->indexOf(m_binsSpin);
            if (binsIndex > 0 && binsIndex - 1 < m_controlsLayout->count()) {
                QLayoutItem* item = m_controlsLayout->itemAt(binsIndex - 1);
                if (item && item->widget()) {
                    item->widget()->setVisible(isHistogram);
                }
            }
        }

        // Update labels and visibility based on chart type
        if (m_xAxisLabel && m_yAxisLabel) {
            switch (m_currentType) {
                case BarChart:
                    m_xAxisLabel->setText("Categories:");
                    m_xAxisLabel->setToolTip("Column to use for bar categories");
                    m_yAxisLabel->setText("Values:");
                    m_yAxisLabel->setToolTip("Column to use for bar values");
                    if (m_xAxisCombo) m_xAxisCombo->setToolTip("Select column for categories (X-axis)");
                    if (m_yAxisCombo) m_yAxisCombo->setToolTip("Select column for values (Y-axis)");
                    break;

                case LineChart:
                    m_xAxisLabel->setText("X-Axis:");
                    m_xAxisLabel->setToolTip("Column to use for X axis (typically numeric or time)");
                    m_yAxisLabel->setText("Y-Axis:");
                    m_yAxisLabel->setToolTip("Column to use for Y axis values");
                    if (m_xAxisCombo) m_xAxisCombo->setToolTip("Select numeric/time column for X-axis");
                    if (m_yAxisCombo) m_yAxisCombo->setToolTip("Select numeric column for Y-axis");
                    break;

                case ScatterChart:
                    m_xAxisLabel->setText("X Values:");
                    m_xAxisLabel->setToolTip("Numeric column for X coordinate");
                    m_yAxisLabel->setText("Y Values:");
                    m_yAxisLabel->setToolTip("Numeric column for Y coordinate");
                    if (m_xAxisCombo) m_xAxisCombo->setToolTip("Select numeric column for X coordinate");
                    if (m_yAxisCombo) m_yAxisCombo->setToolTip("Select numeric column for Y coordinate");
                    break;

                case PieChart:
                    m_xAxisLabel->setText("Labels:");
                    m_xAxisLabel->setToolTip("Column to use for pie slice labels");
                    m_yAxisLabel->setText("Values:");
                    m_yAxisLabel->setToolTip("Column to use for pie slice sizes");
                    if (m_xAxisCombo) m_xAxisCombo->setToolTip("Select column for slice labels (categorical)");
                    if (m_yAxisCombo) m_yAxisCombo->setToolTip("Select numeric column for slice values");
                    break;

                case Histogram:
                    m_xAxisLabel->setText("Data Column:");
                    m_xAxisLabel->setToolTip("Numeric column to create histogram from");
                    m_yAxisLabel->setText("(Frequency)");
                    m_yAxisLabel->setToolTip("Frequency is calculated automatically");
                    if (m_xAxisCombo) m_xAxisCombo->setToolTip("Select numeric column for histogram");
                    break;
            }
        }

        // Update Y-axis visibility for pie chart and histogram
        bool showYAxis = (m_currentType != PieChart && m_currentType != Histogram);
        if (m_yAxisCombo) {
            m_yAxisCombo->setVisible(showYAxis);
        }
        if (m_yAxisLabel) {
            m_yAxisLabel->setVisible(showYAxis);
        }

        qDebug() << "onChartTypeChanged: Calling refresh";
        onRefreshChart();
    } catch (const std::exception &e) {
        qCritical() << "onChartTypeChanged exception:" << e.what();
    } catch (...) {
        qCritical() << "onChartTypeChanged unknown exception";
    }
}

void ChartWidget::onColumnSelectionChanged()
{
    onRefreshChart();
}

void ChartWidget::onAggregationChanged()
{
    onRefreshChart();
}

void ChartWidget::onRefreshChart()
{
    try {
        if (m_data.columnNames.isEmpty()) {
            clearChart();
            return;
        }

        // Clear existing series
        m_chart->removeAllSeries();
        m_barSeries = nullptr;
        m_lineSeries = nullptr;
        m_scatterSeries = nullptr;
        m_pieSeries = nullptr;

        switch (m_currentType) {
        case BarChart:
            createBarChart();
            break;
        case LineChart:
            createLineChart();
            break;
        case ScatterChart:
            createScatterChart();
            break;
        case PieChart:
            createPieChart();
            break;
        case Histogram:
            createHistogram();
            break;
        }

        updateChartTheme();
        configureAxes();
    } catch (const std::exception &e) {
        qCritical() << "ChartWidget::onRefreshChart exception:" << e.what();
        m_chart->setTitle(QString("Chart Error: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "ChartWidget::onRefreshChart unknown exception";
        m_chart->setTitle("Chart Error: Unknown error");
    }
}

void ChartWidget::createBarChart()
{
    try {
        qDebug() << "createBarChart: Starting";

        if (!m_chart) {
            qWarning() << "createBarChart: m_chart is null";
            return;
        }

        ChartManager manager;
        QString xColumn = m_xAxisCombo ? m_xAxisCombo->currentText() : QString();
        QString yColumn = m_yAxisCombo ? m_yAxisCombo->currentText() : QString();

        if (xColumn.isEmpty() || yColumn.isEmpty()) {
            qWarning() << "createBarChart: Empty column selection";
            m_chart->setTitle("Select X and Y columns");
            return;
        }

        QString groupBy = (m_groupByCombo && m_groupByCombo->currentText() != "(None)")
                          ? m_groupByCombo->currentText() : QString();

        ChartManager::AggregationType agg = m_aggregationCombo
            ? static_cast<ChartManager::AggregationType>(m_aggregationCombo->currentIndex())
            : ChartManager::NoAggregation;

        auto chartData = manager.prepareBarChartData(m_data, xColumn, yColumn, groupBy, agg);

        if (chartData.yValues.isEmpty()) {
            qWarning() << "createBarChart: No data to display";
            m_chart->setTitle("No data to display");
            return;
        }

        m_barSeries = new QBarSeries();
        QBarSet *set = new QBarSet(yColumn);

        for (double value : chartData.yValues) {
            *set << value;
        }

        set->setColor(QColor(42, 130, 218));
        m_barSeries->append(set);
        m_chart->addSeries(m_barSeries);

        // Set descriptive title with aggregation info
        QString title = QString("%1 by %2").arg(yColumn).arg(xColumn);
        if (m_aggregationCombo && m_aggregationCombo->currentIndex() > 0) {
            title = QString("%1: %2 by %3")
                .arg(m_aggregationCombo->currentText())
                .arg(yColumn)
                .arg(xColumn);
        }
        m_chart->setTitle(title);
        m_chart->setTitleFont(QFont("Arial", 14, QFont::Bold));

        qDebug() << "createBarChart: Completed successfully";
    } catch (const std::exception &e) {
        qCritical() << "createBarChart exception:" << e.what();
        if (m_chart) m_chart->setTitle(QString("Error: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "createBarChart unknown exception";
        if (m_chart) m_chart->setTitle("Error creating bar chart");
    }
}

void ChartWidget::createLineChart()
{
    try {
        qDebug() << "createLineChart: Starting";

        if (!m_chart) {
            qWarning() << "createLineChart: m_chart is null";
            return;
        }

        ChartManager manager;
        QString xColumn = m_xAxisCombo ? m_xAxisCombo->currentText() : QString();
        QString yColumn = m_yAxisCombo ? m_yAxisCombo->currentText() : QString();

        if (xColumn.isEmpty() || yColumn.isEmpty()) {
            qWarning() << "createLineChart: Empty column selection";
            m_chart->setTitle("Select X and Y columns");
            return;
        }

        auto chartData = manager.prepareLineChartData(m_data, xColumn, yColumn);

        if (chartData.xValues.isEmpty() || chartData.yValues.isEmpty()) {
            qWarning() << "createLineChart: No data to display";
            m_chart->setTitle("No data to display");
            return;
        }

        m_lineSeries = new QLineSeries();
        m_lineSeries->setName(yColumn);

        for (int i = 0; i < chartData.xValues.size(); ++i) {
            m_lineSeries->append(chartData.xValues[i], chartData.yValues[i]);
        }

        m_lineSeries->setColor(QColor(42, 130, 218));
        m_chart->addSeries(m_lineSeries);
        QString title = QString("%1 vs %2").arg(yColumn).arg(xColumn);
        m_chart->setTitle(title);
        m_chart->setTitleFont(QFont("Arial", 14, QFont::Bold));

        qDebug() << "createLineChart: Completed successfully";
    } catch (const std::exception &e) {
        qCritical() << "createLineChart exception:" << e.what();
        if (m_chart) m_chart->setTitle(QString("Error: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "createLineChart unknown exception";
        if (m_chart) m_chart->setTitle("Error creating line chart");
    }
}

void ChartWidget::createScatterChart()
{
    try {
        qDebug() << "createScatterChart: Starting";

        if (!m_chart) {
            qWarning() << "createScatterChart: m_chart is null";
            return;
        }

        ChartManager manager;
        QString xColumn = m_xAxisCombo ? m_xAxisCombo->currentText() : QString();
        QString yColumn = m_yAxisCombo ? m_yAxisCombo->currentText() : QString();

        if (xColumn.isEmpty() || yColumn.isEmpty()) {
            qWarning() << "createScatterChart: Empty column selection";
            m_chart->setTitle("Select X and Y columns");
            return;
        }

        auto chartData = manager.prepareScatterData(m_data, xColumn, yColumn);

        if (chartData.xValues.isEmpty() || chartData.yValues.isEmpty()) {
            qWarning() << "createScatterChart: No data to display";
            m_chart->setTitle("No data to display");
            return;
        }

        m_scatterSeries = new QScatterSeries();
        m_scatterSeries->setName(QString("%1 vs %2").arg(yColumn).arg(xColumn));

        for (int i = 0; i < chartData.xValues.size(); ++i) {
            m_scatterSeries->append(chartData.xValues[i], chartData.yValues[i]);
        }

        m_scatterSeries->setColor(QColor(42, 130, 218));
        m_scatterSeries->setMarkerSize(8);
        m_chart->addSeries(m_scatterSeries);
        QString title = QString("Scatter Plot: %1 vs %2").arg(yColumn).arg(xColumn);
        m_chart->setTitle(title);
        m_chart->setTitleFont(QFont("Arial", 14, QFont::Bold));

        qDebug() << "createScatterChart: Completed successfully";
    } catch (const std::exception &e) {
        qCritical() << "createScatterChart exception:" << e.what();
        if (m_chart) m_chart->setTitle(QString("Error: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "createScatterChart unknown exception";
        if (m_chart) m_chart->setTitle("Error creating scatter chart");
    }
}

void ChartWidget::createPieChart()
{
    try {
        qDebug() << "createPieChart: Starting";

        if (!m_chart) {
            qWarning() << "createPieChart: m_chart is null";
            return;
        }

        ChartManager manager;
        QString labelColumn = m_xAxisCombo ? m_xAxisCombo->currentText() : QString();
        QString valueColumn = m_yAxisCombo ? m_yAxisCombo->currentText() : QString();

        if (labelColumn.isEmpty()) {
            qWarning() << "createPieChart: Empty label column";
            m_chart->setTitle("Select label column");
            return;
        }

        ChartManager::AggregationType agg = m_aggregationCombo
            ? static_cast<ChartManager::AggregationType>(m_aggregationCombo->currentIndex())
            : ChartManager::Count;
        if (agg == ChartManager::NoAggregation) {
            agg = ChartManager::Count;
        }

        auto chartData = manager.preparePieChartData(m_data, labelColumn, valueColumn, agg);

        if (chartData.xLabels.isEmpty() || chartData.yValues.isEmpty()) {
            qWarning() << "createPieChart: No data to display";
            m_chart->setTitle("No data to display");
            return;
        }

        m_pieSeries = new QPieSeries();

        for (int i = 0; i < chartData.xLabels.size(); ++i) {
            m_pieSeries->append(chartData.xLabels[i], chartData.yValues[i]);
        }

        m_chart->addSeries(m_pieSeries);
        QString title = QString("%1 Distribution").arg(labelColumn);
        if (m_aggregationCombo && m_aggregationCombo->currentIndex() > 0) {
            title = QString("%1 Distribution by %2")
                .arg(labelColumn)
                .arg(m_aggregationCombo->currentText());
        }
        m_chart->setTitle(title);
        m_chart->setTitleFont(QFont("Arial", 14, QFont::Bold));

        // Add percentage labels to pie slices
        if (m_pieSeries) {
            for (QPieSlice *slice : m_pieSeries->slices()) {
                if (slice) {
                    slice->setLabelVisible(true);
                    slice->setLabel(QString("%1: %2%")
                        .arg(slice->label())
                        .arg(slice->percentage() * 100, 0, 'f', 1));
                    slice->setLabelColor(Qt::white);
                }
            }
        }

        qDebug() << "createPieChart: Completed successfully";
    } catch (const std::exception &e) {
        qCritical() << "createPieChart exception:" << e.what();
        if (m_chart) m_chart->setTitle(QString("Error: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "createPieChart unknown exception";
        if (m_chart) m_chart->setTitle("Error creating pie chart");
    }
}

void ChartWidget::createHistogram()
{
    try {
        qDebug() << "createHistogram: Starting";

        if (!m_chart) {
            qWarning() << "createHistogram: m_chart is null";
            return;
        }

        ChartManager manager;
        QString column = m_xAxisCombo ? m_xAxisCombo->currentText() : QString();
        int bins = m_binsSpin ? m_binsSpin->value() : DEFAULT_HISTOGRAM_BINS;

        if (column.isEmpty()) {
            qWarning() << "createHistogram: Empty column selection";
            m_chart->setTitle("Select a column for histogram");
            return;
        }

        qDebug() << "createHistogram: Preparing data for column" << column << "with" << bins << "bins";
        auto chartData = manager.prepareHistogramData(m_data, column, bins);

        if (chartData.xLabels.isEmpty() || chartData.yValues.isEmpty()) {
            qWarning() << "createHistogram: No data to display";
            m_chart->setTitle(QString("No numeric data in column '%1'").arg(column));
            return;
        }

        qDebug() << "createHistogram: Creating bar series with" << chartData.yValues.size() << "values";
        m_barSeries = new QBarSeries();
        QBarSet *set = new QBarSet("Frequency");

        for (double value : chartData.yValues) {
            *set << value;
        }

        set->setColor(QColor(42, 130, 218));
        m_barSeries->append(set);
        m_chart->addSeries(m_barSeries);

        QString title = QString("Histogram of %1 (%2 bins)").arg(column).arg(bins);
        m_chart->setTitle(title);
        m_chart->setTitleFont(QFont("Arial", 14, QFont::Bold));

        qDebug() << "createHistogram: Completed successfully";
    } catch (const std::exception &e) {
        qCritical() << "createHistogram exception:" << e.what();
        if (m_chart) m_chart->setTitle(QString("Error: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "createHistogram unknown exception";
        if (m_chart) m_chart->setTitle("Error creating histogram");
    }
}

void ChartWidget::clearChart()
{
    m_chart->removeAllSeries();
    m_chart->setTitle("No Data");
    m_statsLabel->setText("No data");
    
    // Reset series pointers (Qt deletes them when removed from chart)
    m_barSeries = nullptr;
    m_lineSeries = nullptr;
    m_scatterSeries = nullptr;
    m_pieSeries = nullptr;
}

void ChartWidget::updateChartTheme()
{
    m_chart->legend()->setVisible(m_showLegendCheck->isChecked());
    
    // Apply dark theme colors
    if (m_chart->series().isEmpty()) return;
    
    auto series = m_chart->series().first();
    if (auto barSeries = qobject_cast<QBarSeries*>(series)) {
        // Bar series colors are set in creation methods
    } else if (auto lineSeries = qobject_cast<QLineSeries*>(series)) {
        lineSeries->setColor(QColor(42, 130, 218));
    } else if (auto scatterSeries = qobject_cast<QScatterSeries*>(series)) {
        scatterSeries->setColor(QColor(42, 130, 218));
    }
}

void ChartWidget::configureAxes()
{
    try {
        qDebug() << "configureAxes: Starting";

        if (!m_chart) {
            qWarning() << "configureAxes: m_chart is null";
            return;
        }

        if (m_chart->series().isEmpty()) {
            qDebug() << "configureAxes: No series to configure";
            return;
        }

        auto series = m_chart->series().first();
        if (!series) {
            qWarning() << "configureAxes: First series is null";
            return;
        }

        // Remove existing axes safely
        QList<QAbstractAxis*> existingAxes = m_chart->axes();
        for (auto axis : existingAxes) {
            if (axis) {
                m_chart->removeAxis(axis);
                axis->deleteLater();  // Safely delete later
            }
        }

        // Reset axis pointers
        m_xValueAxis = nullptr;
        m_yValueAxis = nullptr;
        m_xCategoryAxis = nullptr;
        m_xDateTimeAxis = nullptr;

        // Create and configure axes based on chart type
        if (m_currentType == PieChart) {
            qDebug() << "configureAxes: Pie chart doesn't need axes";
            return;
        }

        // Create appropriate axes
        if (m_currentType == BarChart || m_currentType == Histogram) {
            qDebug() << "configureAxes: Creating category axes";
            m_xCategoryAxis = new QBarCategoryAxis();
            m_yValueAxis = new QValueAxis();

            // Set category labels for bar chart
            if (m_currentType == BarChart && m_xAxisCombo && m_yAxisCombo) {
                try {
                    ChartManager manager;
                    QString xColumn = m_xAxisCombo->currentText();
                    QString yColumn = m_yAxisCombo->currentText();
                    auto chartData = manager.prepareBarChartData(m_data, xColumn, yColumn);
                    if (!chartData.xLabels.isEmpty()) {
                        m_xCategoryAxis->setCategories(chartData.xLabels);
                    }
                } catch (...) {
                    qWarning() << "configureAxes: Failed to set category labels";
                }
            }

            m_chart->addAxis(m_xCategoryAxis, Qt::AlignBottom);
            m_chart->addAxis(m_yValueAxis, Qt::AlignLeft);
            series->attachAxis(m_xCategoryAxis);
            series->attachAxis(m_yValueAxis);

            // Auto-scale Y axis to include negative values
            if (m_barSeries) {
                qreal minY = 0, maxY = 0;
                bool hasData = false;
                for (QBarSet* set : m_barSeries->barSets()) {
                    if (set) {
                        for (int i = 0; i < set->count(); ++i) {
                            qreal val = set->at(i);
                            if (!hasData) {
                                minY = maxY = val;
                                hasData = true;
                            } else {
                                minY = qMin(minY, val);
                                maxY = qMax(maxY, val);
                            }
                        }
                    }
                }
                if (hasData) {
                    qreal range = maxY - minY;
                    qreal padding = range * 0.1;  // 10% padding
                    m_yValueAxis->setRange(minY - padding, maxY + padding);
                    qDebug() << "Y axis range:" << minY - padding << "to" << maxY + padding;
                }
            }
        } else {
            qDebug() << "configureAxes: Creating value axes";
            m_xValueAxis = new QValueAxis();
            m_yValueAxis = new QValueAxis();

            m_chart->addAxis(m_xValueAxis, Qt::AlignBottom);
            m_chart->addAxis(m_yValueAxis, Qt::AlignLeft);
            series->attachAxis(m_xValueAxis);
            series->attachAxis(m_yValueAxis);

            // Auto-scale axes to include negative values
            if (m_lineSeries || m_scatterSeries) {
                qreal minX = 0, maxX = 0, minY = 0, maxY = 0;
                bool hasData = false;

                QList<QPointF> points;
                if (m_lineSeries) points = m_lineSeries->points();
                else if (m_scatterSeries) points = m_scatterSeries->points();

                for (const QPointF& point : points) {
                    if (!hasData) {
                        minX = maxX = point.x();
                        minY = maxY = point.y();
                        hasData = true;
                    } else {
                        minX = qMin(minX, point.x());
                        maxX = qMax(maxX, point.x());
                        minY = qMin(minY, point.y());
                        maxY = qMax(maxY, point.y());
                    }
                }

                if (hasData) {
                    qreal xRange = maxX - minX;
                    qreal yRange = maxY - minY;
                    qreal xPadding = xRange * 0.1;  // 10% padding
                    qreal yPadding = yRange * 0.1;

                    m_xValueAxis->setRange(minX - xPadding, maxX + xPadding);
                    m_yValueAxis->setRange(minY - yPadding, maxY + yPadding);
                    qDebug() << "X axis range:" << minX - xPadding << "to" << maxX + xPadding;
                    qDebug() << "Y axis range:" << minY - yPadding << "to" << maxY + yPadding;
                }
            }
        }

        // Style axes for dark theme with better labels
        if (m_xValueAxis) {
            m_xValueAxis->setLabelsColor(Qt::white);
            m_xValueAxis->setTitleText(m_xAxisCombo ? m_xAxisCombo->currentText() : "X Axis");
            m_xValueAxis->setTitleBrush(QBrush(Qt::white));
            m_xValueAxis->setGridLineVisible(m_showGridCheck ? m_showGridCheck->isChecked() : true);
            m_xValueAxis->setLabelsFont(QFont("Arial", 10));
            m_xValueAxis->setTitleFont(QFont("Arial", 11, QFont::Bold));
        }
        if (m_xCategoryAxis) {
            m_xCategoryAxis->setLabelsColor(Qt::white);
            m_xCategoryAxis->setTitleText(m_xAxisCombo ? m_xAxisCombo->currentText() : "X Axis");
            m_xCategoryAxis->setTitleBrush(QBrush(Qt::white));
            m_xCategoryAxis->setGridLineVisible(m_showGridCheck ? m_showGridCheck->isChecked() : true);
            m_xCategoryAxis->setLabelsFont(QFont("Arial", 10));
            m_xCategoryAxis->setTitleFont(QFont("Arial", 11, QFont::Bold));
        }
        if (m_yValueAxis) {
            m_yValueAxis->setLabelsColor(Qt::white);
            QString yAxisTitle = m_yAxisCombo ? m_yAxisCombo->currentText() : "Y Axis";
            // Add aggregation info to Y-axis title if applicable
            if (m_aggregationCombo && m_aggregationCombo->currentIndex() > 0) {
                yAxisTitle = QString("%1 (%2)")
                    .arg(yAxisTitle)
                    .arg(m_aggregationCombo->currentText());
            }
            m_yValueAxis->setTitleText(yAxisTitle);
            m_yValueAxis->setTitleBrush(QBrush(Qt::white));
            m_yValueAxis->setGridLineVisible(m_showGridCheck ? m_showGridCheck->isChecked() : true);
            m_yValueAxis->setLabelsFont(QFont("Arial", 10));
            m_yValueAxis->setTitleFont(QFont("Arial", 11, QFont::Bold));
        }

        qDebug() << "configureAxes: Completed successfully";
    } catch (const std::exception &e) {
        qCritical() << "ChartWidget::configureAxes exception:" << e.what();
    } catch (...) {
        qCritical() << "ChartWidget::configureAxes unknown exception";
    }
}

void ChartWidget::onExportChart()
{
    qDebug() << "ChartWidget::onExportChart called";

    try {
        // Validate all required objects exist
        if (!m_chartView) {
            qWarning() << "Export failed: m_chartView is null";
            QMessageBox::warning(this, tr("Export Failed"), tr("Chart view not available"));
            return;
        }

        if (!m_chart) {
            qWarning() << "Export failed: m_chart is null";
            QMessageBox::warning(this, tr("Export Failed"), tr("Chart not available"));
            return;
        }

        qDebug() << "Opening file dialog...";

        // Use nullptr as parent to avoid potential issues with widget hierarchy
        QString fileName;
        try {
            fileName = QFileDialog::getSaveFileName(
                nullptr,  // Use nullptr instead of 'this' to avoid hierarchy issues
                tr("Export Chart"),
                QDir::homePath() + "/chart.png",
                tr("PNG Images (*.png);;SVG Vector Graphics (*.svg)"),
                nullptr,
                QFileDialog::DontUseNativeDialog  // Use Qt dialog instead of native
            );
        } catch (...) {
            qCritical() << "QFileDialog crashed!";
            QMessageBox::critical(nullptr, tr("Error"), tr("File dialog failed to open"));
            return;
        }

        if (fileName.isEmpty()) {
            qDebug() << "User cancelled export";
            return;  // User cancelled
        }

        qDebug() << "Exporting to:" << fileName;

        // Detect format from file extension
        QString suffix = QFileInfo(fileName).suffix().toLower();
        if (suffix.isEmpty()) {
            suffix = "png";  // Default to PNG if no extension
            fileName += ".png";
        }

        bool success = false;
        QString errorMsg;

        if (suffix == "svg") {
            qDebug() << "Attempting SVG export...";
            // Export as SVG
            try {
                // Validate chart scene exists
                QGraphicsScene* scene = m_chart->scene();
                if (!scene) {
                    qWarning() << "Chart scene is null, cannot export SVG";
                    errorMsg = tr("Chart scene not available for SVG export");
                } else {
                    QSvgGenerator generator;
                    generator.setFileName(fileName);

                    QSize size = m_chartView->size();
                    if (size.isEmpty()) {
                        size = QSize(800, 600);  // Fallback size
                    }
                    generator.setSize(size);
                    generator.setViewBox(QRect(0, 0, size.width(), size.height()));
                    generator.setTitle(m_chart->title());
                    generator.setDescription("Chart exported from ParquetSQL");

                    QPainter painter;
                    if (!painter.begin(&generator)) {
                        qWarning() << "Failed to begin painting on SVG generator";
                        errorMsg = tr("Failed to initialize SVG painter");
                    } else {
                        qDebug() << "Rendering scene to SVG...";
                        scene->render(&painter);
                        painter.end();
                        success = true;
                        qDebug() << "SVG export successful";
                    }
                }
            } catch (const std::exception &e) {
                errorMsg = QString("SVG export error: %1").arg(e.what());
                qCritical() << "SVG export exception:" << e.what();
            } catch (...) {
                errorMsg = tr("Unknown SVG export error");
                qCritical() << "SVG export unknown exception";
            }
        } else {
            qDebug() << "Attempting PNG export...";
            // Export as PNG (or any pixmap-supported format)
            try {
                qDebug() << "Grabbing pixmap from chart view...";
                QPixmap pixmap = m_chartView->grab();
                qDebug() << "Pixmap size:" << pixmap.size() << "isNull:" << pixmap.isNull();

                if (pixmap.isNull()) {
                    qWarning() << "Grabbed pixmap is null";
                    errorMsg = tr("Failed to capture chart image");
                } else {
                    qDebug() << "Saving pixmap to file...";
                    if (!pixmap.save(fileName)) {
                        qWarning() << "Failed to save pixmap to file:" << fileName;
                        errorMsg = tr("Failed to save image file");
                    } else {
                        success = true;
                        qDebug() << "PNG export successful";
                    }
                }
            } catch (const std::exception &e) {
                errorMsg = QString("PNG export error: %1").arg(e.what());
                qCritical() << "PNG export exception:" << e.what();
            } catch (...) {
                errorMsg = tr("Unknown PNG export error");
                qCritical() << "PNG export unknown exception";
            }
        }

        qDebug() << "Export result - success:" << success << "error:" << errorMsg;

        if (success) {
            QMessageBox::information(this, tr("Export Successful"),
                                   tr("Chart exported to %1").arg(fileName));
        } else {
            QMessageBox::warning(this, tr("Export Failed"),
                                errorMsg.isEmpty() ? tr("Failed to export chart to %1").arg(fileName) : errorMsg);
        }

    } catch (const std::exception &e) {
        qCritical() << "ChartWidget::onExportChart exception:" << e.what();
        QMessageBox::critical(this, tr("Export Failed"),
                            tr("Export failed with error: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "ChartWidget::onExportChart unknown exception";
        QMessageBox::critical(this, tr("Export Failed"),
                            tr("Export failed with unknown error"));
    }

    qDebug() << "ChartWidget::onExportChart completed";
}