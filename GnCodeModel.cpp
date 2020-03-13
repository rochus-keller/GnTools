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

#include "GnCodeModel.h"
#include "GnErrors.h"
#include "GnLexer.h"
#include "GnParser.h"
#include <QBuffer>
#include <QtDebug>
using namespace Gn;

static const char* s_buildGn = "BUILD.gn";
static const char* s_dotFile = ".gn";

// Werte aus GN master 152c5144ceed9592c20f0 12.7.2019
static const char* s_knownVars[] =
{
    // dotfile
        "arg_file_template",
        "buildconfig",
        "check_targets",
        "exec_script_whitelist",
        "root",
        "script_executable",
        "secondary_source",
        "default_args",

    // Built-in predefined variables (type "gn help <variable>" for more help):
      "current_cpu", //: [string] The processor architecture of the current toolchain.
      "current_os", //: [string] The operating system of the current toolchain.
      "current_toolchain", //: [string] Label of the current toolchain.
      "default_toolchain", //: [string] Label of the default toolchain.
      "host_cpu", // : [string] The processor architecture that GN is running on.
      "host_os", //: [string] The operating system that GN is running on.
      "invoker", // : [string] The invoking scope inside a template.
      "python_path", //: [string] Absolute path of Python.
      "root_build_dir", //: [string] Directory where build commands are run.
      "root_gen_dir", //: [string] Directory for the toolchain's generated files.
      "root_out_dir", //: [string] Root directory for toolchain output files.
      "target_cpu", //: [string] The desired cpu architecture for the build.
      "target_gen_dir", //: [string] Directory for a target's generated files.
      "target_name", //: [string] The name of the current target.
      "target_os", //: [string] The desired operating system for the build.
      "target_out_dir", //: [string] Directory for target output files.

    // Variables you set in targets (type "gn help <variable>" for more help):
      "aliased_deps", //: [scope] Set of crate-dependency pairs.
      "all_dependent_configs", //: [label list] Configs to be forced on dependents.
      "allow_circular_includes_from", //: [label list] Permit includes from deps.
      "arflags", //: [string list] Arguments passed to static_library archiver.
      "args", //: [string list] Arguments passed to an action.
      "asmflags", //: [string list] Flags passed to the assembler.
      "assert_no_deps", //: [label pattern list] Ensure no deps on these targets.
      "bundle_contents_dir", //: Expansion of {{bundle_contents_dir}} in create_bundle.
      "bundle_deps_filter", //: [label list] A list of labels that are filtered out.
      "bundle_executable_dir", //: Expansion of {{bundle_executable_dir}} in create_bundle
      "bundle_resources_dir", //: Expansion of {{bundle_resources_dir}} in create_bundle.
      "bundle_root_dir", //: Expansion of {{bundle_root_dir}} in create_bundle.
      "cflags", //: [string list] Flags passed to all C compiler variants.
      "cflags_c", //: [string list] Flags passed to the C compiler.
      "cflags_cc", //: [string list] Flags passed to the C++ compiler.
      "cflags_objc", //: [string list] Flags passed to the Objective C compiler.
      "cflags_objcc", //: [string list] Flags passed to the Objective C++ compiler.
      "check_includes", //: [boolean] Controls whether a target's files are checked.
      "code_signing_args", //:[string list] Arguments passed to code signing script.
      "code_signing_outputs", //:[file list] Output files for code signing step.
      "code_signing_script", //:[file name] Script for code signing.
      "code_signing_sources", //:[file list] Sources for code signing step.
      "complete_static_lib", //:[boolean] Links all deps into a static library.
      "configs", //:[label list] Configs applying to this target or config.
      "contents", //:Contents to write to file.
      "crate_name", //:[string] The name for the compiled crate.
      "crate_root", //:[string] The root source file for a binary or library.
      "crate_type", //:[string] The type of linkage to use on a shared_library.
      "data", //:[file list] Runtime data file dependencies.
      "data_deps", //:[label list] Non-linked dependencies.
      "data_keys", //:[string list] Keys from which to collect metadata.
      "defines", //:[string list] C preprocessor defines.
      "depfile", //:[string] File name for input dependencies for actions.
      "deps", //:[label list] Private linked dependencies.
      "edition", //:[string] The rustc edition to use in compiliation.
      "friend", //:[label pattern list] Allow targets to include private headers.
      "include_dirs", //:[directory list] Additional include directories.
      "inputs", //:[file list] Additional compile-time dependencies.
      "ldflags", //:[string list] Flags passed to the linker.
      "lib_dirs", //:[directory list] Additional library directories.
      "libs", //:[string list] Additional libraries to link.
      "metadata", //:[scope] Metadata of this target.
      "output_conversion", //:Data format for generated_file targets.
      "output_dir", //:[directory] Directory to put output file in.
      "output_extension", //:[string] Value to use for the output's file extension.
      "output_name", //:[string] Name for the output file other than the default.
      "output_prefix_override", //:[boolean] Don't use prefix for output name.
      "outputs", //:[file list] Output files for actions and copy targets.
      "partial_info_plist", //:[filename] Path plist from asset catalog compiler.
      "pool", //:[string] Label of the pool used by the action.
      "precompiled_header", //:[string] Header file to precompile.
      "precompiled_header_type", //:[string] "gcc" or "msvc".
      "precompiled_source", //:[file name] Source file to precompile.
      "product_type", //:[string] Product type for Xcode projects.
      "public", //:[file list] Declare public header files for a target.
      "public_configs", //:[label list] Configs applied to dependents.
      "public_deps", //:[label list] Declare public dependencies.
      "rebase", //:[boolean] Rebase collected metadata as files.
      "response_file_contents", //:[string list] Contents of .rsp file for actions.
      "script", //:[file name] Script file for actions.
      "sources", //:[file list] Source files for a target.
      "testonly", //:[boolean] Declares a target must only be used for testing.
      "visibility", //:[label list] A list of labels that can depend on a target.
      "walk_keys", //:[string list] Key(s) for managing the metadata collection walk.
      "write_runtime_deps", //:Writes the target's runtime_deps to the given path.
      "xcode_extra_attributes", //:[scope] Extra attributes for Xcode projects.
      "xcode_test_application_name", //:[string] Name for Xcode test target.

    // weitere empirisch gefundene
        "toolchain_args",

    0
};
static const char* s_knownFuncs[] =
{
    // Buildfile functions (type "gn help <function>" for more help):
      "assert", //:Assert an expression is true at generation time.
      "declare_args", //:Declare build arguments.
      "defined", //:Returns whether an identifier is defined.
      "exec_script", //:Synchronously run a script and return the output.
      "foreach", //:Iterate over a list.
      "forward_variables_from", //:Copies variables from a different scope.
      "get_label_info", //:Get an attribute from a target's label.
      "get_path_info", //:Extract parts of a file or directory name.
      "get_target_outputs", //:[file list] Get the list of outputs from a target.
      "getenv", //:Get an environment variable.
      "import", //:Import a file into the current scope.
      "not_needed", //:Mark variables from scope as not needed.
      "print", //:Prints to the console.
      "process_file_template", //:Do template expansion over a list of files.
      "read_file", //:Read a file into a variable.
      "rebase_path", //:Rebase a file or directory to another location.
      "set_default_toolchain", //:Sets the default toolchain name.
      "set_defaults", //:Set default values for a target type.
      "set_sources_assignment_filter", //:Set a pattern to filter source files.
      "split_list", //:Splits a list into N different sub-lists.
      "string_replace", //:Replaces substring in the given string.
      "tool", //:Specify arguments to a toolchain tool.
      "write_file", //:Write a file to disk.
    0
};

