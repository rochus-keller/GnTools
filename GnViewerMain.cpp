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

#include <QApplication>
#include <QFileInfo>
#include <QtDebug>
#include <QDir>
#include "GnMainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("me@rochus-keller.ch");
    a.setOrganizationDomain("github.com/rochus-keller/GnTools");
    a.setApplicationName("GnViewer"); // GN = Go Nuts ?
    a.setApplicationVersion("0.6.6");
    a.setStyle("Fusion");

    QString dirOrFilePath;
    const QStringList args = QCoreApplication::arguments();
    for( int i = 1; i < args.size(); i++ ) // arg 0 enthaelt Anwendungspfad
    {
        if( !args[ i ].startsWith( '-' ) )
        {
            if( !dirOrFilePath.isEmpty() )
            {
                qCritical() << "error: only one path allowed" << endl;
                return -1;
            }
            dirOrFilePath = args[ i ];
        }else
        {
            qCritical() << "error: invalid command line option " << args[i] << endl;
            return -1;
        }
    }

    if( !dirOrFilePath.isEmpty() )
        QDir::setCurrent(QFileInfo(dirOrFilePath).absolutePath());

    Gn::MainWindow w;
    if( !dirOrFilePath.isEmpty() )
        w.showPath(dirOrFilePath);
    else
        w.showHelp();

    return a.exec();
}
