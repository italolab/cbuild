
#include "CompileTaskExec.h"
#include "../../ExecManager.h"
#include "../../stexcept.h"
#include "../../../compiler/Compiler.h"
#include "../../../darv/MainScript.h"
#include "../../../shell/shell.h"
#include "../../../io/io.h"
#include "../../../sourcecode/SourceCodeManager.h"
#include "../../../util/strutil.h"
#include "../../../output/output.h"
#include "../../../msg/messagebuilder.h"

#include "../../../error_messages.h"
#include "../../../info_messages.h"
#include "../../../consts.h"

#include <sstream>
#include <stdexcept>

using std::stringstream;
using std::endl;

void CompileTaskExec::exec( void* mgr ) {
    ExecManager* manager = (ExecManager*)mgr;
    SourceCodeManager* sourceCodeManager = manager->getSourceCodeManager();
    SourceCodeManager* testSourceCodeManager = manager->getTestSourceCodeManager();

    MainScript* script = manager->getMainScript();

    bool isCompileAll = manager->getMainCMDArgManager()->isCompileAll();

    bool isVerbose;
    bool isShowCMDOutput;
    if ( isCompileAll ) {
        isVerbose = manager->getMainCMDArgManager()->isVerbose( tasks::COMPILEALL );
        isShowCMDOutput = manager->getMainCMDArgManager()->isShowCMDOutput( tasks::COMPILEALL );
    } else {
        isVerbose = manager->getMainCMDArgManager()->isVerbose( tasks::COMPILE );
        isShowCMDOutput = manager->getMainCMDArgManager()->isShowCMDOutput( tasks::COMPILE );
    }

    Output& out = manager->out;
    bool isNoResume = manager->getMainCMDArgManager()->isNoResume();

    if ( isVerbose )
        out << endl;
        
    if ( !isNoResume || isVerbose ) {
        if ( isCompileAll ) {
            out << infos::EXECUTING << " " << output::green( tasks::COMPILEALL ) << "..." << endl;
        } else {
            out << infos::EXECUTING << " " << output::green( tasks::COMPILE ) << "..." << endl;
        }
    }

    if ( isCompileAll )
        manager->executaUserTaskIfExists( tasks::COMPILEALL, TaskExecution::BEFORE );
    else manager->executaUserTaskIfExists( tasks::COMPILE, TaskExecution::BEFORE );

    string compiler = script->getPropertyValue( props::COMPILER );
    string exeFileName = script->getPropertyValue( props::OUTPUT_FILE_NAME );

    string binDir = script->getPropertyValue( props::BIN_DIR );
    string objDir = script->getPropertyValue( props::OBJ_DIR );
    string testDir = script->getPropertyValue( props::TEST_DIR );
    string srcDir = script->getPropertyValue( props::SRC_DIR );

    binDir = io::absoluteResolvePath( binDir );
    objDir = io::absoluteResolvePath( objDir );

    binDir = io::addSeparatorToDirIfNeed( binDir );
    objDir = io::addSeparatorToDirIfNeed( objDir );

    this->appCreateDirs( binDir, manager );
    this->appCreateDirs( objDir, manager );

    if ( compiler == "" )
        compiler = consts::DEFAULT_COMPILER;

    vector<TCompilation*> filesToCompile;

    this->loadFilesToCompile( filesToCompile, mgr );

    Shell* shell = new Shell( out );
    shell->setVerbose( isVerbose );
    shell->setShowOutput( isShowCMDOutput );

    int i = 0;
    for( TCompilation* compilation : filesToCompile ) {
        Compiler* comp = new Compiler();
        comp->setCompiler( compiler );
        comp->setCompilerParams( compilation->compilerParams );
        comp->setDefines( compilation->defines );
        comp->setIncludeDirs( compilation->includeDirs );
        comp->setObjectCodeFile( compilation->objFile );
        comp->setSourceCodeFile( compilation->srcFile );

        string cmdline = comp->buildCMDLine();

        delete comp;

        shell->pushCommand( cmdline );

        i++;
    }

    int exitCode = shell->execute();
    delete shell;
    if ( exitCode != 0 )
        throw st_error( nullptr, errors::COMPILING_FAILED );

    sourceCodeManager->saveLastWriteTimesInFile( consts::LAST_WRITE_TIMES_FILE, false );
    testSourceCodeManager->saveLastWriteTimesInFile( consts::LAST_WRITE_TIMES_FILE, true );

    if ( isCompileAll )
        manager->executaUserTaskIfExists( tasks::COMPILEALL, TaskExecution::AFTER );
    else manager->executaUserTaskIfExists( tasks::COMPILE, TaskExecution::AFTER );

    if ( isVerbose ) {
        if ( filesToCompile.empty() )
            out << infos::COMPILATION_UP_TO_DATE << endl;
        else out << infos::SUCCESS_IN_COMPILATION << endl;
    }
}