static const char* s_namedObjs[] =
{
    // Buildfile functions (type "gn help <function>" for more help):
        "config", //:Defines a configuration object.
        "pool", //:Defines a pool object.
        "template", //:Define a template rule.
        "toolchain", //:Defines a toolchain.

    // Target declarations (type "gn help <function>" for more help):
      "action", //:Declare a target that runs a script a single time.
      "action_foreach", //:Declare a target that runs a script over a set of files.
      "bundle_data", //:[iOS/macOS] Declare a target without output.
      "copy", //:Declare a target that copies files.
      "create_bundle", //:[iOS/macOS] Build an iOS or macOS bundle.
      "executable", //:Declare an executable target.
      "generated_file", //:Declare a generated_file target.
      "group", //:Declare a named group of targets.
      "loadable_module", //:Declare a loadable module target.
      "rust_library", //:Declare a Rust library target.
      "shared_library", //:Declare a shared library target.
      "source_set", //:Declare a source set target.
      "static_library", //:Declare a static library target.
      "target", //:Declare an target with the given programmatic type.
    0
};

static QStringList collectBuildFiles( const QDir& dir )
{
    QStringList res;

    QStringList files = dir.entryList( QStringList() << QString("*.gn") << QString("*.gni"), QDir::Files, QDir::Name );
    foreach( const QString& f, files )
    {
        res.append( dir.absoluteFilePath(f) );
    }

    files = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name );
    foreach( const QString& f, files )
        res += collectBuildFiles( QDir( dir.absoluteFilePath(f) ) );

    return res;
}

CodeModel::CodeModel(QObject *parent) : QObject(parent)
{
    d_errs = new Errors(this);
    d_errs->setReportToConsole(true);

}

CodeModel::~CodeModel()
{
    clear();
}

bool CodeModel::parseDir(const QDir& dir)
{
    clear();
    const QString dotfile = findDotFile( dir );
    if( dotfile.isEmpty() )
    {
        d_errs->error(Errors::Semantics, dir.absoluteFilePath(s_dotFile), 0, 0,
                      tr("could not find any dotfile in current or super directories"));
        return false;
    }
    d_sourceRoot = QFileInfo(dotfile).absoluteDir();
    d_errs->setRoot(d_sourceRoot);

    QStringList files;
    files += dotfile;
    files += collectBuildFiles(d_sourceRoot);
    // qDebug() << "####" << files.size() << "files to parse in" << d_sourceRoot.absolutePath();
    foreach( const QString& f, files )
    {
        parseFile(f);
    }
    return d_errs->getErrCount() == 0;
}

QString CodeModel::calcPath(SynTree* ref) const
{
    Q_ASSERT( ref != 0 && ref->d_tok.d_type == Tok_string );
    return calcPath(ref->d_tok.getEscapedVal(), ref->d_tok.d_sourcePath);
}

