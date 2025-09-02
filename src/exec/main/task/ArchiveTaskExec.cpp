
#include "ArchiveTaskExec.h"
#include "../../ExecManager.h"
#include "../../stexcept.h"
#include "../../../compiler/Archiver.h"
#include "../../../shell/shell.h"
#include "../../../io/io.h"
#include "../../../util/strutil.h"
#include "../../../output/output.h"
#include "../../../msg/messagebuilder.h"

#include "../../../consts.h"
#include "../../../error_messages.h"
#include "../../../info_messages.h"

using std::endl;

void ArchiveTaskExec::exec( void* mgr ) {
    ExecManager* manager = (ExecManager*)mgr;
    SourceCodeManager* sourceCodeManager = manager->getSourceCodeManager();

    Output& out = manager->out;
    bool isVerbose = manager->getMainCMDArgManager()->isVerbose( tasks::ARCHIVE );
    bool isShowCMDOutput = manager->getMainCMDArgManager()->isShowCMDOutput( tasks::ARCHIVE );
    
    MainScript* script = manager->getMainScript();

    string archiver = script->getPropertyValue( props::ARCHIVER );
    string archiverParams = script->getPropertyValue( props::ARCHIVER_PARAMS );

    string outputFileName = script->getPropertyValue( props::OUTPUT_FILE_NAME );

    string binDir = script->getPropertyValue( props::BIN_DIR );
    string objDir = script->getPropertyValue( props::OBJ_DIR );

    string libDirs = script->getPropertyValue( props::LIB_DIRS );
    string libs = script->getPropertyValue( props::LIBS );

    string defines = script->getPropertyValue( props::DEFINES );

    binDir = io::path::absoluteResolvePath( binDir );
    objDir = io::path::absoluteResolvePath( objDir );

    binDir = io::path::addSeparatorToDirIfNeed( binDir );
    objDir = io::path::addSeparatorToDirIfNeed( objDir );

    if ( archiver == "" )
        archiver = consts::DEFAULT_ARCHIVER;

    if ( outputFileName == "" ) {
        messagebuilder b( errors::PROPERTY_NOT_DEFINED_FOR_ARCHIVING );
        b << props::OUTPUT_FILE_NAME;
        throw st_error( nullptr, b.str() );
    }

    vector<string> objectCodeFiles;
    vector<CodeInfo*> sourceCodeInfos = sourceCodeManager->sourceCodeInfos();
    for( CodeInfo* info : sourceCodeInfos )
        objectCodeFiles.push_back( objDir + info->objFilePath );

    if ( objectCodeFiles.empty() ) {
        out << output::green( infos::NOTHING_TO_ARCHIVE ) << endl;
    } else {
        Archiver* arcr = new Archiver();
        arcr->setArchiver( archiver );
        arcr->setArchiverParams( archiverParams );
        arcr->setDefines( defines );
        arcr->setLibraryDirs( libDirs );
        arcr->setLibraries( libs );
        arcr->setObjectCodeFiles( objectCodeFiles );
        arcr->setOutputFile( binDir + outputFileName );
        string cmdline = arcr->buildCMDLine();    

        delete arcr;

        Shell* shell = new Shell( out );
        shell->setVerbose( isVerbose );
        shell->setShowOutput( isShowCMDOutput );
        shell->pushCommand( cmdline );

        int exitCode = shell->execute();
        if ( exitCode != 0 )
            throw st_error( nullptr, errors::ARCHIVING_FAILED );

        delete shell;
    }
}
