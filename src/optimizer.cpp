#include "optimizer.h"
#include "/home/soheil/llvm-build/llvm-install/include/llvm/ADT/StringSet.h"
#include "/home/soheil/llvm-build/llvm-install/include/llvm/Support/raw_ostream.h"

#include <iostream>
#include <algorithm>
#include <string>

using namespace std;

Assignment * LastResultAssignment;
Expr * RightHandSide;

bool ResultFinder;
bool OnCheck;
bool Flag = false;

string VariableName[100];
int VariableNameNextIndex = 1;
int VariableNameLastIndex = 0;

int ResoltPointer = 0;

namespace {
class InputCheck : public ASTVisitor {
  llvm::StringSet<> Scope; // StringSet to store declared variables
  bool HasError; // Flag to indicate if an error occurred

  enum ErrorType { Twice, Not }; // Enum to represent error types: Twice - variable declared twice, Not - variable not declared

  void error(ErrorType ET, llvm::StringRef V) {
    // Function to report errors
    llvm::errs() << "Variable " << V << " is "
                 << (ET == Twice ? "already" : "not")
                 << " declared\n";
    HasError = true; // Set error flag to true
  }

public:
  InputCheck() : HasError(false) {} // Constructor

  bool hasError() { return HasError; } // Function to check if an error occurred

  // Visit function for GSM nodes
  virtual void visit(GSM &Node) override { 
    for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
    {
      (*I)->accept(*this); // Visit each child node
    }
  };

  // Visit function for Factor nodes
  virtual void visit(Factor &Node) override {
    if (OnCheck && Node.getKind() == Factor::Ident)
      {
        if (VariableName[VariableNameLastIndex] != Node.getVal())
        {
          auto it = std::find(std::begin(VariableName), std::end(VariableName), Node.getVal().str().c_str());
          if (it != std::end(VariableName)) {}
          else {
            llvm::errs() << Node.getVal().str().c_str()<<", ";
            VariableName[VariableNameNextIndex++] = Node.getVal().str().c_str();
          }
        }
        }
    if (Node.getKind() == Factor::Ident) {
      // Check if identifier is in the scope
      if (Scope.find(Node.getVal()) == Scope.end())
        error(Not, Node.getVal());
    }
  };

  // Visit function for BinaryOp nodes
  virtual void visit(BinaryOp &Node) override {
    if (Node.getLeft())
      Node.getLeft()->accept(*this);
    else
      HasError = true;

    auto right = Node.getRight();
    if (right)
      right->accept(*this);
    else
      HasError = true;

    if (Node.getOperator() == BinaryOp::Operator::Div && right) {
      Factor * f = (Factor *)right;

      if (right && f->getKind() == Factor::ValueKind::Number) {
        int intval;
        f->getVal().getAsInteger(10, intval);

        if (intval == 0) {
          llvm::errs() << "Division by zero is not allowed." << "\n";
          HasError = true;
        }
      }
    }
  };

  // Visit function for Assignment nodes
  virtual void visit(Assignment &Node) override {
    if (!Flag)
    {
      ResoltPointer++;
    }
    Factor *dest = Node.getLeft();

    dest->accept(*this);

    if (dest->getKind() == Factor::Number) {
        llvm::errs() << "Assignment destination must be an identifier.";
        HasError = true;
    }

    if (dest->getKind() == Factor::Ident) {
      if (ResultFinder && !OnCheck)
        if (dest->getVal().str().find(VariableName[VariableNameLastIndex]) != std::string::npos) {
            // system(dest->getVal().str().c_str());
            LastResultAssignment = &Node;
            if(!Flag)
              Flag = true;
        }
      if (!ResultFinder && !OnCheck)
      {
        if (dest->getVal().str().find(VariableName[VariableNameLastIndex]) != std::string::npos)
        {
            if (LastResultAssignment == &Node)
            {
                RightHandSide = Node.getRight();
                OnCheck = true;
            }
        }
      }
      // Check if the identifier is in the scope
      if (Scope.find(dest->getVal()) == Scope.end())
        error(Not, dest->getVal());
    }

    if (Node.getRight())
      Node.getRight()->accept(*this);
    OnCheck = false;
  };

  virtual void visit(Declaration &Node) override {
    if (ResultFinder)
    {
      ResoltPointer++;
    }    
    for (auto I = Node.begin(), E = Node.end(); I != E;
         ++I)
    {
      if (ResultFinder && !OnCheck)
        if (((llvm::StringRef *)I)->str().find(VariableName[VariableNameLastIndex]) != std::string::npos) {
            // system(((llvm::StringRef *)I)->str().c_str());
        }
      if (!Scope.insert(*I).second)
        error(Twice, *I); // If the insertion fails (element already exists in Scope), report a "Twice" error
    }
    if (Node.getExpr())
      Node.getExpr()->accept(*this); // If the Declaration node has an expression, recursively visit the expression node
  };

  virtual void visit(LoopStatement &Node) override {
    
  };
  virtual void visit(ElseIf &Node) override {

  };
  virtual void visit(IfStatement &Node) override {

  };
};
}

void Opt::optimizer(AST *Tree) {
  if (!Tree)
    return ; // If the input AST is not valid, return false indicating no errors

  InputCheck Checks[100];
  int CheckIndex = 0;


  VariableName[0] = "Result";

  // InputCheck Check; // Create an instance of the InputCheck class for semantic analysis
  ResultFinder = true;
  OnCheck = false;

  GSM * tmp = (GSM *)Tree;
  llvm::SmallVector<Expr *> commands = tmp->getExprs();

  Tree->accept(Checks[CheckIndex++]); // Find the last Assignment to the Result value by traversing the AST using the accept function
  commands.erase(commands.begin() + ResoltPointer - 1, commands.end());

  llvm::errs() << "Resolt Fount on " << ResoltPointer - 1<< "th command: \n";
  GSM * alter = new GSM(commands);
  Tree = (AST *) alter;

  ResultFinder = false;
  llvm::errs() << "Live variables: ";
  Tree->accept(Checks[CheckIndex++]); // Find the last Assignment to the Result value by traversing the AST using the accept function

  while (VariableNameLastIndex < VariableNameNextIndex - 1)
  {
    VariableNameLastIndex ++;
    OnCheck = false;

    ResultFinder = true;    
    Tree->accept(Checks[CheckIndex++]); // Find the last Assignment to the Result value by traversing the AST using the accept function

    ResultFinder = false;
    Tree->accept(Checks[CheckIndex++]); // Find the last Assignment to the Result value by traversing the AST using the accept function
  }
  llvm::errs() << "\n";
  Opt::Tree = Tree;
  return;
}