QString CodeModel::calcPath(const QByteArray& path, const QByteArray& ref ) const
{
    QString tmp = QString::fromUtf8(path);
    if( tmp.startsWith("//") )
        tmp = d_sourceRoot.absoluteFilePath(tmp.mid(2));
    else if( tmp.startsWith("/"))
        ((void)0); // NOP, absolute path
    else if( !tmp.isEmpty() && !ref.isEmpty() )
        tmp = QFileInfo(QString::fromUtf8(ref)).absoluteDir().absoluteFilePath(tmp);
    else if( !tmp.isEmpty() )
        tmp = d_sourceRoot.absoluteFilePath(tmp);
    return QFileInfo(tmp).canonicalFilePath();
}

QString CodeModel::calcPath(QByteArray path, const QByteArray& ref, bool addBUILDgn) const
{
    if( addBUILDgn )
        path += QByteArray("/") + s_buildGn;

    return calcPath(path,ref);
}

QString CodeModel::relativePath(const QByteArray& sourcePath) const
{
    return d_sourceRoot.relativeFilePath(QString::fromUtf8(sourcePath));
}

SynTree*CodeModel::findSymbolBySourcePos(const QByteArray& sourcePath, quint32 line, quint16 col) const
{
    Files::const_iterator i = d_files.find(sourcePath.constData());
    if( i == d_files.end() )
        i = d_files.find(Lexer::getSymbol(sourcePath).constData());
    if( i == d_files.end() || i.value().d_st == 0 )
        return 0;

    return findSymbolBySourcePos( i.value().d_st, line, col );
}

SynTree*CodeModel::findFromPath(const QByteArray& path, const QByteArray& callerPath)
{
    if( path.isEmpty() )
        return 0;

    PathIdentPair pip = extractPathIdentFromString(path);
    if( pip.first.isEmpty() && pip.second.isEmpty() )
        return 0;

    QByteArray file = callerPath;
    if( !pip.first.isEmpty() )
    {
        file = calcPath( pip.first, callerPath, !pip.second.isEmpty() ).toUtf8();
        if( file.isEmpty() )
            return 0;
    }
    const Scope* s = getScope( file );
    if( s == 0 )
        return 0;
    if( !pip.second.isEmpty() )
    {
        const QByteArray name = Lexer::getSymbol(pip.second);
        Scope* o = s->findObject(name);
        if( o )
            return o->d_st;
    }else
        return s->d_st;
    return 0;
}

SynTree*CodeModel::findDefinition(const SynTree* st)
{
    if( st == 0 )
        return 0;
    if( st->d_tok.d_type == Tok_string )
        return findFromPath( st->d_tok.getEscapedVal(), st->d_tok.d_sourcePath );
    else if( st->d_tok.d_type == Tok_identifier )
    {
        ObjRefs::const_iterator i = d_allObjDefs.find( st->d_tok.d_val.constData() );
        if( i != d_allObjDefs.end() )
        {
            if( i.value().size() == 1 )
                return i.value().first()->d_st;
        }
    }
    return 0;
}

QByteArrayList CodeModel::getFileList() const
{
    QByteArrayList res;
    Files::const_iterator i;
    for( i = d_files.begin(); i != d_files.end(); ++i )
        res.append( i.value().d_name );
    std::sort( res.begin(), res.end() );
    return res;
}

CodeModel::Scope*CodeModel::getScope(const QByteArray& sourcePath) const
{
    Files::const_iterator i = d_files.find( sourcePath.constData() );
    if( i == d_files.end() )
        i = d_files.find(Lexer::getSymbol(sourcePath).constData());
    if( i == d_files.end() || i.value().d_st == 0 )
        return 0;
    else
        return const_cast<Scope*>(&i.value());
}

bool CodeModel::isKnownVar(const char* id) const
{
    return d_knownVars.contains( id );
}

bool CodeModel::isKnownObj(const QByteArray& id) const
{
    return d_knownFuncs.contains( id.constData() ) ||
            d_namedObjs.contains( id.constData() );
}

bool CodeModel::isKnownId(const QByteArray& id) const
{
    return d_knownVars.contains( id.constData() ) || d_knownFuncs.contains( id.constData() ) ||
            d_namedObjs.contains( id.constData() );
}

QString CodeModel::findDotFile(const QDir& dir)
{
    const QFileInfo path = dir.absoluteFilePath(s_dotFile);
    if( path.exists() )
        return path.absoluteFilePath();
    else
    {
        QDir up = path.absoluteDir();
        if( !up.cdUp() )
            return QString();
        else
            return findDotFile( up );
    }
}

