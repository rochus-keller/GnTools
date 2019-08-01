/*
* Copyright 2019 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the GN parser library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
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

#include "GnLexer.h"
#include "GnErrors.h"
#include "GnFileCache.h"
#include <QBuffer>
#include <QFile>
#include <QIODevice>
using namespace Gn;

QHash<QByteArray,QByteArray> Lexer::d_symbols;

Lexer::Lexer(QObject *parent) : QObject(parent),
    d_lastToken(Tok_Invalid),d_lineNr(0),d_colNr(0),d_in(0),d_err(0),d_fcache(0),
    d_ignoreComments(true), d_packComments(true),d_real("\\d*\\.?\\d+(e[-+]?\\d+)?")
{

}

void Lexer::setStream(QIODevice* in, const QString& sourcePath)
{
    if( in == 0 )
        setStream( sourcePath );
    else
    {
        d_in = in;
        d_lineNr = 0;
        d_colNr = 0;
        d_sourcePath = getSymbol(sourcePath.toUtf8());
        d_lastToken = Tok_Invalid;
    }
}

bool Lexer::setStream(const QString& sourcePath)
{
    QIODevice* in = 0;

    if( d_fcache )
    {
        bool found;
        QByteArray content = d_fcache->getFile(sourcePath, &found );
        if( found )
        {
            QBuffer* buf = new QBuffer(this);
            buf->setData( content );
            buf->open(QIODevice::ReadOnly);
            in = buf;
        }
    }

    if( in == 0 )
    {
        QFile* file = new QFile(sourcePath, this);
        if( !file->open(QIODevice::ReadOnly) )
        {
            if( d_err )
            {
                d_err->error(Errors::Lexer, sourcePath, 0, 0,
                                 tr("cannot open file from path %1").arg(sourcePath) );
            }
            delete file;
            return false;
        }
        in = file;
    }
    // else
    setStream( in, sourcePath );
    return true;
}

Token Lexer::nextToken()
{
    Token t;
    if( !d_buffer.isEmpty() )
    {
        t = d_buffer.first();
        d_buffer.pop_front();
    }else
        t = nextTokenImp();
    if( t.d_type == Tok_Comment && d_ignoreComments )
        t = nextToken();
    return t;
}

Token Lexer::peekToken(quint8 lookAhead)
{
    Q_ASSERT( lookAhead > 0 );
    while( d_buffer.size() < lookAhead )
        d_buffer.push_back( nextTokenImp() );
    return d_buffer[ lookAhead - 1 ];
}

QList<Token> Lexer::tokens(const QString& code)
{
    return tokens( code.toLatin1() );
}

QList<Token> Lexer::tokens(const QByteArray& code, const QString& path)
{
    QBuffer in;
    in.setData( code );
    in.open(QIODevice::ReadOnly);
    setStream( &in, path );

    QList<Token> res;
    Token t = nextToken();
    while( t.isValid() )
    {
        res << t;
        t = nextToken();
    }
    return res;
}

QByteArray Lexer::getSymbol(const QByteArray& str)
{
    if( str.isEmpty() )
        return str;
    QByteArray& sym = d_symbols[str];
    if( sym.isEmpty() )
        sym = str;
    return sym;
}

void Lexer::clearSymbols()
{
    d_symbols.clear();
}

bool Lexer::isValidIdent(const QByteArray& id)
{
    if( id.isEmpty() )
        return false;
    if( !::isalpha(id[0]) && id[0] != '_' )
        return false;
    for( int i = 1; i < id.size(); i++ )
    {
        if( !::isalnum(id[i]) && id[i] != '_' )
            return false;
    }
    return true;
}

static inline bool isCharacter(char ch, bool withDigit = true )
{
    return ( withDigit ? ::isalnum(ch) : ::isalpha(ch) ) || ch == '_' || ch == '$' || ch == '\\';
}

Token Lexer::nextTokenImp()
{
    if( d_in == 0 )
        return token(Tok_Eof);
    skipWhiteSpace();

    while( d_colNr >= d_line.size() )
    {
        if( d_in->atEnd() )
        {
            Token t = token( Tok_Eof, 0 );
            if( d_in->parent() == this )
                d_in->deleteLater();
            return t;
        }
        nextLine();
        skipWhiteSpace();
    }
    Q_ASSERT( d_colNr < d_line.size() );
    while( d_colNr < d_line.size() )
    {
        const char ch = quint8(d_line[d_colNr]);

        if( ch == '"' )
            return string();
        else if( ::isalpha(ch) || ch == '_' )
            return ident();
        else if( ::isdigit(ch) )
            return number();
        // else
        int pos = d_colNr;
        TokenType tt = tokenTypeFromString(d_line,&pos);

        if( tt == Tok_Hash )
            return lineComment();
        else if( tt == Tok_Invalid || pos == d_colNr )
            return token( Tok_Invalid, 1, QString("unexpected character '%1' %2").arg(char(ch)).arg(int(ch)).toUtf8() );
        else {
            const int len = pos - d_colNr;
            return token( tt, len, d_line.mid(d_colNr,len) );
        }
    }
    Q_ASSERT(false);
    return token(Tok_Invalid);
}

int Lexer::skipWhiteSpace()
{
    const int colNr = d_colNr;
    while( d_colNr < d_line.size() && ::isspace( d_line[d_colNr] ) )
        d_colNr++;
    return d_colNr - colNr;
}

void Lexer::nextLine()
{
    d_colNr = 0;
    d_lineNr++;
    d_line = d_in->readLine();

    if( d_line.endsWith("\r\n") )
        d_line.chop(2);
    else if( d_line.endsWith('\n') || d_line.endsWith('\r') || d_line.endsWith('\025') )
        d_line.chop(1);
}

int Lexer::lookAhead(int off) const
{
    if( int( d_colNr + off ) < d_line.size() )
    {
        return d_line[ d_colNr + off ];
    }else
        return 0;
}

Token Lexer::token(TokenType tt, int len, const QByteArray& val)
{
    QByteArray v = val;
    if( tt == Tok_identifier )
        v = getSymbol(v);
    Token t( tt, d_lineNr, d_colNr + 1, len, v );
    d_lastToken = t;
    d_colNr += len;
    t.d_sourcePath = d_sourcePath; // sourcePath is symbol too
    if( tt == Tok_Invalid && d_err != 0 )
        d_err->error(Errors::Syntax, t.d_sourcePath, t.d_lineNr, t.d_colNr, t.d_val );
    return t;
}

static inline bool isOnlyDecimalDigit( const QByteArray& str )
{
    for( int i = 0; i < str.size(); i++ )
    {
        if( !::isdigit(str[i]) )
            return false;
    }
    return true;
}

Token Lexer::ident()
{
    int off = 1;
    while( true )
    {
        const char c = lookAhead(off);
        if( !::isalnum(c) && c != '_' )
            break;
        else
            off++;
    }
    const QByteArray str = d_line.mid(d_colNr, off );
    Q_ASSERT( !str.isEmpty() );

    int pos = 0;
    TokenType t = tokenTypeFromString( str, &pos );
    if( t != Tok_Invalid && pos != str.size() )
        t = Tok_Invalid;

    if( t != Tok_Invalid )
        return token( t, off );
    else
        return token( Tok_identifier, off, str );
}

Token Lexer::number()
{
    int off = 1;
    while( true )
    {
        const char c = lookAhead(off);
        if( !::isdigit(c) )
            break;
        else
            off++;
    }
    const QByteArray str = d_line.mid(d_colNr, off );
    Q_ASSERT( !str.isEmpty() );
    return token( Tok_integer, off, str );
}

Token Lexer::lineComment()
{
    return token( Tok_Comment, d_line.size() - d_colNr, d_line.mid(d_colNr + 2).trimmed() );
}

Token Lexer::string()
{
    int off = 1;
    bool escaped = false;
    while( true )
    {
        const char c = lookAhead(off);
        off++;
        if( c == 0 )
            return token( Tok_Invalid, off, "non-terminated string" );
        else if( c == '\\' )
        {
            // ignore
            const char c2 = lookAhead(off);
            if( c2 == '"' || c2 == '\\' || c2 == '$' )
                escaped = true;
        }else if( c == '"' && !escaped )
            break;
        else
            escaped = false;
    }
    // wir lassen den String hier original damit die Offsets übereinstimmen mit dem was der Highligher im Code sieht!
    QByteArray str = d_line.mid(d_colNr + 0, off - 0 ); // mit ""
    //str.replace("\\\\", "\\" );
    //str.replace("\\$", "$");
    //str.replace("\\\"", "\"");
    return token( Tok_string, off, str );
}

