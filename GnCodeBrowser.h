#ifndef GNCODEBROWSER_H
#define GNCODEBROWSER_H

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

#include <QPlainTextEdit>

namespace Gn
{
    class SynTree;
    class CodeModel;

    class CodeBrowser : public QPlainTextEdit
    {
        Q_OBJECT
    public:
        CodeBrowser(CodeModel*,QWidget* p);

        void clear();

        void updateExtraSelections();
        void pushLocation(SynTree* st);
        void goBack();
        void goForward();
        void setCursorPosition( SynTree* id, bool center, bool pushLoc = false );
        void setCursorPosition(int line, int col, bool center, int sel = -1 );
        void markNonTerms(const QList<const SynTree*>& s);
        SynTree* getCur() const { return d_cur; }
        const QByteArray& getSourcePath() const { return d_sourcePath; }
        void find( const QString&, bool fromTop = true );
        void findAgain();

    signals:
        void sigShowFile( const QByteArray& );

    protected:
        void mouseMoveEvent(QMouseEvent* e);
        void mousePressEvent(QMouseEvent* e);

        bool loadFile( const QByteArray& path );
        void find( bool fromTop );

    private:
        CodeModel* d_mdl;
        QByteArray d_sourcePath;
        typedef QList<QTextEdit::ExtraSelection> ESL;
        ESL d_link;
        SynTree* d_goto;
        ESL d_nonTerms;
        QString d_find;
        SynTree* d_cur;
        QList<SynTree*> d_backHisto; // d_backHisto.last() ist aktuell angezeigtes Objekt
        QList<SynTree*> d_forwardHisto;
        bool d_pushBackLock;
    };
}

#endif // GNCODEBROWSER_H