void CodeModel::clear()
{
    d_errs->clear();
    for( Files::iterator i = d_files.begin(); i != d_files.end(); ++i )
    {
        delete i.value().d_st;
    }
    d_files.clear();
    Lexer::clearSymbols();
    d_knownVars.clear();
    d_knownFuncs.clear();
    d_namedObjs.clear();
    const char** s = 0;
    s = s_knownVars;
    while( *s )
    {
        d_knownVars.insert( Lexer::getSymbol(*s).constData() );
        s++;
    }
    s = s_knownFuncs;
    while( *s )
    {
        d_knownFuncs.insert( Lexer::getSymbol(*s).constData() );
        s++;
    }
    s = s_namedObjs;
    while( *s )
    {
        d_namedObjs.insert( Lexer::getSymbol(*s).constData() );
        s++;
    }
    d_foreach = Lexer::getSymbol("foreach").constData();
    d_import = Lexer::getSymbol("import").constData();
    d_fileKind = Lexer::getSymbol("file");
    d_declare_args = Lexer::getSymbol("declare_args");

    d_allRhs.clear();
    d_allLhs.clear();
    d_allObjDefs.clear();
    d_allFuncRefs.clear();
    d_allImports.clear();
    d_allUnresolvedImports.clear();
    d_allUnnamedObjs.clear();
    d_unresolvedRefs.clear();
    d_declaredArgs.clear();
}

CodeModel::Scope* CodeModel::parseFile(const QString& path, bool /*viaImport*/)
{
    const QByteArray pathSym = Lexer::getSymbol(path.toUtf8());
    Files::iterator i = d_files.find(pathSym.constData());
    if( i != d_files.end() )
        return &i.value();

    QFile in( path );
    if( !in.open(QIODevice::ReadOnly) )
    {
        d_errs->warning( Errors::Lexer, path, 0, 0, tr("cannot open file for reading") );
        return 0;
    }
//    qDebug() << "*** parsing file" << ( d_files.size() + 1 ) << d_sourceRoot.relativeFilePath(path) <<
//                ( viaImport ? "via import" : "" );
    Gn::Lexer lex;
    lex.setStream( &in, path );
    lex.setErrors(d_errs);
    lex.setIgnoreComments(false);
    lex.setPackComments(true);
    Gn::Parser p(&lex,d_errs);
    p.RunParser();

    Q_ASSERT( p.d_root.d_children.isEmpty() ||
              ( p.d_root.d_children.size() == 1 &&
                p.d_root.d_children.first()->d_tok.d_type == SynTree::R_StatementList ) );
    Scope* scope = 0;
    if( !p.d_root.d_children.isEmpty() )
    {
        SynTree* st = p.d_root.d_children.first();
        scope = &d_files[pathSym.constData()];
        scope->d_kind = d_fileKind;
        scope->d_name = pathSym;
        p.d_root.d_children.clear();
        // Analyze file
        Q_ASSERT( scope != 0 && st != 0 );
        statementList(st,scope);
        scope->d_st = st; // wird erst gesetzt wenn Analyse fertig
    }

    return scope;
}

void CodeModel::statementList(SynTree* st, Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_StatementList );
    foreach( SynTree* s, st->d_children )
    {
        statement(s,sc);
    }
}

void CodeModel::statement(SynTree* st, Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_Statement && st->d_children.size() == 1 );
    switch( st->d_children.first()->d_tok.d_type )
    {
    case SynTree::R_Assignment:
        assignment_( st->d_children.first(), sc );
        break;
    case SynTree::R_Call:
        call_( st->d_children.first(), sc );
        break;
    case SynTree::R_Condition:
        condition_( st->d_children.first(), sc );
        break;
    default:
        break;
    }
}

void CodeModel::call_(SynTree* st, Scope* sc)
{
    Q_ASSERT( st->d_children.size() >= 3 && st->d_children[0]->d_tok.d_type == Tok_identifier );

    const char* kind = st->d_children[0]->d_tok.d_val.constData();
    d_allFuncRefs[kind].append(st->d_children[0]);
    if( kind == d_foreach )
    {
        // no new scope
        loop_(st,sc);
    }else if( kind == d_import )
    {
        import_(st,sc);
    }else if( d_namedObjs.contains(kind)
              || !d_knownFuncs.contains(kind) ) // template instances
    {
        // new scope
        namedObj_( st, sc, st->d_children[0]->d_tok.d_val );
    }else
    {
        // new scope
        function_( st, sc, st->d_children[0]->d_tok.d_val );
    }
}

void CodeModel::loop_(SynTree* st, CodeModel::Scope* scope)
{
    if( st->d_children[2]->d_tok.d_type == Tok_Rpar || st->d_children.last()->d_tok.d_type != SynTree::R_Block )
    {
        d_errs->error(Errors::Syntax,st,tr("invalid foreach statement") );
        return;
    }
    Q_ASSERT( st->d_children[2]->d_tok.d_type == SynTree::R_ExprList && st->d_children.size() == 5
            && st->d_children.last()->d_tok.d_type == SynTree::R_Block );

    if( st->d_children[2]->d_children.size() != 2 )
    {
        d_errs->error(Errors::Syntax,st,tr("invalid expression list in foreach statement") );
        return;
    }

    SynTree* var = flatten(st->d_children[2]->d_children.first() );
    if( var->d_tok.d_type != Tok_identifier )
    {
        d_errs->error(Errors::Syntax,st,tr("invalid loop variable in foreach statement") );
    }else
        d_allLhs[var->d_tok.d_val.constData()].append(var);

    expr( st->d_children[2]->d_children.last(), scope );

    // TODO: muss man var irgendwie in den scope bringen?
    block( st->d_children.last(), scope );
}

