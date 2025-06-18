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
#include <QDebug>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_controlsLayout(nullptr)
    , m_controlsGroup(nullptr)
    , m_chartTypeCombo(nullptr)
    , m_xAxisCombo(nullptr)
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
    
    // Create controls group
    m_controlsGroup = new QGroupBox("Chart Configuration");
    m_controlsGroup->setMaximumHeight(120);
    m_controlsLayout = new QHBoxLayout(m_controlsGroup);
    
    // Chart type selector
    m_controlsLayout->addWidget(new QLabel("Type:"));
    m_chartTypeCombo = new QComboBox();
    m_chartTypeCombo->addItems({"Bar Chart", "Line Chart", "Scatter Plot", "Pie Chart", "Histogram"});
    m_controlsLayout->addWidget(m_chartTypeCombo);
    
    // X-axis selector
    m_controlsLayout->addWidget(new QLabel("X-Axis:"));
    m_xAxisCombo = new QComboBox();
    m_controlsLayout->addWidget(m_xAxisCombo);
    
    // Y-axis selector
    m_controlsLayout->addWidget(new QLabel("Y-Axis:"));
    m_yAxisCombo = new QComboBox();
    m_controlsLayout->addWidget(m_yAxisCombo);
    
    // Group by selector
    m_controlsLayout->addWidget(new QLabel("Group By:"));
    m_groupByCombo = new QComboBox();
    m_groupByCombo->addItem("(None)");
    m_controlsLayout->addWidget(m_groupByCombo);
    
    // Aggregation selector
    m_controlsLayout->addWidget(new QLabel("Aggregation:"));
    m_aggregationCombo = new QComboBox();
    m_aggregationCombo->addItems({"None", "Count", "Sum", "Average", "Min", "Max", "Std Dev"});
    m_controlsLayout->addWidget(m_aggregationCombo);
    
    // Histogram bins (initially hidden)
    m_controlsLayout->addWidget(new QLabel("Bins:"));
    m_binsSpin = new QSpinBox();
    m_binsSpin->setRange(5, 100);
    m_binsSpin->setValue(DEFAULT_HISTOGRAM_BINS);
    m_binsSpin->setVisible(false);
    m_controlsLayout->addWidget(m_binsSpin);
    
    // Control buttons
    m_refreshButton = new QPushButton("Refresh");
    m_exportButton = new QPushButton("Export");
    m_controlsLayout->addWidget(m_refreshButton);
    m_controlsLayout->addWidget(m_exportButton);
    
    // Options
    m_showGridCheck = new QCheckBox("Grid");
    m_showGridCheck->setChecked(true);
    m_showLegendCheck = new QCheckBox("Legend");
    m_showLegendCheck->setChecked(true);
    m_controlsLayout->addWidget(m_showGridCheck);
    m_controlsLayout->addWidget(m_showLegendCheck);
    
    m_controlsLayout->addStretch();
    
    // Stats label
    m_statsLabel = new QLabel("No data");
    m_controlsLayout->addWidget(m_statsLabel);
    
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
    m_currentType = static_cast<ChartType>(m_chartTypeCombo->currentIndex());
    
    // Show/hide bins spinner for histogram
    bool isHistogram = (m_currentType == Histogram);
    m_binsSpin->setVisible(isHistogram);
    m_binsSpin->parentWidget()->layout()->itemAt(
        m_controlsLayout->indexOf(m_binsSpin) - 1)->widget()->setVisible(isHistogram);
    
    // Update Y-axis visibility for pie chart
    bool showYAxis = (m_currentType != PieChart && m_currentType != Histogram);
    m_yAxisCombo->setVisible(showYAxis);
    m_controlsLayout->itemAt(m_controlsLayout->indexOf(m_yAxisCombo) - 1)->widget()->setVisible(showYAxis);
    
    onRefreshChart();
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
}

