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

#include "GnScopeTreeMdl.h"
#include "GnSynTree.h"
#include <QBrush>
#include <QPixmap>
#include <QtDebug>
#include <QTreeView>
using namespace Gn;

ScopeTreeMdl::ScopeTreeMdl(QTreeView* parent) :
    QAbstractItemModel(parent)
{

}

QTreeView*ScopeTreeMdl::getParent() const
{
    return static_cast<QTreeView*>(QObject::parent());
}

void ScopeTreeMdl::setScope( CodeModel::Scope* s )
{
    beginResetModel();
    d_root = Slot();
    d_root.d_scope = s;
    fill(&d_root);
    endResetModel();
}

CodeModel::Scope* ScopeTreeMdl::getSymbol(const QModelIndex& index) const
{
    if( !index.isValid() || d_root.d_scope == 0 )
        return 0;
    Slot* s = static_cast<Slot*>( index.internalPointer() );
    Q_ASSERT( s != 0 );
    return s->d_scope;
}

QModelIndex ScopeTreeMdl::findSymbol(const CodeModel::Scope* nt)
{
    return findSymbol( &d_root, nt );
}

QVariant ScopeTreeMdl::data(const QModelIndex& index, int role) const
{
    if( !index.isValid() || d_root.d_scope == 0 )
        return QVariant();

    Slot* s = static_cast<Slot*>( index.internalPointer() );
    Q_ASSERT( s != 0 );
    switch( role )
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        {
            QString str;
            if( !s->d_scope->d_name.isEmpty() )
                str = QString(" \"%1\" ").arg( s->d_scope->d_name.constData() );
            else if( s->d_scope->d_params != 0 )
                str = QString("...");
            return QString("%1(%2)").arg(s->d_scope->d_kind.constData()).arg(str);
        }
    case Qt::FontRole:
        break;
    case Qt::ForegroundRole:
        break;
    }
    return QVariant();
}

QModelIndex ScopeTreeMdl::parent ( const QModelIndex & index ) const
{
    if( index.isValid() )
    {
        Slot* s = static_cast<Slot*>( index.internalPointer() );
        Q_ASSERT( s != 0 );
        if( s->d_parent == &d_root )
            return QModelIndex();
        // else
        Q_ASSERT( s->d_parent != 0 );
        Q_ASSERT( s->d_parent->d_parent != 0 );
        return createIndex( s->d_parent->d_parent->d_children.indexOf( s->d_parent ), 0, s->d_parent );
    }else
        return QModelIndex();
}

int ScopeTreeMdl::rowCount ( const QModelIndex & parent ) const
{
    if( parent.isValid() )
    {
        Slot* s = static_cast<Slot*>( parent.internalPointer() );
        Q_ASSERT( s != 0 );
        return s->d_children.size();
    }else
        return d_root.d_children.size();
}

QModelIndex ScopeTreeMdl::index ( int row, int column, const QModelIndex & parent ) const
{
    const Slot* s = &d_root;
    if( parent.isValid() )
    {
        s = static_cast<Slot*>( parent.internalPointer() );
        Q_ASSERT( s != 0 );
    }
    if( row < s->d_children.size() && column < columnCount( parent ) )
        return createIndex( row, column, s->d_children[row] );
    else
        return QModelIndex();
}

Qt::ItemFlags ScopeTreeMdl::flags( const QModelIndex & index ) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; //  | Qt::ItemIsDragEnabled;
}

void ScopeTreeMdl::fill(ScopeTreeMdl::Slot* super)
{
    if( super->d_scope == 0 )
        return;

    Sorter sort;
    foreach( CodeModel::Scope* s, super->d_scope->d_allScopes )
        sort.insert( ( s->d_kind + s->d_name ).toLower(), s );
    Sorter::const_iterator i;
    for( i = sort.begin(); i != sort.end(); ++i )
    {
        Slot* s = new Slot( super );
        s->d_scope = i.value();
        fill( s );
    }
}

QModelIndex ScopeTreeMdl::findSymbol(ScopeTreeMdl::Slot* slot, const CodeModel::Scope* nt) const
{
    for( int i = 0; i < slot->d_children.size(); i++ )
    {
        Slot* s = slot->d_children[i];
        if( s->d_scope == nt )
            return createIndex( i, 0, s );
        QModelIndex index = findSymbol( s, nt );
        if( index.isValid() )
            return index;
    }
    return QModelIndex();
}