void CodeModel::import_(SynTree* st, CodeModel::Scope* sc)
{
    // Import
    // The imported file's scope will be merged with the scope at the point import
    // was called. If there is a conflict (both the current scope and the imported
    // file define some variable or rule with the same name but different value), a
    // runtime error will be thrown. Therefore, it's good practice to minimize the
    // stuff that an imported file defines.

    if( st->d_children[2]->d_tok.d_type == Tok_Rpar )
    {
        d_errs->error(Errors::Syntax,st,tr("invalid import statement") );
        return;
    }
    Q_ASSERT( st->d_children[2]->d_tok.d_type == SynTree::R_ExprList && !st->d_children[2]->d_children.isEmpty() );
    SynTree* ref = flatten(st->d_children[2]);

    bool resolved = false;
    if( ref->d_tok.d_type == Tok_string )
    {
        string(ref,sc);
        if( ref->d_children.isEmpty() )
        {
            // Name known
            const QString path = calcPath(ref);
            if( !path.isEmpty() )
            {
                const QByteArray pathSym = Lexer::getSymbol(path.toUtf8());
                d_allImports[pathSym.constData()].append(ref);
                if( QFileInfo(path).exists() )
                {
                    // TODO: ev. verhindern dass direkt selber importiert
                    Scope* res = parseFile( path, true );
                    if( res )
                    {
                        resolved = true;
                        sc->d_relovedImports.insert(res->d_name.constData(),res);
                    }
                }else
                    d_errs->warning(Errors::Semantics, ref, tr("import file doesn't exist: %1").arg(path));
            }
        }
    }else
    {
        expr(st->d_children[2]->d_children.first(),sc);
    }
    if( !resolved )
    {
        sc->d_unresolvedImports.append(st->d_children[2]->d_children.first() );
        d_allUnresolvedImports.append(st->d_children[2]->d_children.first());
    }
}

void CodeModel::expr(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_Expr && !st->d_children.isEmpty() );

    unaryExpr( st->d_children.first(), sc );
    if( st->d_children.size() > 1 )
    {
        if( st->d_children.size() == 2 )
        {
            exprNlr( st->d_children.last(), sc );
        }else
            Q_ASSERT( false );
    }
}

void CodeModel::exprList(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_ExprList );
    foreach( SynTree* e, st->d_children )
        expr(e,sc);
}

void CodeModel::block(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_Block && st->d_children.size() == 3
              && st->d_children.first()->d_tok.d_type == Tok_Lbrace
              && st->d_children.last()->d_tok.d_type == Tok_Rbrace );

    statementList( st->d_children[1], sc );
}

SynTree* CodeModel::flatten(SynTree* st, int stopAt)
{
    if( st == 0 )
        return 0;
    while( st->d_children.size() == 1 && ( stopAt == 0 || st->d_tok.d_type != stopAt ) )
        st = st->d_children.first();
    return st;
}

SynTree*CodeModel::firstToken(SynTree* st)
{
    if( st == 0 )
        return 0;
    if( st->d_tok.d_type < TT_MaxToken )
        return st;
    foreach( SynTree* sub, st->d_children )
    {
        SynTree* res = firstToken(sub);
        if( res )
            return res;
    }
    return 0;
}

static int findNonEscapedDollar( const QByteArray& str, int start = 0 )
{
    int pos = start;
    while( pos < str.size() )
    {
        pos = str.indexOf( '$', pos );
        if( pos != -1 )
        {
            if( pos > 0 && str[pos-1] == '\\' )
            {
                pos++;
                continue;
            }else
                return pos;
        }else
            return pos;
    }
    return -1;
}

CodeModel::Dollars CodeModel::findDollars(const QByteArray& str)
{
    // str ist begrenzt durch "" und kann die Escapes \\, \$ und \" enthalten
    int pos = findNonEscapedDollar( str );
    Dollars res;
    while( pos != -1 )
    {
        int len = 0;
        if( pos + 1 < str.size() )
        {
            if( str[pos+1] == '{' )
            {
                const int pos2 = str.indexOf('}', pos+1 );
                if( pos2 == -1 )
                {
                    return res;
                }else
                {
                    len = pos2 - pos + 1;
                    res << Dollar( pos, len );
                }
            }else if( str[pos+1] == '0' )
            {
                len = 5; // hex char $0xff
                res << Dollar(pos,len);
            }else
            {
                int i = pos + 1;
                while( i < str.size() )
                {
                    if( !::isalnum( str[i] ) && str[i] != '_' )
                        break;
                    i++;
                }
                len = i - pos;
                res << Dollar(pos,len);
            }
        }
        pos = findNonEscapedDollar( str, pos + len );
    }
    return res;
}

static inline char get( const QByteArray& str, int i )
{
    if( i < str.size() )
        return str[i];
    else
        return 0;
}

