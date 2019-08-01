#ifndef GNMAINWINDOW_H
#define GNMAINWINDOW_H

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

#include <QMainWindow>
#include <GnTools/GnSynTree.h>

class QPlainTextEdit;
class QTreeWidget;
class QTreeView;
class QLabel;
class QDir;
class QModelIndex;
class QLineEdit;
class QComboBox;

namespace Gn
{
    class ScopeTreeMdl;
    class CodeModel;
    class CodeBrowser;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget *parent = 0);

        void showPath(const QString& );
        void showHelp();
        void logMessage( const QString& );

    protected:
        void createFileList();
        void createScopeTree();
        void createXrefList();
        void createLog();
        void createQueryList();
        void fillXrefList( const SynTree* );
        void fillXrefList( const QByteArray&, const SynTree* = 0 );
        typedef QMap<QByteArray,const char*> Sorter;
        void addQueryResults( const Sorter& );

        // overrides
        void closeEvent(QCloseEvent* event);

    protected slots:
        void onDefDblClicked(const QModelIndex&);
        void onCursorPositionChanged();
        void onXrefDblClicked();
        void onGoBack();
        void onGoForward();
        void onGotoLine();
        void onFindInFile();
        void onFindAgain();
        void onOpen();
        void onFilesDblClicked();
        void onFileChanged( const QByteArray& );
        void onHelp();
        void onXrefSearch();
        void onShowFullscreen();
        void onQuery(int);
        void onQueryDblClicked();

    private:
        CodeModel* d_mdl;
        CodeBrowser* d_codeView;
        QTreeWidget* d_fileList;
        QLabel* d_rootDir;
        QLabel* d_sourceLoc;
        QPlainTextEdit* d_msgLog;
        QTreeView* d_defsList;
        ScopeTreeMdl* d_stm;
        QTreeWidget* d_xrefList;
        QLineEdit* d_xrefSearch;
        QTreeWidget* d_queryResults;
        QComboBox* d_queries;
    };
}

#endif // GNMAINWINDOW_H
