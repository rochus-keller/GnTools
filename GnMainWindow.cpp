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

#include "GnMainWindow.h"
#include "GnScopeTreeMdl.h"
#include "GnCodeModel.h"
#include "GnCodeBrowser.h"
#include "GnHighlighter.h"
#include "GnLexer.h"
#include <QDockWidget>
#include <QFile>
#include <QPainter>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QtDebug>
#include <QApplication>
#include <QSettings>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QLabel>
#include <QShortcut>
#include <QInputDialog>
#include <QDir>
#include <QFileDialog>
#include <QDate>
#include <algorithm>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
using namespace Gn;

Q_DECLARE_METATYPE(Gn::SynTree*)

static const char* s_queries[] =
{
    "<select>",
    "Unresolved Imports",
    "Definitions with dynamic names",
    "LHS only vars",
    "RHS only vars",
    "Dynamic references",
    "Declared args",
    0
};

static MainWindow* s_this = 0;
static void report(QtMsgType type, const QString& message )
{
	if( s_this )
	{
		switch(type)
		{
		case QtDebugMsg:
			s_this->logMessage(QLatin1String("INF: ") + message);
			break;
		case QtWarningMsg:
			s_this->logMessage(QLatin1String("WRN: ") + message);
			break;
		case QtCriticalMsg:
		case QtFatalMsg:
			s_this->logMessage(QLatin1String("ERR: ") + message);
			break;
		}
	}
}

static QtMessageHandler s_oldHandler = 0;
void messageHander(QtMsgType type, const QMessageLogContext& ctx, const QString& message)
{
    if( s_oldHandler )
        s_oldHandler(type, ctx, message );
	report(type,message);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    s_this = this;

    setWindowTitle( tr("%1 v%2").arg( qApp->applicationName() ).arg( qApp->applicationVersion() ) );

    d_mdl = new Gn::CodeModel(this);

    QWidget* pane = new QWidget(this);
    QVBoxLayout* vbox = new QVBoxLayout(pane);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    d_sourceLoc = new QLabel(this);
    d_sourceLoc->setMargin(2);
    d_sourceLoc->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vbox->addWidget(d_sourceLoc);

    d_codeView = new CodeBrowser(d_mdl,this);
    vbox->addWidget(d_codeView);

    setCentralWidget(pane);

    setDockNestingEnabled(true);
    setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );
    setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
    setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setCorner( Qt::TopLeftCorner, Qt::LeftDockWidgetArea );

    createFileList();
    createScopeTree();
    createXrefList();
    createLog();
    createQueryList();

    connect( d_codeView, SIGNAL( cursorPositionChanged() ), this, SLOT(  onCursorPositionChanged() ) );
    connect( d_codeView, SIGNAL(sigShowFile(QByteArray)), this, SLOT(onFileChanged(QByteArray)) );

    QSettings s;
    const QVariant state = s.value( "DockState" );
    if( !state.isNull() )
        restoreState( state.toByteArray() );

    new QShortcut(tr("ALT+LEFT"),this,SLOT(onGoBack()) );
    new QShortcut(tr("ALT+RIGHT"),this,SLOT(onGoForward()) );
    new QShortcut(tr("CTRL+Q"),this,SLOT(close()) );
    new QShortcut(tr("CTRL+L"),this,SLOT(onGotoLine()) );
    new QShortcut(tr("CTRL+SHIFT+L"),this,SLOT(onGotoFileLine()) );
    new QShortcut(tr("CTRL+F"),this,SLOT(onFindInFile()) );
    new QShortcut(tr("CTRL+G"),this,SLOT(onFindAgain()) );
    new QShortcut(tr("F3"),this,SLOT(onFindAgain()) );
    new QShortcut(tr("F1"),this,SLOT(onHelp()) );
    new QShortcut(tr("CTRL+O"),this,SLOT(onOpen()) );
    new QShortcut(tr("ESC"), d_msgLog->parentWidget(), SLOT(close()) );
    new QShortcut(tr("SHIFT+ESC"), d_msgLog, SLOT(clear()) );
    new QShortcut(tr("F11"),this,SLOT(onShowFullscreen()) );

    s_oldHandler = qInstallMessageHandler(messageHander);

    if( s.value("Fullscreen").toBool() )
        showFullScreen();
    else
        showMaximized();
}

