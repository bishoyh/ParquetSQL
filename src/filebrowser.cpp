#include "filebrowser.h"
#include <QDir>
#include <QStandardPaths>

FileBrowser::FileBrowser(QObject *parent)
    : QObject(parent)
    , m_model(new QFileSystemModel(this))
{
    QStringList filters;
    filters << "*.parquet" << "*.csv" << "*.tsv";
    m_model->setNameFilters(filters);
    m_model->setNameFilterDisables(false);
    
    m_rootPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    m_model->setRootPath(m_rootPath);
    
    connect(m_model, &QFileSystemModel::dataChanged,
            this, &FileBrowser::onModelDataChanged);
}

QModelIndex FileBrowser::getRootIndex() const
{
    return m_model->index(m_rootPath);
}

void FileBrowser::setRootPath(const QString &path)
{
    if (QDir(path).exists()) {
        m_rootPath = path;
        m_model->setRootPath(m_rootPath);
    }
}

QString FileBrowser::getCurrentPath() const
{
    return m_rootPath;
}

void FileBrowser::onModelDataChanged()
{
}