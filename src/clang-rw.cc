//-------------------------------------------------------------------------
// clang-rw.cc: Source-to-source transformation with Clang that uses
// the code rewriting interface "Rewriter."
//
// NOTE: to compile in Clang 2.X, set the -DCLANG_2 flag.
//
// Author: Nate Hardison (nate@cs.harvard.edu)
// 
// Based largely off code written by Eli Bendersky (eliben@gmail.com) and
// posted at: http://eli.thegreenplace.net/2012/06/08/basic-source-to-source-transformation-with-clang/
//
// Also got significant help from the Clang tutorials at:
// https://github.com/loarabia/Clang-tutorial/wiki/TutorialOrig
//--------------------------------------------------------------------------
#include "clang-rw.h"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/HeaderSearchOptions.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Parse/Parser.h"
#include "clang/Rewrite/Rewriter.h"
#include "clang/Rewrite/Rewriters.h"

#include "llvm/Support/Host.h"

using namespace clang;

class RemoveFunctionsConsumer : public ASTConsumer
{
public:
    RemoveFunctionsConsumer(Rewriter &rw, std::set<FileID> &files,
                           std::set<std::string> &functions, SourceManager &sm)
        : rw_(rw), files_(files), functions_(functions), sm_(sm) {}

#ifdef OSX 
    virtual bool
#else
    virtual void
#endif
    HandleTopLevelDecl(DeclGroupRef dgr)
    {
        for (DeclGroupRef::iterator it = dgr.begin(); it != dgr.end(); it++)
        {
            if (const FunctionDecl *fd = llvm::dyn_cast<FunctionDecl>(*it))
            {
                // use hasBody(fd) to reset fd to point to definition
                if (fd->hasBody(fd))
                {
                    std::string name = fd->getNameInfo().getName().getAsString();
                    SourceLocation start = fd->getLocStart();
                    if (functions_.find(name) != functions_.end() && !sm_.isInSystemHeader(start))
                    {
                        // why, oh why can't you just subtract SourceLocations?
                        unsigned begin = sm_.getFileOffset(start);
                        unsigned end = sm_.getFileOffset(fd->getLocEnd());
                        unsigned length = end - begin + 1;
                        
                        // need a SourceLocation for first arg, not a file offset
                        rw_.RemoveText(start, length);

                        // remember the file ID
                        files_.insert(sm_.getFileID(start));
                    }
                }
            }
        }
// me loves the preprocessor
#ifdef OSX
        return true;
#endif
    }

private:
    Rewriter &rw_;
    std::set<FileID> &files_;
    std::set<std::string> &functions_;
    SourceManager &sm_;
};

void rewrite(std::string filename, std::set<std::string> functions)
{
    // need to remember which files get rewritten
    std::set<FileID> files;

    // --------------------------------------- //
    // !!!!!!!!!! CRAZY CLANG STUFF !!!!!!!!!! //
    // --------------------------------------- //

    // CompilerInstance manages the various objects needed to run the compiler
    CompilerInstance ci;
    ci.createDiagnostics(0, NULL);

    // Initialize target info with the default triple for our platform.
    TargetOptions to;
#ifdef OSX
    to.Triple = llvm::sys::getDefaultTargetTriple();
#else
    to.Triple = llvm::sys::getHostTriple();
#endif

    TargetInfo *ti = TargetInfo::CreateTargetInfo(ci.getDiagnostics(), to);
    ci.setTarget(ti);

    ci.createFileManager();
    FileManager &fm = ci.getFileManager();

    ci.createSourceManager(fm);
    SourceManager &sm = ci.getSourceManager();
   
    // my attempt to get clang to find stdlib headers, but it fails
    HeaderSearchOptions hso = ci.getHeaderSearchOpts();
    hso.AddPath("/usr/include", frontend::System, false, false, true);
    hso.AddPath("/usr/local/include", frontend::System, false, false, true);

    ci.createPreprocessor();
    ci.getPreprocessorOpts().UsePredefines = false;

    Rewriter rw;
    rw.setSourceMgr(sm, ci.getLangOpts());

    // Set the main file handled by the source manager to the input file.
    const FileEntry *fileIn = fm.getFile(filename);
    if (fileIn == NULL)
        throw std::invalid_argument("Invalid filename: " + filename);

    sm.createMainFileID(fileIn);
    ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(),
                                             &ci.getPreprocessor());

    // this consumer will do the heavy lifting for us
    RemoveFunctionsConsumer consumer(rw, files, functions, sm);

    // parse the AST, rewriting in the process
    ci.createASTContext();
    ParseAST(ci.getPreprocessor(), &consumer, ci.getASTContext());

    ci.getDiagnosticClient().EndSourceFile();

    // --------------------------------------- //
    // !!!!!!!! END CRAZY CLANG STUFF !!!!!!!! //
    // --------------------------------------- //

    // write the rewritten results out to new files
    std::set<FileID>::iterator b = files.begin(), e = files.end();
    for ( ; b != e; b++)
    {
        // get the appropriate RewriteBuffer using the FileID
        const RewriteBuffer *rb = rw.getRewriteBufferFor(*b);

        // I can't believe it's this hard to get a filename
        PresumedLoc ploc = sm.getPresumedLoc(sm.getLocForStartOfFile(*b));
        std::string filename = ploc.getFilename();

        // write out the results, using a suffix for now
        std::ofstream file((filename + ".rewritten").c_str(), std::ios::out);
        if (!file.is_open())
            throw std::runtime_error("Couldn't open file for rewriting.");

        file << std::string(rb->begin(), rb->end()) << "\n";

        file.close();
    }
}
