#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <QObject>
#include <QFileSystemModel>
#include <QModelIndex>

class FileBrowser : public QObject
{
    Q_OBJECT

public:
    explicit FileBrowser(QObject *parent = nullptr);
    
    QFileSystemModel* getModel() const { return m_model; }
    QModelIndex getRootIndex() const;
    
    void setRootPath(const QString &path);
    QString getCurrentPath() const;

signals:
    void fileSelected(const QString &filePath);

private slots:
    void onModelDataChanged();

private:
    QFileSystemModel *m_model;
    QString m_rootPath;
};

#endif // FILEBROWSER_H