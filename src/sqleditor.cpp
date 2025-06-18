#include "sqleditor.h"
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QStringListModel>
#include <QMimeData>
#include <QFont>
#include <QRegularExpression>

SQLSyntaxHighlighter::SQLSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(QColor(0, 0, 255));
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bSELECT\\b" << "\\bFROM\\b" << "\\bWHERE\\b"
                    << "\\bINSERT\\b" << "\\bUPDATE\\b" << "\\bDELETE\\b"
                    << "\\bCREATE\\b" << "\\bDROP\\b" << "\\bALTER\\b"
                    << "\\bTABLE\\b" << "\\bINDEX\\b" << "\\bVIEW\\b"
                    << "\\bJOIN\\b" << "\\bINNER\\b" << "\\bLEFT\\b"
                    << "\\bRIGHT\\b" << "\\bOUTER\\b" << "\\bON\\b"
                    << "\\bGROUP\\b" << "\\bBY\\b" << "\\bORDER\\b"
                    << "\\bHAVING\\b" << "\\bLIMIT\\b" << "\\bOFFSET\\b"
                    << "\\bUNION\\b" << "\\bAND\\b" << "\\bOR\\b"
                    << "\\bNOT\\b" << "\\bIS\\b" << "\\bNULL\\b"
                    << "\\bDISTINCT\\b" << "\\bASC\\b" << "\\bDESC\\b"
                    << "\\bIN\\b" << "\\bLIKE\\b" << "\\bBETWEEN\\b"
                    << "\\bEXISTS\\b" << "\\bAS\\b" << "\\bCASE\\b"
                    << "\\bWHEN\\b" << "\\bTHEN\\b" << "\\bELSE\\b"
                    << "\\bEND\\b";

    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    functionFormat.setForeground(QColor(128, 0, 128));
    functionFormat.setFontWeight(QFont::Bold);
    QStringList functionPatterns;
    functionPatterns << "\\bCOUNT\\b" << "\\bSUM\\b" << "\\bAVG\\b"
                     << "\\bMIN\\b" << "\\bMAX\\b" << "\\bLEN\\b"
                     << "\\bUPPER\\b" << "\\bLOWER\\b" << "\\bTRIM\\b"
                     << "\\bSUBSTR\\b" << "\\bREPLACE\\b" << "\\bCONCAT\\b";

    foreach (const QString &pattern, functionPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = functionFormat;
        highlightingRules.append(rule);
    }

    stringFormat.setForeground(QColor(0, 128, 0));
    rule.pattern = QRegularExpression("'([^'\\\\]|\\\\.)*'");
    rule.format = stringFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\"([^\"\\\\]|\\\\.)*\"");
    rule.format = stringFormat;
    highlightingRules.append(rule);

    commentFormat.setForeground(QColor(128, 128, 128));
    commentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("--[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("/\\*.*\\*/");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    numberFormat.setForeground(QColor(255, 140, 0));
    rule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);
}

void SQLSyntaxHighlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

SQLEditor::SQLEditor(QWidget *parent)
    : QTextEdit(parent)
    , m_highlighter(nullptr)
    , m_completer(nullptr)
{
    setupEditor();
    setupCompleter();
    
    m_highlighter = new SQLSyntaxHighlighter(document());
    
    connect(this, &QTextEdit::textChanged, this, &SQLEditor::onTextChanged);
}

void SQLEditor::setupEditor()
{
    QFont font = QFont("Monaco", 10);
    if (!font.exactMatch()) {
        font = QFont("Courier New", 10);
    }
    font.setFixedPitch(true);
    setFont(font);
    
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
    setLineWrapMode(QTextEdit::NoWrap);
    
    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, QColor(248, 248, 248));
    setPalette(palette);
}

void SQLEditor::setupCompleter()
{
    m_sqlKeywords << "SELECT" << "FROM" << "WHERE" << "INSERT" << "UPDATE"
                  << "DELETE" << "CREATE" << "DROP" << "ALTER" << "TABLE"
                  << "INDEX" << "VIEW" << "JOIN" << "INNER" << "LEFT"
                  << "RIGHT" << "OUTER" << "ON" << "GROUP" << "BY"
                  << "ORDER" << "HAVING" << "LIMIT" << "OFFSET" << "UNION"
                  << "AND" << "OR" << "NOT" << "IS" << "NULL" << "DISTINCT"
                  << "ASC" << "DESC" << "IN" << "LIKE" << "BETWEEN"
                  << "EXISTS" << "AS" << "CASE" << "WHEN" << "THEN"
                  << "ELSE" << "END" << "COUNT" << "SUM" << "AVG"
                  << "MIN" << "MAX" << "LEN" << "UPPER" << "LOWER"
                  << "TRIM" << "SUBSTR" << "REPLACE" << "CONCAT";

    m_completer = new QCompleter(m_sqlKeywords, this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setWidget(this);
    
    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &SQLEditor::insertCompletion);
}

QString SQLEditor::getQuery() const
{
    return toPlainText();
}

void SQLEditor::setQuery(const QString &query)
{
    setPlainText(query);
}

void SQLEditor::clear()
{
    QTextEdit::clear();
}

void SQLEditor::keyPressEvent(QKeyEvent *event)
{
    if (m_completer && m_completer->popup()->isVisible()) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            event->ignore();
            return;
        default:
            break;
        }
    }

    bool isShortcut = ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_Space);
    if (!m_completer || !isShortcut)
        QTextEdit::keyPressEvent(event);

    const bool ctrlOrShift = event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (!m_completer || (ctrlOrShift && event->text().isEmpty()))
        return;

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-=");
    bool hasModifier = (event->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();

    if (!isShortcut && (hasModifier || event->text().isEmpty() || completionPrefix.length() < 2
                      || eow.contains(event->text().right(1)))) {
        m_completer->popup()->hide();
        return;
    }

    if (completionPrefix != m_completer->completionPrefix()) {
        m_completer->setCompletionPrefix(completionPrefix);
        m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    }
    
    QRect cr = cursorRect();
    cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
                + m_completer->popup()->verticalScrollBar()->sizeHint().width());
    m_completer->complete(cr);
}

void SQLEditor::insertCompletion(const QString &completion)
{
    if (m_completer->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - m_completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

QString SQLEditor::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void SQLEditor::onTextChanged()
{
}

void SQLEditor::insertFromMimeData(const QMimeData *source)
{
    if (source->hasText()) {
        QString text = source->text();
        insertPlainText(text);
    } else {
        QTextEdit::insertFromMimeData(source);
    }
}