#ifndef OPT_H
#define OPT_H

#include "AST.h"
#include "Lexer.h"

class Opt {
public:
  bool optimizer(AST *Tree);
};

#endif
