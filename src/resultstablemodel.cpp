#include "resultstablemodel.h"
#include <QFont>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QRegularExpression>

ResultsTableModel::ResultsTableModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_currentPage(0)
    , m_rowsPerPage(DEFAULT_ROWS_PER_PAGE)
    , m_totalRows(0)
{
}

int ResultsTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_visibleData.size();
}

int ResultsTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_columnNames.size();
}

QVariant ResultsTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_visibleData.size() ||
        index.column() >= m_columnNames.size()) {
        return QVariant();
    }

    const QVariantList &row = m_visibleData[index.row()];
    if (index.column() >= row.size()) {
        return QVariant();
    }

    const QVariant &value = row[index.column()];

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return formatValue(value);
        
    case Qt::TextAlignmentRole:
        if (value.typeId() == QMetaType::Int || value.typeId() == QMetaType::LongLong ||
            value.typeId() == QMetaType::Double || value.typeId() == QMetaType::ULongLong) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        
    case Qt::BackgroundRole:
        if (index.row() % 2 == 0) {
            return QBrush(QColor(248, 248, 248));
        }
        return QBrush(Qt::white);
        
    case Qt::FontRole: {
        QFont font;
        if (value.isNull()) {
            font.setItalic(true);
        }
        return font;
    }
    
    case Qt::ForegroundRole:
        if (value.isNull()) {
            return QBrush(QColor(128, 128, 128));
        }
        return QBrush(Qt::black);
        
    default:
        return QVariant();
    }
}

QVariant ResultsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole ||
        section >= m_columnNames.size()) {
        return QVariant();
    }

    return m_columnNames[section];
}

Qt::ItemFlags ResultsTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void ResultsTableModel::setResults(const DuckDBManager::QueryResult &results)
{
    beginResetModel();
    
    m_columnNames = results.columnNames;
    m_allData = results.rows;
    m_totalRows = results.totalRows;
    m_currentPage = 0;
    
    updateVisibleData();
    
    endResetModel();
    
    emit pageChanged(m_currentPage, getTotalPages());
}

void ResultsTableModel::clear()
{
    beginResetModel();
    
    m_columnNames.clear();
    m_allData.clear();
    m_visibleData.clear();
    m_totalRows = 0;
    m_currentPage = 0;
    
    endResetModel();
    
    emit pageChanged(0, 0);
}

int ResultsTableModel::getTotalPages() const
{
    if (m_totalRows == 0 || m_rowsPerPage == 0) {
        return 0;
    }
    return (m_totalRows + m_rowsPerPage - 1) / m_rowsPerPage;
}

void ResultsTableModel::setCurrentPage(int page)
{
    int totalPages = getTotalPages();
    if (page < 0 || page >= totalPages) {
        return;
    }
    
    if (m_currentPage != page) {
        beginResetModel();
        m_currentPage = page;
        updateVisibleData();
        endResetModel();
        
        emit pageChanged(m_currentPage, totalPages);
    }
}

void ResultsTableModel::setRowsPerPage(int rowsPerPage)
{
    if (rowsPerPage <= 0 || m_rowsPerPage == rowsPerPage) {
        return;
    }
    
    beginResetModel();
    
    int oldTotalPages = getTotalPages();
    m_rowsPerPage = rowsPerPage;
    int newTotalPages = getTotalPages();
    
    if (m_currentPage >= newTotalPages) {
        m_currentPage = qMax(0, newTotalPages - 1);
    }
    
    updateVisibleData();
    
    endResetModel();
    
    emit pageChanged(m_currentPage, newTotalPages);
}

void ResultsTableModel::updateVisibleData()
{
    m_visibleData.clear();
    
    if (m_allData.isEmpty() || m_rowsPerPage <= 0) {
        return;
    }
    
    int startRow = m_currentPage * m_rowsPerPage;
    int endRow = qMin(startRow + m_rowsPerPage, m_allData.size());
    
    for (int i = startRow; i < endRow; ++i) {
        m_visibleData.append(m_allData[i]);
    }
}

QString ResultsTableModel::formatValue(const QVariant &value) const
{
    if (value.isNull()) {
        return "<NULL>";
    }
    
    switch (value.typeId()) {
    case QMetaType::Double: {
        double d = value.toDouble();
        if (d == static_cast<int>(d)) {
            return QString::number(static_cast<int>(d));
        }
        return QString::number(d, 'f', 6).remove(QRegularExpression("\\.?0+$"));
    }
    case QMetaType::QString: {
        QString str = value.toString();
        if (str.length() > 200) {
            return str.left(200) + "...";
        }
        return str;
    }
    case QMetaType::Bool:
        return value.toBool() ? "true" : "false";
    default:
        return value.toString();
    }
}