void MainWindow::showPath(const QString& path)
{
    d_msgLog->clear();
    d_fileList->clear();
    d_rootDir->clear();
    d_mdl->parseDir(path);
    d_stm->setScope(0);
    d_xrefList->clear();
    d_codeView->clear();
    d_sourceLoc->clear();
    d_xrefSearch->clear();
    d_queries->setCurrentIndex(0);
    d_queryResults->clear();

    d_rootDir->setText( d_mdl->getSourceRoot().absolutePath() );

    QByteArrayList files = d_mdl->getFileList();
    foreach( const QByteArray& file, files )
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(d_fileList);
        item->setText( 0, d_mdl->getSourceRoot().relativeFilePath( QString::fromUtf8(file) ) );
        item->setToolTip(0,item->text(0));
        item->setData( 0, Qt::UserRole, file );
    }
    setWindowTitle( tr("%3 - %1 v%2").arg( qApp->applicationName() ).arg( qApp->applicationVersion() )
                    .arg( d_rootDir->text() ));
}

void MainWindow::showHelp()
{
    onHelp();
}

void MainWindow::logMessage(const QString& str)
{
    d_msgLog->parentWidget()->show();
    d_msgLog->appendPlainText(str);
}

void MainWindow::createFileList()
{
    QDockWidget* dock = new QDockWidget( tr("Files"), this );
    dock->setObjectName("Files");
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::DockWidgetMovable );
    QWidget* pane = new QWidget(dock);
    QVBoxLayout* vbox = new QVBoxLayout(pane);
    vbox->setMargin(0);
    vbox->setSpacing(2);
    d_rootDir = new QLabel(pane);
    d_rootDir->setTextInteractionFlags(Qt::TextSelectableByMouse);
    d_rootDir->setMargin(2);
    d_rootDir->setWordWrap(true);
    d_rootDir->setFrameStyle(QFrame::StyledPanel);
    vbox->addWidget(d_rootDir);
    d_fileList = new QTreeWidget(pane);
    d_fileList->setAlternatingRowColors(true);
    d_fileList->setHeaderHidden(true);
    d_fileList->setSortingEnabled(false);
    d_fileList->setAllColumnsShowFocus(true);
    d_fileList->setRootIsDecorated(false);
    vbox->addWidget(d_fileList);
    dock->setWidget(pane);
    addDockWidget( Qt::LeftDockWidgetArea, dock );
    connect(d_fileList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onFilesDblClicked()) );
}

void MainWindow::createScopeTree()
{
    QDockWidget* dock = new QDockWidget( tr("Defs in File"), this );
    dock->setObjectName("Defs");
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::DockWidgetMovable );
    d_defsList = new QTreeView(dock);
    d_defsList->setAlternatingRowColors(true);
    d_defsList->setHeaderHidden(true);
    d_defsList->setSortingEnabled(false);
    d_defsList->setAllColumnsShowFocus(true);
    d_defsList->setRootIsDecorated(false);
    d_defsList->setExpandsOnDoubleClick(false);
    d_stm = new ScopeTreeMdl(d_defsList);
    d_defsList->setModel(d_stm);
    dock->setWidget(d_defsList);
    addDockWidget( Qt::LeftDockWidgetArea, dock );
    connect( d_defsList,SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDefDblClicked(QModelIndex)) );
}

