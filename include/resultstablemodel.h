#ifndef RESULTSTABLEMODEL_H
#define RESULTSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QVariantList>
#include "duckdbmanager.h"

class ResultsTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ResultsTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    
    void setResults(const DuckDBManager::QueryResult &results);
    void clear();
    
    int getCurrentPage() const { return m_currentPage; }
    int getTotalPages() const;
    int getTotalRows() const { return m_totalRows; }
    int getRowsPerPage() const { return m_rowsPerPage; }
    
    void setCurrentPage(int page);
    void setRowsPerPage(int rowsPerPage);

signals:
    void pageChanged(int currentPage, int totalPages);

private:
    void updateVisibleData();
    QString formatValue(const QVariant &value) const;
    
    QStringList m_columnNames;
    QList<QVariantList> m_allData;
    QList<QVariantList> m_visibleData;
    
    int m_currentPage;
    int m_rowsPerPage;
    int m_totalRows;
    
    static constexpr int DEFAULT_ROWS_PER_PAGE = 1000;
};

#endif // RESULTSTABLEMODEL_H