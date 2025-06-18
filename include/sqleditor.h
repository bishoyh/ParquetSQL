#ifndef SQLEDITOR_H
#define SQLEDITOR_H

#include <QTextEdit>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QCompleter>
#include <QRegularExpression>

class SQLSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SQLSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat numberFormat;
};

class SQLEditor : public QTextEdit
{
    Q_OBJECT

public:
    explicit SQLEditor(QWidget *parent = nullptr);
    
    QString getQuery() const;
    void setQuery(const QString &query);
    void clear();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void insertFromMimeData(const QMimeData *source) override;

private slots:
    void insertCompletion(const QString &completion);
    void onTextChanged();

private:
    void setupEditor();
    void setupCompleter();
    QString textUnderCursor() const;
    
    SQLSyntaxHighlighter *m_highlighter;
    QCompleter *m_completer;
    QStringList m_sqlKeywords;
};

#endif // SQLEDITOR_H