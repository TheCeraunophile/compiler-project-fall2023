#ifndef OPT_H
#define OPT_H

#include "AST.h"
#include "Lexer.h"

class Opt {
public:
  void optimizer(AST *Tree);
};

#endif
