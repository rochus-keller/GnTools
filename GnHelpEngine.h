#ifndef GNHELPENGINE_H
#define GNHELPENGINE_H

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

#include <QObject>
#include <QHash>

namespace Gn
{
    class HelpEngine : public QObject
    {
    public:
        explicit HelpEngine(QObject *parent = 0);

        struct Section
        {
            enum Kind { Unknown = 0, Function = 1, Variable = 2, Command = 3 };
            Section(unsigned p = 0, unsigned l = 0, unsigned k = 0 ):d_pos(p),d_len(l),d_kind(k){}
            unsigned d_pos;
            unsigned d_len : 24;
            unsigned d_kind : 8;
        };
        typedef QList<Section> SectionList;

        QString getHelpFrom( const QByteArray& name );

    protected:
        void parseFile();
        static QString formatMd(const QByteArray& str , int kind = 0);
        static QByteArray getSection( int pos, int len );
    private:
        typedef QHash<QByteArray,SectionList> Sections;
        Sections d_sections;
    };
}

#endif // GNHELPENGINE_H