void MainWindow::createXrefList()
{
    QDockWidget* dock = new QDockWidget( tr("Crossrefs"), this );
    dock->setObjectName("Xref");
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::DockWidgetMovable );
    QWidget* pane = new QWidget(dock);
    QVBoxLayout* vbox = new QVBoxLayout(pane);
    vbox->setMargin(0);
    vbox->setSpacing(2);
    d_xrefSearch = new QLineEdit(pane);
    vbox->addWidget(d_xrefSearch);
    d_xrefList = new QTreeWidget(pane);
    d_xrefList->setAlternatingRowColors(true);
    d_xrefList->setHeaderHidden(true);
    d_xrefList->setSortingEnabled(false);
    d_xrefList->setAllColumnsShowFocus(true);
    d_xrefList->setRootIsDecorated(false);
    vbox->addWidget(d_xrefList);
    dock->setWidget(pane);
    addDockWidget( Qt::RightDockWidgetArea, dock );
    connect(d_xrefList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onXrefDblClicked()) );
    connect(d_xrefSearch,SIGNAL(editingFinished()),this,SLOT(onXrefSearch()) );
}

void MainWindow::createLog()
{
    QDockWidget* dock = new QDockWidget( tr("Message Log"), this );
    dock->setObjectName("Log");
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
    d_msgLog = new QPlainTextEdit(dock);
    d_msgLog->setReadOnly(true);
    d_msgLog->setLineWrapMode( QPlainTextEdit::NoWrap );
    new LogPainter(d_msgLog->document());
    dock->setWidget(d_msgLog);
    addDockWidget( Qt::BottomDockWidgetArea, dock );
}

void MainWindow::createQueryList()
{
    QDockWidget* dock = new QDockWidget( tr("Queries"), this );
    dock->setObjectName("Queries");
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::DockWidgetMovable );
    QWidget* pane = new QWidget(dock);
    QVBoxLayout* vbox = new QVBoxLayout(pane);
    vbox->setMargin(0);
    vbox->setSpacing(2);
    d_queries = new QComboBox(pane);
    const char** query = s_queries;
    while( *query )
    {
        d_queries->addItem(*query);
        query++;
    }
    d_queries->setMinimumWidth(100);
    vbox->addWidget(d_queries);
    d_queryResults = new QTreeWidget(pane);
    d_queryResults->setAlternatingRowColors(true);
    d_queryResults->setHeaderHidden(true);
    d_queryResults->setSortingEnabled(false);
    d_queryResults->setAllColumnsShowFocus(true);
    d_queryResults->setRootIsDecorated(false);
    vbox->addWidget(d_queryResults);
    dock->setWidget(pane);
    addDockWidget( Qt::RightDockWidgetArea, dock );
    connect(d_queryResults, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onQueryDblClicked()) );
    connect(d_queries,SIGNAL(currentIndexChanged(int)),this,SLOT(onQuery(int)) );
}

static bool UsedByLessThan( const SynTree* lhs, const SynTree* rhs )
{
    return lhs->d_tok.d_sourcePath < rhs->d_tok.d_sourcePath ||
            (!(rhs->d_tok.d_sourcePath < lhs->d_tok.d_sourcePath) &&
             lhs->d_tok.d_lineNr < rhs->d_tok.d_lineNr );
}

void MainWindow::fillXrefList(const SynTree* id)
{
    if( id == 0 )
        return;

    QByteArray str;
    if( id->d_tok.d_type == Tok_string )
        str = id->d_tok.getEscapedVal();
    else if( id->d_tok.d_type == Tok_identifier )
        str = id->d_tok.d_val;
    else
        return;

    fillXrefList(str, id);
}