static inline bool looksLikeImplicitNamePath( const QByteArray& str )
{
    Q_ASSERT( !str.contains(':') );
    if( str.isEmpty() )
        return false;

    int i = 0;
    if( str[i] == '"' )
        i++;
    int lastSlash = -1;
    if( get(str,i) == '/' ) // starts with "/"
    {
        lastSlash = i;
        i++;
        if( get(str,i) == '/' ) // starts with "//"
        {
            lastSlash = i;
            i++;
        }
    }
    while( i < str.size() )
    {
        const char c = get(str,i);
        if( c == '\\' )
            return false;
        else if( c == '/' )
        {
            if( lastSlash != -1 && i - lastSlash == 0 ) // embedded "//"
                return false;
            lastSlash = i;
        }
        i++;
    }
    i = str.size() - 1;
    if( get(str,i) == '"' )
        i--;
    if( lastSlash == i )
        return false; // ends with "/"
//    if( lastSlash == -1 )
//        return false; // no "/" at all
    // now check whether the last part has a suffix
    return str.indexOf( '.', lastSlash + 1 ) == -1; // true wenn kein '.' enthalten
}

bool CodeModel::looksLikeFilePath( const QByteArray& str )
{
    if( str.contains('/') )
        return true;
    if( str.contains('\\') )
        return true;
    if( str.contains('.') && !str.endsWith('.') )
        return true;
    return false;
}

CodeModel::PathIdentPair CodeModel::extractPathIdentFromString(QByteArray str)
{
    if( str.isEmpty() )
        return PathIdentPair();

    const int pos1 = str.indexOf(':');
    if( pos1 == -1 )
    {
        // allow for implicit names
        if( looksLikeImplicitNamePath(str) )
        {
            QByteArray name;
            const int pos = str.lastIndexOf('/');
            if( pos != -1 )
                name = str.mid(pos+1);
            //else
                // name = str; // TODO: we require at least one "/" which might be too strict
            return PathIdentPair(str,name);
        }
        return PathIdentPair(str,QByteArray()); // path only
#if 0   // no, if ':' is missing and no implicit path/name we always conclude path here
        if( looksLikeFilePath(str) )
            return PathIdentPair(str,QByteArray()); // path only
        else if( str.contains(' ') )
            return PathIdentPair(); // neither path nor name
        else
            return PathIdentPair(QByteArray(),str); // name only
#endif
    }
    if( str.indexOf(':', pos1+1 ) != -1 )
        return PathIdentPair(); // invalid format

    PathIdentPair res;
    const int pos2 = str.indexOf( '(', pos1+1 );
    if( pos2 != -1 )
    {
        if( pos2 - pos1 == 1 )
            return PathIdentPair(); // invalid ident
        else
            res.second = str.mid(pos1+1, pos2-pos1-1 );
    }else
        res.second = str.mid(pos1+1);

    res.first = str.left(pos1);
    return res;
}

void CodeModel::condition_(SynTree* st, CodeModel::Scope* scope)
{
    Q_ASSERT( st->d_tok.d_type == SynTree::R_Condition && st->d_children.size() >= 5 );
    expr( st->d_children[2], scope );
    block( st->d_children[4], scope );
    if( st->d_children.size() > 5 )
    {
        Q_ASSERT( st->d_children.size() == 7 );
        if( st->d_children[6]->d_tok.d_type == SynTree::R_Condition )
            condition_( st->d_children[6], scope );
        else
            block( st->d_children[6], scope );
    }
}

void CodeModel::assignment_(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_children.size() == 3 && st->d_children.first()->d_tok.d_type == SynTree::R_LValue &&
            st->d_children[1]->d_tok.d_type == SynTree::R_AssignOp &&
              st->d_children.last()->d_tok.d_type == SynTree::R_Expr );
    lvalue( st->d_children.first(), sc );
    expr( st->d_children.last(), sc );
}

void CodeModel::lvalue(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_LValue && !st->d_children.isEmpty()
            && st->d_children.first()->d_tok.d_type == Tok_identifier );

    SynTree* id = st->d_children.first();

    if( st->d_children.size() > 1 )
    {
        if( st->d_children[1]->d_tok.d_type == Tok_Lbrack ) // ArrayAccess
        {
            Q_ASSERT( st->d_children.size() == 4 && st->d_children[3]->d_tok.d_type == Tok_Rbrack );
            varLhs(id,sc);
            expr( st->d_children[2], sc );
        }else if( st->d_children[1]->d_tok.d_type == Tok_Dot ) // ScopeAccess
        {
            Q_ASSERT( st->d_children.size() == 3 && st->d_children[2]->d_tok.d_type == Tok_identifier );
            varRhs(id,sc); // TODO ist das richtig?
            varLhs(st->d_children[2],sc);
        }else
            Q_ASSERT( false );
    }else // plain ident
        varLhs(id,sc);
}

void CodeModel::primaryExpr(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && !st->d_children.isEmpty() );

    switch( st->d_children.first()->d_tok.d_type )
    {
    case SynTree::R_Call:
        call_( st->d_children.first(), sc );
        break;
    case Tok_string:
        string( st->d_children.first(), sc ); // parses Tok_string if $ included
        break;
    case Tok_Lpar:
        Q_ASSERT( st->d_children.size() == 3 );
        expr( st->d_children[1], sc );
        break;
    case SynTree::R_Scope_:
        Q_ASSERT( !st->d_children.first()->d_children.isEmpty() );
        block( st->d_children.first()->d_children.first(), sc ); // TODO: neuer scope?
        break;
    case Tok_identifier:
        varRhs(st->d_children.first(), sc);
        break;
    case SynTree::R_ArrayAccess:
        arrayAccess(st->d_children.first(), sc);
        break;
    case SynTree::R_ScopeAccess:
        scopeAccess(st->d_children.first(), sc);
        break;
    case SynTree::R_List_:
        list(st->d_children.first(), sc);
        break;
    }
}

