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

#include "GnHelpEngine.h"

#include <QBuffer>
#include <QFile>
#include <QtDebug>
using namespace Gn;

static const char* s_nameRef = "### <a name=";
static const int s_nameRefLen = 13;

HelpEngine::HelpEngine(QObject *parent) : QObject(parent)
{

}

QString HelpEngine::getHelpFrom(const QByteArray& name)
{
    if( d_sections.isEmpty() )
        parseFile();

    Sections::const_iterator i = d_sections.find(name);
    if( i == d_sections.end() )
        return QString();

    QString html = "<html><body>";
    foreach( const Section& s, i.value() )
    {
        if( s.d_kind == Section::Command )
            continue;
        html += formatMd(getSection(s.d_pos, s.d_len),s.d_kind);
    }
    html += "</body></html>";
    return html;
}

void HelpEngine::parseFile()
{
    QFile in(":/embedded_files/gn_help.md");
    if( !in.open(QIODevice::ReadOnly ) )
        Q_ASSERT( false );

    int pos = in.pos();
    QByteArray line = in.readLine();
    while( !in.atEnd() )
    {
        if( line.startsWith(s_nameRef) )
        {
            Section s;
            s.d_pos = pos;
            const int l = s_nameRefLen;
            const int r = line.indexOf('"',l+1);
            // var_, func_, cmd_
            QByteArray name = line.mid(l, r-l);
            if( name.startsWith( "var_" ) )
            {
                s.d_kind = Section::Variable;
                name = name.mid(4);
            }else if( name.startsWith("func_") )
            {
                s.d_kind = Section::Function;
                name = name.mid(5);
            }else if( name.startsWith("cmd_") )
            {
                s.d_kind = Section::Command;
                name = name.mid(4);
            }
            pos = in.pos();
            line = in.readLine();
            while( !in.atEnd() && !line.startsWith(s_nameRef) )
            {
                pos = in.pos();
                line = in.readLine();
            }
            s.d_len = pos - s.d_pos;
            d_sections[name].append(s);
        }else
        {
            pos = in.pos();
            line = in.readLine();
        }
    }
}

QString HelpEngine::formatMd(const QByteArray& str, int /*kind*/)
{
    QBuffer buf;
    buf.setData(str);
    buf.open(QIODevice::ReadOnly);

    QString html;
    QByteArray line = buf.readLine();
    while( !buf.atEnd() )
    {
        if( line.startsWith("###") )
        {
            const QString level = "h4"; // line[4] == '#' ? "h4" : "h3";
            html += "<" + level + ">";
            QByteArray title;
            const int apos = line.indexOf("</a>");
            if( apos != -1 )
            {
//                switch( kind )
//                {
//                case Section::Function:
//                    title = "Function ";
//                    break;
//                case Section::Variable:
//                    title = "Variable ";
//                    break;
//                case Section::Command:
//                    title = "Command ";
//                    break;
//                }
                title += line.mid(apos+4);
            }else
                title = line.mid(5);
            title.replace("**","");
            html += title.trimmed();
            html += "</" + level + ">";
        }else if( line.startsWith( "```" ) )
        {
            html += "<pre>";
            line = buf.readLine();
            while( !buf.atEnd() && !line.startsWith( "```" ) )
            {
                html += QString::fromUtf8(line).toHtmlEscaped();
                line = buf.readLine();
            }
            html += "\n</pre>";
        }else
        {
            // html += QString::fromUtf8( line );
        }
        line = buf.readLine();
    }

    return html;
}

QByteArray HelpEngine::getSection(int pos, int len)
{
    QFile in(":/embedded_files/gn_help.md");
    if( !in.open(QIODevice::ReadOnly ) )
        Q_ASSERT( false );
    in.seek(pos);
    return in.read(len);
}