void MainWindow::fillXrefList(const QByteArray& str, const SynTree* id )
{
    d_xrefSearch->clear();
    d_xrefList->clear();

    CodeModel::PathIdentPair pip = CodeModel::extractPathIdentFromString(str);
    if( pip.first.isEmpty() && pip.second.isEmpty() )
        return;
    else if( pip.second.contains('$') )
        return;
    else if( !pip.second.isEmpty() )
        pip.first = d_mdl->calcPath( pip.first, d_codeView->getSourcePath() ).toUtf8();
    else
    {
        const QByteArray path = d_mdl->calcPath( pip.first, d_codeView->getSourcePath() ).toUtf8();
        if( path.isEmpty() )
        {
            pip.second = pip.first;
            pip.first.clear();
        }else
            pip.first = path;
    }

//        if( !Lexer::isValidIdent(name) )
//            return;

    const QByteArray path = Lexer::getSymbol(pip.first);
    const QByteArray name = Lexer::getSymbol(pip.second);

    if( !name.isEmpty() )
        d_xrefSearch->setText( QString::fromUtf8(name) );
    else
        d_xrefSearch->setText( QString::fromUtf8(str) );

    QFont bold = d_xrefList->font();
    bold.setBold(true);

    QList<const SynTree*> nt;
    CodeModel::ObjRefs::const_iterator i1 = d_mdl->getAllObjDefs().find(name.constData());
    if( i1 != d_mdl->getAllObjDefs().end() )
    {
        foreach( CodeModel::Scope* s, i1.value() )
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(d_xrefList);
            item->setText( 0, QString("Def: %1:%2:%3").arg(d_mdl->relativePath(s->d_st->d_tok.d_sourcePath))
                                                          .arg(s->d_st->d_tok.d_lineNr).arg(s->d_st->d_tok.d_colNr) );
            item->setToolTip( 0, item->text(0) );
            item->setData( 0, Qt::UserRole, QVariant::fromValue(s->d_st) );
            if( s->d_params == id )
                item->setFont(0,bold);
        }
    }
    CodeModel::VarRefs::const_iterator i2 = d_mdl->getAllFuncRefs().find(name.constData());
    if( i2 != d_mdl->getAllFuncRefs().end() )
    {
        foreach( SynTree* s, i2.value() )
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(d_xrefList);
            item->setText( 0, QString("Ref: %1:%2:%3").arg(d_mdl->relativePath(s->d_tok.d_sourcePath))
                                                          .arg(s->d_tok.d_lineNr).arg(s->d_tok.d_colNr) );
            item->setToolTip( 0, item->text(0) );
            item->setData( 0, Qt::UserRole, QVariant::fromValue(s) );
            if( s == id )
                item->setFont(0,bold);
            else if( s->d_tok.d_sourcePath.constData() == d_codeView->getSourcePath().constData() )
                nt.append(s);
        }
    }
    i2 = d_mdl->getAllLhs().find(name.constData());
    if( i2 != d_mdl->getAllLhs().end() )
    {
        foreach( SynTree* s, i2.value() )
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(d_xrefList);
            item->setText( 0, QString("Lhs: %1:%2:%3").arg(d_mdl->relativePath(s->d_tok.d_sourcePath))
                                                          .arg(s->d_tok.d_lineNr).arg(s->d_tok.d_colNr) );
            item->setToolTip( 0, item->text(0) );
            item->setData( 0, Qt::UserRole, QVariant::fromValue(s) );
            if( s == id )
                item->setFont(0,bold);
            else if( s->d_tok.d_sourcePath.constData() == d_codeView->getSourcePath().constData() )
                nt.append(s);
        }
    }
    i2 = d_mdl->getAllRhs().find(name.constData());
    if( i2 != d_mdl->getAllRhs().end() )
    {
        foreach( SynTree* s, i2.value() )
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(d_xrefList);
            item->setText( 0, QString("Rhs: %1:%2:%3").arg(d_mdl->relativePath(s->d_tok.d_sourcePath))
                                                          .arg(s->d_tok.d_lineNr).arg(s->d_tok.d_colNr) );
            item->setToolTip( 0, item->text(0) );
            item->setData( 0, Qt::UserRole, QVariant::fromValue(s) );
            if( s == id )
                item->setFont(0,bold);
            else if( s->d_tok.d_sourcePath.constData() == d_codeView->getSourcePath().constData() )
                nt.append(s);
        }
    }
    if( !path.isEmpty() )
    {
        i2 = d_mdl->getAllImports().find(path.constData());
        if( i2 != d_mdl->getAllImports().end() )
        {
            foreach( SynTree* s, i2.value() )
            {
                QTreeWidgetItem* item = new QTreeWidgetItem(d_xrefList);
                item->setText( 0, QString("Imp: %1:%2:%3").arg(d_mdl->relativePath(s->d_tok.d_sourcePath))
                                                              .arg(s->d_tok.d_lineNr).arg(s->d_tok.d_colNr) );
                item->setToolTip( 0, item->text(0) );
                item->setData( 0, Qt::UserRole, QVariant::fromValue(s) );
                if( s == id )
                    item->setFont(0,bold);
            }
        }
    }
    d_codeView->markNonTerms(nt);
}

