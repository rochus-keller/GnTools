#ifndef GnScopeTreeMdl_H
#define GnScopeTreeMdl_H

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

#include <QAbstractItemModel>
#include <GnTools/GnCodeModel.h>

class QTreeView;

namespace Gn
{
    class ScopeTreeMdl : public QAbstractItemModel
    {
    public:
        explicit ScopeTreeMdl(QTreeView *parent = 0);

        QTreeView* getParent() const;
        void setScope( CodeModel::Scope* );
        CodeModel::Scope* getSymbol( const QModelIndex & ) const;
        QModelIndex findSymbol( const CodeModel::Scope* );

        // overrides
        int columnCount ( const QModelIndex & parent = QModelIndex() ) const { return 1; }
        QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
        QModelIndex parent ( const QModelIndex & index ) const;
        int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        Qt::ItemFlags flags ( const QModelIndex & index ) const;

    private:
        struct Slot
        {
            CodeModel::Scope* d_scope;
            QList<Slot*> d_children;
            Slot* d_parent;
            Slot(Slot* p = 0 ):d_parent(p),d_scope(0){ if( p ) p->d_children.append(this); }
            ~Slot() { foreach( Slot* s, d_children ) delete s; }
        };
        typedef QMultiMap<QByteArray,CodeModel::Scope*> Sorter;
        void fill( Slot* );
        QModelIndex findSymbol(Slot*, const CodeModel::Scope* nt) const;
        Slot d_root;
    };
}


#endif // GnScopeTreeMdl_H