void CodeModel::unaryExpr(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_UnaryExpr );

    if( st->d_children.size() == 1 )
    {
        primaryExpr( st->d_children.first(), sc );
    }else if( st->d_children.size() == 2 )
    {
        Q_ASSERT( st->d_children.first()->d_tok.d_type == SynTree::R_UnaryOp );
        unaryExpr( st->d_children.last(), sc );
    }else
        Q_ASSERT( false );
}

void CodeModel::exprNlr(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_Expr_nlr_ && st->d_children.size() >= 2 );

    Q_ASSERT( st->d_children.first()->d_tok.d_type == SynTree::R_BinaryOp );
    expr( st->d_children[1], sc );
    if( st->d_children.size() > 2 )
    {
        if( st->d_children.size() == 3 )
        {
            exprNlr( st->d_children.last(), sc );
        }else
            Q_ASSERT(false);
    }
}

void CodeModel::string(SynTree* st, CodeModel::Scope* sc)
{
    // ev. findDollars verwenden
    Q_ASSERT( st != 0 && st->d_tok.d_type == Tok_string );

    const QByteArray str = st->d_tok.d_val;
    int pos = findNonEscapedDollar( str );
    while( pos != -1 )
    {
        int len = 0;
        if( pos + 1 < str.size() )
        {
            if( str[pos+1] == '{' )
            {
                const int pos2 = str.indexOf('}', pos+1 );
                if( pos2 == -1 )
                {
                    d_errs->error(Errors::Syntax, st, tr("'${' without terminating '}'") );
                    return;
                }else
                {
                    len = pos2 - pos;
                    stringVar_( st, sc, pos+2, pos2 - pos - 2 );
                }
            }else if( str[pos+1] == '0' )
                len = 5; // hex char $0xff
            else
            {
                int i = pos + 1;
                while( i < str.size() )
                {
                    if( !::isalnum( str[i] ) && str[i] != '_' )
                        break;
                    i++;
                }
                len = i - pos;
                stringVar_( st, sc, pos+1, i - pos - 1 );
            }
        }
        pos = findNonEscapedDollar( str, pos + len );
    }
    PathIdentPair pip = extractPathIdentFromString(str);
    if( !pip.second.isEmpty() && !pip.second.startsWith('\\') ) // avoid Windows paths
    {
        const bool dlr = findNonEscapedDollar( pip.second ) != -1;
        if( !dlr )
        {
            QByteArray name = pip.second;
            if( name.endsWith('"') )
                name.chop(1); // get rid of trailing "
            name = Lexer::getSymbol(name);
            d_allFuncRefs[name.constData()].append(st);
        }
        if( dlr || findNonEscapedDollar( pip.first ) != -1 )
            d_unresolvedRefs.append(st);
    }
}

void CodeModel::arrayAccess(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_ArrayAccess && st->d_children.size() == 4
            && st->d_children[1]->d_tok.d_type == Tok_Lbrack
              && st->d_children[3]->d_tok.d_type == Tok_Rbrack );
    varRhs( st->d_children.first(), sc );
    expr( st->d_children[2], sc );
}

void CodeModel::scopeAccess(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_ScopeAccess && st->d_children.size() == 3
            && st->d_children[1]->d_tok.d_type == Tok_Dot );
    varRhs( st->d_children.first(), sc );
    varRhs( st->d_children.last(), sc ); // meistens invoker.name
}

void CodeModel::varRhs(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == Tok_identifier );

    sc->d_rhs[st->d_tok.d_val.constData()].append(st);
    d_allRhs[st->d_tok.d_val.constData()].append(st);
}

void CodeModel::varLhs(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == Tok_identifier );

    sc->d_lhs[st->d_tok.d_val.constData()].append(st);
    d_allLhs[st->d_tok.d_val.constData()].append(st);
    if( sc->d_kind == d_declare_args )
        d_declaredArgs.append(st);
}

void CodeModel::list(SynTree* st, CodeModel::Scope* sc)
{
    Q_ASSERT( st != 0 && st->d_tok.d_type == SynTree::R_List_ && st->d_children.size() >= 2
            && st->d_children.first()->d_tok.d_type == Tok_Lbrack
            && st->d_children.last()->d_tok.d_type == Tok_Rbrack );
    for( int i = 1; i < st->d_children.size() - 1; i++ )
    {
        expr( st->d_children[i], sc );
    }
}

static void remap( SynTree* st, SynTree* ref, int pos )
{
    st->d_tok.d_lineNr = ref->d_tok.d_lineNr;
    st->d_tok.d_colNr += ref->d_tok.d_colNr + pos - 1;
    st->d_tok.d_sourcePath = ref->d_tok.d_sourcePath;
    foreach( SynTree* sub, st->d_children )
        remap( sub, ref, pos );
}