void MainWindow::addQueryResults(const MainWindow::Sorter& sorter)
{
    QFont italic = d_queryResults->font();
    italic.setItalic(true);

    for( Sorter::const_iterator j = sorter.begin(); j != sorter.end(); ++j )
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(d_queryResults);
        item->setText( 0, j.value() );
        item->setToolTip( 0, item->text(0) );
        item->setData( 0, Qt::UserRole, QByteArray(j.value()) );
        if( d_mdl->isKnownVar(j.value()) )
            item->setFont(0,italic);
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings s;
    s.setValue( "DockState", saveState() );
    event->setAccepted(true);
}

void MainWindow::onDefDblClicked(const QModelIndex& i)
{
    const CodeModel::Scope* nt = d_stm->getSymbol(i);

    if( nt == 0 )
        return;

    if( nt->d_st )
    {
        d_codeView->setCursorPosition( nt->d_st, true, true );
    }
}

void MainWindow::onCursorPositionChanged()
{
    QTextCursor cur = d_codeView->textCursor();
    const int line = cur.blockNumber() + 1;
    const int col = cur.positionInBlock() + 1;
    if( d_codeView->getCur() )
    {
        d_sourceLoc->setText( QString("%1   %2:%3   %4").arg(d_codeView->getSourcePath().constData()).
                        arg(line).arg(col).arg( SynTree::rToStr(d_codeView->getCur()->d_tok.d_type) ) );
    }else
        d_sourceLoc->setText( QString("%1   %2:%3").arg(d_codeView->getSourcePath().constData()).arg(line).arg(col) );

    fillXrefList( d_codeView->getCur() );
}

void MainWindow::onXrefDblClicked()
{
    if( d_xrefList->currentItem() == 0 )
        return;

    SynTree* st = d_xrefList->currentItem()->data(0,Qt::UserRole).value<SynTree*>();
    if( st == 0 )
        return;
    d_codeView->setCursorPosition( st, true, true );
}

void MainWindow::onGoBack()
{
    d_codeView->goBack();
}

void MainWindow::onGoForward()
{
    d_codeView->goForward();
}

void MainWindow::onGotoLine()
{
    QTextCursor cur = d_codeView->textCursor();
    int line = cur.blockNumber();
	bool ok	= false;
	line = QInputDialog::getInt(
				this, tr("Goto Line"),
        tr("Enter a valid line number:"),
        line + 1, 1, 999999, 1,	&ok );
    if( !ok )
        return;
    QTextBlock block = d_codeView->document()->findBlockByNumber(line-1);
    cur.setPosition( block.position() );
    d_codeView->setTextCursor( cur );
    d_codeView->centerCursor();
    d_codeView->updateExtraSelections();
}

void MainWindow::onFindInFile()
{
    bool ok	= false;
    const QString sel = d_codeView->textCursor().selectedText();
    QString res = QInputDialog::getText( this, tr("Find in File"),
        tr("Enter search string:"), QLineEdit::Normal, sel, &ok );
    if( !ok )
        return;
    d_codeView->find( res, sel.isEmpty() );
}

void MainWindow::onFindAgain()
{
    d_codeView->findAgain();
}

void MainWindow::onOpen()
{
    QString path = QFileDialog::getExistingDirectory(this,tr("Open Project Directory"),QDir::currentPath() );
    if( path.isEmpty() )
        return;
    QDir::setCurrent(path);
    showPath(path);
}

void MainWindow::onFilesDblClicked()
{
    if( d_fileList->currentItem() == 0 )
        return;

    const QByteArray sourcePath = d_fileList->currentItem()->data(0,Qt::UserRole).toByteArray();
    const CodeModel::Scope* sc = d_mdl->getScope(sourcePath);
    if( sc == 0 )
        return;
    d_codeView->setCursorPosition( sc->d_st, true, true );

}