void ChartWidget::createBarChart()
{
    ChartManager manager;
    QString xColumn = m_xAxisCombo->currentText();
    QString yColumn = m_yAxisCombo->currentText();
    
    if (xColumn.isEmpty() || yColumn.isEmpty()) {
        return;
    }
    
    QString groupBy = m_groupByCombo->currentText() == "(None)" ? QString() : m_groupByCombo->currentText();
    
    ChartManager::AggregationType agg = static_cast<ChartManager::AggregationType>(m_aggregationCombo->currentIndex());
    
    auto chartData = manager.prepareBarChartData(m_data, xColumn, yColumn, groupBy, agg);
    
    if (chartData.yValues.isEmpty()) {
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
    m_chart->setTitle(QString("%1 by %2").arg(yColumn).arg(xColumn));
}

void ChartWidget::createLineChart()
{
    ChartManager manager;
    QString xColumn = m_xAxisCombo->currentText();
    QString yColumn = m_yAxisCombo->currentText();
    
    auto chartData = manager.prepareLineChartData(m_data, xColumn, yColumn);
    
    m_lineSeries = new QLineSeries();
    m_lineSeries->setName(yColumn);
    
    for (int i = 0; i < chartData.xValues.size(); ++i) {
        m_lineSeries->append(chartData.xValues[i], chartData.yValues[i]);
    }
    
    m_lineSeries->setColor(QColor(42, 130, 218));
    m_chart->addSeries(m_lineSeries);
    m_chart->setTitle(QString("%1 vs %2").arg(yColumn).arg(xColumn));
}

void ChartWidget::createScatterChart()
{
    ChartManager manager;
    QString xColumn = m_xAxisCombo->currentText();
    QString yColumn = m_yAxisCombo->currentText();
    
    auto chartData = manager.prepareScatterData(m_data, xColumn, yColumn);
    
    m_scatterSeries = new QScatterSeries();
    m_scatterSeries->setName(QString("%1 vs %2").arg(yColumn).arg(xColumn));
    
    for (int i = 0; i < chartData.xValues.size(); ++i) {
        m_scatterSeries->append(chartData.xValues[i], chartData.yValues[i]);
    }
    
    m_scatterSeries->setColor(QColor(42, 130, 218));
    m_scatterSeries->setMarkerSize(8);
    m_chart->addSeries(m_scatterSeries);
    m_chart->setTitle(QString("%1 vs %2").arg(yColumn).arg(xColumn));
}

void ChartWidget::createPieChart()
{
    ChartManager manager;
    QString labelColumn = m_xAxisCombo->currentText();
    QString valueColumn = m_yAxisCombo->currentText();
    
    ChartManager::AggregationType agg = static_cast<ChartManager::AggregationType>(m_aggregationCombo->currentIndex());
    if (agg == ChartManager::NoAggregation) {
        agg = ChartManager::Count;
    }
    
    auto chartData = manager.preparePieChartData(m_data, labelColumn, valueColumn, agg);
    
    m_pieSeries = new QPieSeries();
    
    for (int i = 0; i < chartData.xLabels.size(); ++i) {
        m_pieSeries->append(chartData.xLabels[i], chartData.yValues[i]);
    }
    
    m_chart->addSeries(m_pieSeries);
    m_chart->setTitle(QString("%1 Distribution").arg(labelColumn));
}

void ChartWidget::createHistogram()
{
    ChartManager manager;
    QString column = m_xAxisCombo->currentText();
    int bins = m_binsSpin->value();
    
    auto chartData = manager.prepareHistogramData(m_data, column, bins);
    
    m_barSeries = new QBarSeries();
    QBarSet *set = new QBarSet("Frequency");
    
    for (double value : chartData.yValues) {
        *set << value;
    }
    
    set->setColor(QColor(42, 130, 218));
    m_barSeries->append(set);
    m_chart->addSeries(m_barSeries);
    m_chart->setTitle(QString("Histogram of %1").arg(column));
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
    if (m_chart->series().isEmpty()) return;
    
    auto series = m_chart->series().first();
    
    // Remove existing axes
    for (auto axis : m_chart->axes()) {
        m_chart->removeAxis(axis);
    }
    
    // Create and configure axes based on chart type
    if (m_currentType == PieChart) {
        // Pie charts don't need axes
        return;
    }
    
    // Create appropriate axes
    if (m_currentType == BarChart || m_currentType == Histogram) {
        m_xCategoryAxis = new QBarCategoryAxis();
        m_yValueAxis = new QValueAxis();
        
        // Set category labels for bar chart
        if (m_currentType == BarChart) {
            ChartManager manager;
            QString xColumn = m_xAxisCombo->currentText();
            QString yColumn = m_yAxisCombo->currentText();
            auto chartData = manager.prepareBarChartData(m_data, xColumn, yColumn);
            m_xCategoryAxis->setCategories(chartData.xLabels);
        }
        
        m_chart->addAxis(m_xCategoryAxis, Qt::AlignBottom);
        m_chart->addAxis(m_yValueAxis, Qt::AlignLeft);
        series->attachAxis(m_xCategoryAxis);
        series->attachAxis(m_yValueAxis);
    } else {
        m_xValueAxis = new QValueAxis();
        m_yValueAxis = new QValueAxis();
        
        m_chart->addAxis(m_xValueAxis, Qt::AlignBottom);
        m_chart->addAxis(m_yValueAxis, Qt::AlignLeft);
        series->attachAxis(m_xValueAxis);
        series->attachAxis(m_yValueAxis);
    }
    
    // Style axes for dark theme
    if (m_xValueAxis) {
        m_xValueAxis->setLabelsColor(Qt::white);
        m_xValueAxis->setTitleText(m_xAxisCombo->currentText());
        m_xValueAxis->setTitleBrush(QBrush(Qt::white));
        m_xValueAxis->setGridLineVisible(m_showGridCheck->isChecked());
    }
    if (m_xCategoryAxis) {
        m_xCategoryAxis->setLabelsColor(Qt::white);
        m_xCategoryAxis->setTitleText(m_xAxisCombo->currentText());
        m_xCategoryAxis->setTitleBrush(QBrush(Qt::white));
        m_xCategoryAxis->setGridLineVisible(m_showGridCheck->isChecked());
    }
    if (m_yValueAxis) {
        m_yValueAxis->setLabelsColor(Qt::white);
        m_yValueAxis->setTitleText(m_yAxisCombo->currentText());
        m_yValueAxis->setTitleBrush(QBrush(Qt::white));
        m_yValueAxis->setGridLineVisible(m_showGridCheck->isChecked());
    }
}

void ChartWidget::onExportChart()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Chart"),
        QString("chart.png"),
        tr("PNG Images (*.png);;SVG Images (*.svg);;PDF Files (*.pdf)")
    );
    
    if (!fileName.isEmpty()) {
        QPixmap pixmap = m_chartView->grab();
        if (pixmap.save(fileName)) {
            QMessageBox::information(this, tr("Export Successful"), 
                                   tr("Chart exported to %1").arg(fileName));
        } else {
            QMessageBox::warning(this, tr("Export Failed"), 
                                tr("Failed to export chart to %1").arg(fileName));
        }
    }
}