void CodeModel::stringVar_(SynTree* st, CodeModel::Scope* sc, int pos, int len)
{
    QBuffer buf;
    const QByteArray str = st->d_tok.d_val.mid(pos,len);
    buf.setData(str);
    buf.open(QIODevice::ReadOnly);
    Gn::Lexer lex;
    lex.setStream( &buf, QString("%1:%2:%3").arg(st->d_tok.d_sourcePath.constData())
                   .arg(st->d_tok.d_lineNr).arg(st->d_tok.d_colNr+pos));
    lex.setErrors(d_errs);
    lex.setIgnoreComments(true);
    Gn::Parser p(&lex,d_errs);
    p.ParsePrimaryExpr();

    Q_ASSERT( p.d_root.d_children.size() <= 1 );
    if( !p.d_root.d_children.isEmpty() )
    {
        SynTree* primary = p.d_root.d_children.first();
        Q_ASSERT( primary->d_tok.d_type == SynTree::R_PrimaryExpr && !primary->d_children.isEmpty() );
        switch( primary->d_children.first()->d_tok.d_type )
        {
        case SynTree::R_ArrayAccess:
        case SynTree::R_ScopeAccess:
        case Tok_identifier:
            Q_ASSERT( primary->d_children.size() == 1 );
            remap( primary, st, pos );
            primaryExpr(primary,sc);
            st->d_children.append( primary->d_children.first() );
            primary->d_children.clear(); // damit nicht mit parser gelöscht
            break;
        default:
            d_errs->error(Errors::Syntax,st,tr("embedding of %1 in strings not allowed")
                          .arg(SynTree::rToStr(primary->d_children.first()->d_tok.d_type)));
            break;
        }
    }
}

void CodeModel::namedObj_(SynTree* st, CodeModel::Scope* sc, const QByteArray& kind )
{
    if( st->d_children[2]->d_tok.d_type == Tok_Rpar )
    {
        d_errs->error(Errors::Syntax,st,tr("invalid named object statement") );
        return;
    }
    Q_ASSERT( st->d_children[2]->d_tok.d_type == SynTree::R_ExprList && !st->d_children[2]->d_children.isEmpty()
            && st->d_children.size() >= 4 );

    Scope* newScope = new Scope();
    newScope->d_outer = sc;
    sc->d_allScopes.append(newScope);
    newScope->d_st = st;
    newScope->d_kind = kind;

    // source_set, config, action, action_foreach et. kommt auch mit ident "target_name" oder varianten
    // davon als name statt string,aber immer wenn in templates
    SynTree* name = flatten(st->d_children[2]->d_children.first() );
    if( name->d_tok.d_type == Tok_string )
    {
        string(name,sc);
        newScope->d_params = name;
        if( name->d_children.isEmpty() )
        {
            // Name known
            const QByteArray sym = Lexer::getSymbol(name->d_tok.getEscapedVal());
            sc->d_objectDefs.insert(sym.constData(),newScope);
            d_allObjDefs[sym.constData()].append(newScope);
            newScope->d_name = sym;
        }else
            d_allUnnamedObjs.append(newScope);
    }else
    {
        // Name not yet known
        newScope->d_params = st->d_children[2]->d_children.first(); // single expression
        expr(newScope->d_params,sc);
        d_allUnnamedObjs.append(newScope);
    }

    if( st->d_children.size() > 4 )
    {
        if( st->d_children.size() == 5 )
            block( st->d_children.last(), newScope );
        else
            Q_ASSERT( false );
    }
}

void CodeModel::function_(SynTree* st, CodeModel::Scope* sc, const QByteArray& kind)
{
    Q_ASSERT( st->d_children.size() >= 3 );
    if( st->d_children[2]->d_tok.d_type == SynTree::R_ExprList )
        exprList(st->d_children[2],sc);

    if( st->d_children.last()->d_tok.d_type == SynTree::R_Block )
    {
        Scope* newScope = new Scope();
        newScope->d_outer = sc;
        sc->d_allScopes.append(newScope);
        newScope->d_st = st;
        newScope->d_kind = kind;
        if( st->d_children[2]->d_tok.d_type == SynTree::R_ExprList )
            newScope->d_params = st->d_children[2];
        block( st->d_children.last(), newScope );
    }
}

SynTree*CodeModel::findSymbolBySourcePos(SynTree* st, quint32 line, quint16 col) const
{
    foreach( SynTree* sub, st->d_children )
    {
        if( sub->d_tok.d_lineNr <= line)
        {
            SynTree* res = findSymbolBySourcePos(sub,line,col);
            if( res )
                return res;
        }
    }
    if( st->d_tok.d_lineNr == line && st->d_tok.d_colNr <= col &&
            col <= ( st->d_tok.d_colNr + st->d_tok.d_len ) )
        return st;
    else
        return 0;
}

CodeModel::Scope::~Scope()
{
    foreach( Scope* sc, d_allScopes )
        delete sc;
}

CodeModel::Scope*CodeModel::Scope::findObject(const QByteArray& name, bool recursive, bool imports) const
{
    Scope* s = d_objectDefs.value(name.constData());
    if( s )
        return s;
    if( recursive && d_outer )
        s = d_outer->findObject(name,recursive);
    if( s )
        return s;
    if( imports )
    {
        ScopeHash::const_iterator i;
        for( i = d_relovedImports.begin(); i != d_relovedImports.end(); i++ )
        {
            s = i.value()->findObject(name,recursive);
            if( s )
                return s;
        }
    }
    return 0;
}