void MainWindow::onFileChanged(const QByteArray& path)
{
    d_stm->setScope( d_mdl->getScope(path) );
    d_defsList->expandAll();
    QTreeWidgetItem* cur = 0;
    QFont bold = d_fileList->font();
    bold.setBold(true);
    for( int i = 0; i < d_fileList->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem* item = d_fileList->topLevelItem(i);
        if( item->data(0,Qt::UserRole).toByteArray() == path )
        {
            item->setFont(0, bold);
            cur = item;
        }else
            item->setFont(0,d_fileList->font());
    }
    d_fileList->scrollToItem(cur);
    d_fileList->setCurrentItem(cur);
}

void MainWindow::onHelp()
{
    logMessage(tr("Welcome to %1 %2\nAuthor: %3\nSite: %4\nLicense: GPL\n").arg( qApp->applicationName() )
               .arg( qApp->applicationVersion() ).arg( qApp->organizationName() ).arg( qApp->organizationDomain() ));
    logMessage(tr("Shortcuts:"));
    logMessage(tr("CTRL+O to open the directory containing the GN files") );
    logMessage(tr("Double-click on an item in the File list to show source") );
    logMessage(tr("CTRL+L to go to a specific line in current file") );
    logMessage(tr("CTRL+F to find a string in the current file") );
    logMessage(tr("CTRL+G or F3 to find another match in the current file") );
    logMessage(tr("CTRL-click on the strings or idents in the source to navigate") );
    logMessage(tr("ALT+LEFT to move backwards in the navigation history") );
    logMessage(tr("ALT+RIGHT to move forward in the navigation history") );
    logMessage(tr("ESC to close Message Log") );
    logMessage(tr("SHIFT+ESC to clear Message Log") );
    logMessage(tr("F1 to print help message to log") );
    logMessage(tr("F11 to toggle fullscreen mode") );
    logMessage(tr("CTRL+Q or ALT+F4 to close the application") );
}

void MainWindow::onXrefSearch()
{
    fillXrefList( d_xrefSearch->text().toUtf8() );
}

void MainWindow::onShowFullscreen()
{
    QSettings s;
    if( isFullScreen() )
    {
        showMaximized();
        s.setValue("Fullscreen", false );
    }else
    {
        showFullScreen();
        s.setValue("Fullscreen", true );
    }
}

void MainWindow::onQuery(int q)
{
    d_queryResults->clear();
    switch( q )
    {
    case 0:
        break;
    case 1: // unresolved imports
        foreach( SynTree* s, d_mdl->getAllUnresolvedImports() )
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(d_queryResults);
            item->setText( 0, QString("%1:%2:%3").arg(d_mdl->relativePath(s->d_tok.d_sourcePath))
                                                          .arg(s->d_tok.d_lineNr).arg(s->d_tok.d_colNr) );
            item->setToolTip( 0, item->text(0) );
            item->setData( 0, Qt::UserRole, QVariant::fromValue(s) );
        }
        break;
    case 2: // definitions with dynamic names
        foreach( CodeModel::Scope* sc, d_mdl->getAllUnnamedObjs() )
        {
            SynTree* s = sc->d_params;
            QTreeWidgetItem* item = new QTreeWidgetItem(d_queryResults);
            item->setText( 0, QString("%1:%2:%3").arg(d_mdl->relativePath(s->d_tok.d_sourcePath))
                                                          .arg(s->d_tok.d_lineNr).arg(s->d_tok.d_colNr) );
            item->setToolTip( 0, item->text(0) );
            item->setData( 0, Qt::UserRole, QVariant::fromValue(s) );
        }
        break;
     case 3: //   "LHS only vars",
        {
            CodeModel::VarRefs::const_iterator i;
            Sorter sorter;
            for( i = d_mdl->getAllLhs().begin(); i != d_mdl->getAllLhs().end(); ++i )
            {
                if( d_mdl->getAllRhs().contains( i.key() ) )
                    continue;
                sorter.insert( i.key(), i.key() );
            }
            addQueryResults(sorter);
        }
        break;
     case 4: //   "RHS only vars",
        {
            CodeModel::VarRefs::const_iterator i;
            Sorter sorter;
            for( i = d_mdl->getAllRhs().begin(); i != d_mdl->getAllRhs().end(); ++i )
            {
                if( d_mdl->getAllLhs().contains( i.key() ) )
                    continue;
                sorter.insert( i.key(), i.key() );
            }
            addQueryResults(sorter);
        }
        break;
    case 5: // Dynamic references
        foreach( SynTree* s, d_mdl->getUnresolvedRefs() )
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(d_queryResults);
            item->setText( 0, QString("%1:%2:%3").arg(d_mdl->relativePath(s->d_tok.d_sourcePath))
                                                          .arg(s->d_tok.d_lineNr).arg(s->d_tok.d_colNr) );
            item->setToolTip( 0, item->text(0) );
            item->setData( 0, Qt::UserRole, QVariant::fromValue(s) );
        }
        break;
    case 6: // "Declared args"
        {
            Sorter sorter;
            foreach( SynTree* s, d_mdl->getDeclaredArgs() )
            {
                sorter.insert( s->d_tok.d_val, s->d_tok.d_val.constData() );
            }
            addQueryResults(sorter);
        }
        break;
    }
}

