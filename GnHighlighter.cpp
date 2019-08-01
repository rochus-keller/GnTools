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

#include "GnHighlighter.h"
#include "GnLexer.h"
#include "GnCodeModel.h"
using namespace Gn;

Highlighter::Highlighter(CodeModel* mdl, QTextDocument* parent) :
    QSyntaxHighlighter(parent),d_mdl(mdl)
{
    Q_ASSERT( mdl != 0 );
    for( int i = 0; i < C_Max; i++ )
    {
        d_format[i].setFontWeight(QFont::Normal);
        d_format[i].setForeground(Qt::black);
        d_format[i].setBackground(Qt::transparent);
    }
    d_format[C_Num].setForeground(QColor(0, 153, 153));
    const QColor brown( 147, 39, 39 );
    d_format[C_Str].setForeground(QColor(208, 16, 64));
    d_format[C_Dollar].setForeground(Qt::darkRed); // brown);
    d_format[C_Cmt].setForeground(QColor(153, 153, 136));
    d_format[C_Kw].setForeground(QColor(68, 85, 136));
    d_format[C_Kw].setFontWeight(QFont::Bold);
    d_format[C_Op].setForeground(QColor(68, 85, 136)); // QColor(153, 0, 0));
    d_format[C_Op].setFontWeight(QFont::Bold);
    d_format[C_Known].setForeground(QColor(153, 0, 115));
    //d_format[C_Known].setFontWeight(QFont::Bold);

}

QTextCharFormat Highlighter::formatForCategory(int c) const
{
    return d_format[c];
}

void Highlighter::highlightBlock(const QString& text)
{
    const int previousBlockState_ = previousBlockState();
    int lexerState = 0, initialBraceDepth = 0;
    if (previousBlockState_ != -1) {
        lexerState = previousBlockState_ & 0xff;
        initialBraceDepth = previousBlockState_ >> 8;
    }

    int braceDepth = initialBraceDepth;


    int start = 0;
    if( lexerState > 0 )
    {
        // wir sind in einem Multi Line Comment
        // suche das Ende
        QTextCharFormat f = formatForCategory(C_Cmt);
        f.setProperty( TokenProp, int(Tok_Comment) );
        int pos = 0;
        //Lexer::parseComment( text.toLatin1(), pos, lexerState );
        if( lexerState > 0 )
        {
            // the whole block ist part of the comment
            setFormat( start, text.size(), f );
            setCurrentBlockState( (braceDepth << 8) | lexerState);
            return;
        }else
        {
            // End of Comment found
            setFormat( start, pos , f );
            lexerState = 0;
            braceDepth--;
            start = pos;
        }
    }


    Gn::Lexer lex;
    lex.setIgnoreComments(false);
    lex.setPackComments(false);

    QList<Token> tokens =  lex.tokens(text.mid(start));
    for( int i = 0; i < tokens.size(); ++i )
    {
        Token &t = tokens[i];
        t.d_colNr += start;

        QTextCharFormat f;
        if( t.d_type == Tok_Comment )
            f = formatForCategory(C_Cmt); // one line comment
        else if( t.d_type == Tok_string )
            f = formatForCategory(C_Str);
        else if( t.d_type == Tok_integer )
            f = formatForCategory(C_Num);
        else if( tokenTypeIsLiteral(t.d_type) )
        {
            f = formatForCategory(C_Op);
        }else if( tokenTypeIsKeyword(t.d_type) )
        {
            f = formatForCategory(C_Kw);
        }else if( t.d_type == Tok_identifier )
        {
            if( d_mdl->isKnownId(t.d_val) )
                f = formatForCategory(C_Known);
            else
                f = formatForCategory(C_Ident);
        }

        if( f.isValid() )
        {
            setFormat( t.d_colNr-1, t.d_len, f );
            if( t.d_type == Tok_string )
            {
                CodeModel::Dollars dd = CodeModel::findDollars( t.d_val );
                foreach( CodeModel::Dollar d, dd )
                {
                    d.d_pos++;
                    d.d_len--;
                    if( t.d_val[d.d_pos] == '{' )
                    {
                        d.d_pos++;
                        d.d_len -= 2;
                    }else if( t.d_val[d.d_pos] == '0' )
                        continue;
                    // TODO: anscheinend ist auch ScopeAccess zulässig!
                    if( d_mdl->isKnownObj( Lexer::getSymbol(t.d_val.mid(d.d_pos, d.d_len ) ) ) )
                        f = formatForCategory(C_Known);
                    else
                        f = formatForCategory(C_Ident);
                    //f = formatForCategory(C_Dollar);
                    setFormat( t.d_colNr + d.d_pos - 1, d.d_len, f );
                }
            }
        }
    }

    setCurrentBlockState((braceDepth << 8) | lexerState );
}



LogPainter::LogPainter(QTextDocument* parent):QSyntaxHighlighter(parent)
{

}

void LogPainter::highlightBlock(const QString& text)
{
    QColor c = Qt::black;
    if( text.startsWith("WRN:") )
        c = Qt::blue;
    else if( text.startsWith("ERR:") )
        c = Qt::red;

    setFormat( 0, text.size(), c );
}
