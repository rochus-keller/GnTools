/*
* Copyright 2019 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the GN Viewer application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the application under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include "GnCodeBrowser.h"
#include "GnHighlighter.h"
#include "GnCodeModel.h"
#include "GnSynTree.h"
#include <QApplication>
#include <QtDebug>
using namespace Gn;

CodeBrowser::CodeBrowser(CodeModel* mdl, QWidget* p):QPlainTextEdit(p),d_goto(0),d_mdl(mdl),d_pushBackLock(false),d_cur(0)
{
    setReadOnly(true);
    setLineWrapMode( QPlainTextEdit::NoWrap );
    setTabStopWidth( 30 );
    setTabChangesFocus(true);
    setMouseTracking(true);
    new Highlighter( mdl, document() );
    QFont f;
    f.setStyleHint( QFont::TypeWriter );
    f.setFamily("Mono");
    f.setPointSize(9);
    setFont(f);
}

void CodeBrowser::clear()
{
    QPlainTextEdit::clear();
    d_sourcePath.clear();
    d_cur = 0;
    d_backHisto.clear();
    d_forwardHisto.clear();
    d_link.clear();
    d_goto = 0;
    d_nonTerms.clear();
    d_find.clear();
}

bool CodeBrowser::loadFile(const QByteArray& path)
{
    if( d_sourcePath == path )
        return true;
    d_sourcePath = path;
    QFile in(QString::fromUtf8(d_sourcePath));
    if( !in.open(QIODevice::ReadOnly) )
        return false;
    setPlainText( QString::fromLatin1(in.readAll()) );
    emit sigShowFile(path);
    return true;
}

void CodeBrowser::mouseMoveEvent(QMouseEvent* e)
{
    QPlainTextEdit::mouseMoveEvent(e);
    if( QApplication::keyboardModifiers() == Qt::ControlModifier )
    {
        QTextCursor cur = cursorForPosition(e->pos());
        SynTree* id = d_mdl->findSymbolBySourcePos(d_sourcePath,cur.blockNumber() + 1,
                                                                      cur.positionInBlock() + 1);
        const bool alreadyArrow = !d_link.isEmpty();
        d_link.clear();
        if( id )
        {
            const int off = cur.positionInBlock() + 1 - id->d_tok.d_colNr;
            cur.setPosition(cur.position() - off);
            cur.setPosition( cur.position() + id->d_tok.d_len, QTextCursor::KeepAnchor );
            d_goto = d_mdl->findDefinition(id);
            if( d_goto )
            {
                QTextEdit::ExtraSelection sel;
                sel.cursor = cur;
                sel.format.setFontUnderline(true);
                d_link << sel;
                if( !alreadyArrow )
                    QApplication::setOverrideCursor(Qt::ArrowCursor);
            }
        }
        if( alreadyArrow && d_link.isEmpty() )
            QApplication::restoreOverrideCursor();
        updateExtraSelections();
    }else if( !d_link.isEmpty() )
    {
        QApplication::restoreOverrideCursor();
        d_link.clear();
        updateExtraSelections();
    }
}

void CodeBrowser::mousePressEvent(QMouseEvent* e)
{
    QTextCursor cur = cursorForPosition(e->pos());
    d_cur = d_mdl->findSymbolBySourcePos(d_sourcePath,cur.blockNumber() + 1,cur.positionInBlock() + 1);
    pushLocation( d_cur );
    if( !d_link.isEmpty() )
    {
        QApplication::restoreOverrideCursor();
        d_link.clear();
        setCursorPosition( d_goto, true, true );
    }else
    {
        QPlainTextEdit::mousePressEvent(e); // erst hier da ansonsten im oberen Code durch Mode selektiert wird
        if( d_cur && QApplication::keyboardModifiers() == Qt::ControlModifier )
        {
            //pushLocation( Location( cur.blockNumber(), cur.positionInBlock() ) );
            d_goto = d_mdl->findDefinition(d_cur);
            if( d_goto )
                setCursorPosition( d_goto, true, true );
            else
                updateExtraSelections();
        }else
            updateExtraSelections();
    }
}

void CodeBrowser::updateExtraSelections()
{
    ESL sum;

    QTextEdit::ExtraSelection line;
    line.format.setBackground(QColor(Qt::yellow).lighter(190));
    line.format.setProperty(QTextFormat::FullWidthSelection, true);
    line.cursor = textCursor();
    line.cursor.clearSelection();
    sum << line;

    if( d_cur )
    {
        QTextEdit::ExtraSelection line;
        line.format.setBackground(QColor(Qt::yellow).lighter(120));
        line.cursor = textCursor();
        line.cursor.setPosition(document()->findBlockByNumber(d_cur->d_tok.d_lineNr - 1).position() +
                                d_cur->d_tok.d_colNr - 1 );
        line.cursor.setPosition( line.cursor.position() + d_cur->d_tok.d_len, QTextCursor::KeepAnchor);
        sum << line;
    }

    sum << d_nonTerms;

    sum << d_link;

    setExtraSelections(sum);
}

void CodeBrowser::pushLocation(SynTree* st)
{
    if( d_pushBackLock || st == 0 )
        return;
    if( !d_backHisto.isEmpty() && d_backHisto.last() == st )
        return; // o ist bereits oberstes Element auf dem Stack.
    d_backHisto.removeAll( st );
    d_backHisto.push_back( st );
    d_cur = st;
}

void CodeBrowser::goBack()
{
    if( d_backHisto.size() <= 1 )
        return;

    d_pushBackLock = true;
    d_forwardHisto.push_back( d_backHisto.last() );
    d_backHisto.pop_back();
    d_cur = d_backHisto.last();
    setCursorPosition( d_cur, true );
    d_pushBackLock = false;
}

void CodeBrowser::goForward()
{
    if( d_forwardHisto.isEmpty() )
        return;

    d_cur = d_forwardHisto.last();
    d_backHisto.push_back( d_cur );
    d_forwardHisto.pop_back();
    setCursorPosition( d_cur, true );
}

void CodeBrowser::setCursorPosition(SynTree* id, bool center, bool pushLoc)
{
    id = CodeModel::firstToken(id);
    d_cur = id;
    const int line = id->d_tok.d_lineNr - 1;
    const int col = id->d_tok.d_colNr;
    loadFile( id->d_tok.d_sourcePath );
    // Qt-Koordinaten
    if( line >= 0 && line < document()->blockCount() )
    {
        QTextBlock block = document()->findBlockByNumber(line);
        QTextCursor cur = textCursor();
        cur.setPosition( block.position() + col );
        setTextCursor( cur );
        if( center )
            centerCursor();
        else
            ensureCursorVisible();
        if( pushLoc )
            pushLocation( id );
        updateExtraSelections();
    }
}

void CodeBrowser::setCursorPosition(int line, int col, bool center, int sel)
{
    // Qt-Koordinaten
    if( line >= 0 && line < document()->blockCount() )
    {
        QTextBlock block = document()->findBlockByNumber(line);
        QTextCursor cur = textCursor();
        cur.setPosition( block.position() + col );
        if( sel > 0 )
            cur.setPosition( block.position() + col + sel, QTextCursor::KeepAnchor );
        setTextCursor( cur );
        if( center )
            centerCursor();
        else
            ensureCursorVisible();
        updateExtraSelections();
    }
}

void CodeBrowser::markNonTermsFromCursor()
{
    QTextCursor cur = textCursor();
    /*
    CodeModel::IdentUse id = d_mdl->findSymbolBySourcePos(d_path,cur.blockNumber() + 1,positionInBlock(cur) + 1);
    if( id.first )
    {
        QList<const SynTree*> syms = d_mdl->findReferencingSymbols( id.second, d_path );
        markNonTerms(syms);
    }
    */
}

