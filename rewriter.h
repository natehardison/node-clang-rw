#ifndef CLANG_REWRITER_REWRITER_H
#define CLANG_REWRITER_REWRITER_H

#include <string>
#include <set>

void rewrite(std::string filename, std::set<std::string> &functions);

#endif /* CLANG_REWRITER_REWRITER_H */