void MainWindow::onQueryDblClicked()
{
    if( d_queryResults->currentItem() == 0 )
        return;

    SynTree* st = d_queryResults->currentItem()->data(0,Qt::UserRole).value<SynTree*>();
    if( st != 0 )
        d_codeView->setCursorPosition( st, true, true );
    else
        fillXrefList(d_queryResults->currentItem()->data(0,Qt::UserRole).toByteArray());
}

void MainWindow::onGotoFileLine()
{
    bool ok	= false;
    QString res = QInputDialog::getText( this, tr("Goto File/Line"), tr("Path:Line:Col:"),
                                                QLineEdit::Normal, QString(), &ok );
    if( !ok || res.isEmpty() )
        return;

    // "//build/dart/dart_action.gni:101:3"

    if( res.endsWith(QChar(':')) )
        res.chop(1);

    const int firstColon = res.indexOf(QChar(':'));
    const int secondColon = firstColon != -1 ? res.indexOf(QChar(':'),firstColon+1) : -1;
    CodeModel::PathIdentPair pip;
    int row = 0, col = 0;
    if( firstColon == -1 )
        pip = CodeModel::extractPathIdentFromString(res.toUtf8()); // "//build/dart/dart_action.gni", "//runtime/bin"
    else
    {
        pip.first = res.left(firstColon).toUtf8();
        if( secondColon == -1 ) // "//runtime/bin:dart", "//build/dart/dart_action.gni:101"
        {
            row = res.mid(firstColon+1).toUInt(&ok);
            if( !ok )
                pip = CodeModel::extractPathIdentFromString(res.toUtf8());
        }else
        {
            // "//build/dart/dart_action.gni:101:3"
            row = res.mid(firstColon+1, secondColon - firstColon - 1).toUInt(&ok);
            if( !ok )
            {
                logMessage(tr("ERR: invalid row") );
                return;
            }
            col = res.mid(secondColon+1).toUInt(&ok);
            if( !ok )
            {
                logMessage(tr("ERR: invalid col") );
                return;
            }
        }
    }

    const QByteArray path = Lexer::getSymbol(
                d_mdl->calcPath( pip.first, /*d_codeView->getSourcePath*/QByteArray(), !pip.second.isEmpty() ).toUtf8());
    CodeModel::Scope* sc = d_mdl->getScope(path);
    if( sc == 0 )
    {
        logMessage(tr("ERR: file not found") );
        return;
    }
    if( !pip.second.isEmpty() )
    {
        sc = sc->findObject(Lexer::getSymbol(pip.second).constData(),false,false);
        if( sc == 0 )
        {
            logMessage(tr("ERR: label not found in file") );
            return;
        }
        d_codeView->setCursorPosition(sc->d_st,true,true);
    }else
        d_codeView->setCursorPosition( path, row-1, col-1, true );
}