void CompileTaskExec::loadFilesToCompile( vector<Compilation*>& compilations, void* mgr ) {
    ExecManager* manager = (ExecManager*)mgr;
    SourceCodeManager* sourceCodeManager = manager->getSourceCodeManager();
    SourceCodeManager* testSourceCodeManager = manager->getTestSourceCodeManager();

    bool isCompileAll = manager->getMainCMDArgManager()->isCompileAll();

    MainScript* script = manager->getMainScript();

    string compilerParams = script->getPropertyValue( props::COMPILER_PARAMS );
    string testCompilerParams = script->getPropertyValue( props::TEST_COMPILER_PARAMS );

    string includeDirs = script->getPropertyValue( props::INCLUDE_DIRS );
    string defines = script->getPropertyValue( props::DEFINES );

    string testIncludeDirs = script->getPropertyValue( props::TEST_INCLUDE_DIRS );
    string testDefines = script->getPropertyValue( props::TEST_DEFINES );

    string objDir = script->getPropertyValue( props::OBJ_DIR );
    string testDir = script->getPropertyValue( props::TEST_DIR );
    string srcDir = script->getPropertyValue( props::SRC_DIR );

    objDir = io::absoluteResolvePath( objDir );
    srcDir = io::absoluteResolvePath( srcDir );

    objDir = io::addSeparatorToDirIfNeed( objDir );

    testIncludeDirs += " " + srcDir;
   
    vector<CodeInfo*> sourceCodeInfos = sourceCodeManager->sourceCodeInfos();
    for( CodeInfo* info : sourceCodeInfos ) {
        string dir = io::dirPath( objDir + info->objFilePath );
        if ( dir != "" )
            this->appCreateDirs( dir, manager );
    }

    vector<CodeInfo*> filesToCompile;
    if ( isCompileAll ) {
        filesToCompile = sourceCodeInfos;
    } else {
        sourceCodeManager->loadFilesToCompile( filesToCompile, consts::LAST_WRITE_TIMES_FILE );
    }

    for( CodeInfo* sourceCodeInfo : filesToCompile ) {
        Compilation* comp = new Compilation();
        comp->compilerParams = compilerParams;
        comp->includeDirs = includeDirs;
        comp->defines = defines;
        comp->objFile = objDir + sourceCodeInfo->objFilePath;
        comp->srcFile = sourceCodeInfo->filePath;

        compilations.push_back( comp );
    } 

    if ( testDir != "" ) {
        vector<CodeInfo*> testSourceCodeInfos = testSourceCodeManager->sourceCodeInfos();
        for( CodeInfo* info : testSourceCodeInfos ) {
            string dir = io::dirPath( objDir + info->objFilePath );
            if ( dir != "" )
                this->appCreateDirs( dir, manager );
        }

        vector<CodeInfo*> testFilesToCompile;
        if ( isCompileAll ) {
            testFilesToCompile = testSourceCodeInfos;
        } else {
            testSourceCodeManager->loadFilesToCompile( testFilesToCompile, consts::LAST_WRITE_TIMES_FILE );
        }

        filesToCompile.insert( filesToCompile.end(), testFilesToCompile.begin(), testFilesToCompile.end() );

        for( CodeInfo* sourceCodeInfo : testFilesToCompile ) {
            Compilation* comp = new Compilation();
            comp->compilerParams = testCompilerParams;
            comp->includeDirs = testIncludeDirs;
            comp->defines = testDefines;
            comp->objFile = objDir + sourceCodeInfo->objFilePath;
            comp->srcFile = sourceCodeInfo->filePath;
            compilations.push_back( comp );
        }
    }
}

void CompileTaskExec::appCreateDirs( string dirPath, void* mgr ) {
    ExecManager* manager = (ExecManager*)mgr;
    CMD* mainCMD = manager->getMainCMD();

    try {
        io::createDirs( dirPath );
    } catch ( const io_error& e ) {
        messagebuilder b( errors::DIRECTORY_NOT_CREATED );
        b << io::absolutePath( dirPath );
        throw st_error( mainCMD, b.str() );
    }
}