void CodeBrowser::markNonTerms(const QList<const SynTree*>& s)
{
    d_nonTerms.clear();
    QTextCharFormat format;
    format.setBackground( QColor(247,245,243).darker(120) );
    foreach( const SynTree* n, s )
    {
        QTextCursor c( document()->findBlockByNumber( n->d_tok.d_lineNr - 1) );
        c.setPosition( c.position() + n->d_tok.d_colNr - 1 );
        c.setPosition( c.position() + n->d_tok.d_val.size(), QTextCursor::KeepAnchor );

        QTextEdit::ExtraSelection sel;
        sel.format = format;
        sel.cursor = c;

        d_nonTerms << sel;
    }
    updateExtraSelections();
}

void CodeBrowser::find(const QString& str, bool fromTop)
{
    d_find = str;
    find(fromTop);
}

void CodeBrowser::findAgain()
{
    find(false);
}

void CodeBrowser::find(bool fromTop)
{
    QTextCursor cur = textCursor();
    int line = cur.block().blockNumber();
    int col = cur.positionInBlock();

    if( fromTop )
    {
        line = 0;
        col = 0;
    }else
        col++;
    const int count = document()->blockCount();
    int pos = -1;
    const int start = qMax(line,0);
    bool turnedAround = false;
    for( int i = start; i < count; i++ )
    {
        pos = document()->findBlockByNumber(i).text().indexOf( d_find, col, Qt::CaseInsensitive );
        if( pos != -1 )
        {
            line = i;
            col = pos;
            break;
        }else if( i < count )
            col = 0;
        if( pos == -1 && start != 0 && !turnedAround && i == count - 1 )
        {
            turnedAround = true;
            i = -1;
        }
    }
    if( pos != -1 )
    {
        setCursorPosition( line, col, true, d_find.size() );
    }
}
