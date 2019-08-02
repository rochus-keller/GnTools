#ifndef GNCODEMODEL_H
#define GNCODEMODEL_H

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

#include <QObject>
#include <QDir>
#include <QSet>

/*
 *  Responsibilities:
 *  - Find .gn (dotfile, toplevel gn file)
 *  - Find all *.gn and *.gni at and below toplevel
 *  - Parse all these files
 *  - Crossref all identifier uses including target names etc.
*/

namespace Gn
{
    class Errors;
    class SynTree;

    class CodeModel : public QObject
    {
    public:
        typedef QList<SynTree*> SynTreeList;
        typedef QHash<const char*,SynTreeList> VarRefs;

        struct Scope
        {
            Scope():d_outer(0),d_st(0),d_params(0) {}
            ~Scope();

            Scope* findObject( const QByteArray& name, bool recursive = true ) const;

            QByteArray d_kind;
            QByteArray d_name; // name or filepath symbol
            SynTree* d_params; // func params

            // Files and function calls followed by { } blocks introduce new scopes.
            // File contains targets which are named scopes
            SynTree* d_st;

            typedef QHash<const char*,Scope*> ScopeHash;
            ScopeHash d_objectDefs;
            QList<Scope*> d_allScopes; // owner
            ScopeHash d_relovedImports;
            SynTreeList d_unresolvedImports;

            VarRefs d_lhs;
            VarRefs d_rhs;

            Scope* d_outer;
        };
        typedef QList<Scope*> ScopeList;
        typedef QHash<const char*,ScopeList> ObjRefs;

        explicit CodeModel(QObject *parent = 0);
        ~CodeModel();

        bool parseDir( const QDir& );
        QString calcPath(SynTree* ref ) const;
        QString calcPath(const QByteArray& path , const QByteArray& ref) const;
        QString relativePath( const QByteArray& sourcePath ) const;
        SynTree* findSymbolBySourcePos(const QByteArray& sourcePath, quint32 line, quint16 col ) const;
        SynTree* findFromPath( const QByteArray& path, const QByteArray& callerPath = QByteArray() );
        SynTree* findDefinition( const SynTree* );

        const QDir& getSourceRoot() const { return d_sourceRoot; }
        QByteArrayList getFileList() const;
        Scope* getScope( const QByteArray& sourceFile ) const;
        bool isKnownVar( const char* ) const;
        bool isKnownObj( const QByteArray& ) const;
        bool isKnownId( const QByteArray& ) const;

        const VarRefs& getAllRhs() const { return d_allRhs; }
        const VarRefs& getAllLhs() const { return d_allLhs; }
        const VarRefs& getAllFuncRefs() const { return d_allFuncRefs; }
        const VarRefs& getAllImports() const { return d_allImports; }
        const ObjRefs& getAllObjDefs() const { return d_allObjDefs; }
        const SynTreeList& getAllUnresolvedImports() const { return d_allUnresolvedImports; }
        const ScopeList& getAllUnnamedObjs() const { return d_allUnnamedObjs; }
        const SynTreeList& getUnresolvedRefs() const { return d_unresolvedRefs; }
        const SynTreeList& getDeclaredArgs() const { return d_declaredArgs; }

        static SynTree* flatten(SynTree* st, int stopAt = 0);
        static SynTree* firstToken(SynTree* st);

        struct Dollar
        {
            quint16 d_pos;
            quint16 d_len;
            Dollar(quint16 p = 0, quint16 l = 0 ):d_pos(p),d_len(l){}
        };
        typedef QList<Dollar> Dollars;
        static Dollars findDollars( const QByteArray& str );
        typedef QPair<QByteArray,QByteArray> PathIdentPair; // see 'gn help labels'
        static PathIdentPair extractPathIdentFromString( QByteArray str );

    protected:
        QString findDotFile(const QDir&);
        void clear();
        Scope* parseFile(const QString& path, bool viaImport=false);
        void statementList(SynTree*,Scope*);
        void statement(SynTree*,Scope*);
        void call_(SynTree* st, Scope* s);
        void loop_(SynTree* st, Scope* sc);
        void import_(SynTree* st, Scope* sc);
        void expr(SynTree* st, Scope* sc);
        void exprList(SynTree* st, Scope* sc);
        void block(SynTree* st, Scope* sc);
        void condition_(SynTree* st, Scope* scope);
        void assignment_(SynTree* st, Scope*);
        void lvalue(SynTree* st, Scope* sc);
        void primaryExpr(SynTree* st, Scope* sc);
        void unaryExpr(SynTree* st, Scope* sc);
        void exprNlr(SynTree* st, Scope* sc);
        void string(SynTree* st, Scope* sc);
        void arrayAccess(SynTree* st, Scope* sc);
        void scopeAccess(SynTree* st, Scope* sc);
        void varRhs(SynTree* st, Scope* sc);
        void varLhs(SynTree* st, Scope* sc);
        void list(SynTree* st, Scope* sc);
        void stringVar_(SynTree* st, Scope* sc, int pos, int len);
        void namedObj_(SynTree* st, Scope* sc, const QByteArray& kind);
        void function_(SynTree* st, Scope* sc, const QByteArray& kind);
        SynTree* findSymbolBySourcePos(SynTree*, quint32 line, quint16 col ) const;
    private:
        Errors* d_errs;
        QDir d_sourceRoot;
        typedef QHash<const char*,Scope> Files;
        Files d_files;
        QSet<const char*> d_knownVars, d_knownFuncs, d_namedObjs;
        const char* d_foreach;
        const char* d_import;
        const char* d_declare_args;
        QByteArray d_fileKind;
        VarRefs d_allRhs,d_allLhs,d_allFuncRefs,d_allImports;
        ObjRefs d_allObjDefs;
        SynTreeList d_allUnresolvedImports;
        ScopeList d_allUnnamedObjs; // not owned
        SynTreeList d_unresolvedRefs, d_declaredArgs;
    };
}

#endif // GNCODEMODEL_H
