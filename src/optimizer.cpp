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

// string OnDelete;
bool DeleteTime;
bool AsertDelete = false;

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
            VariableName[VariableNameNextIndex++] = Node.getVal().str().c_str();
          }
        }
        }
    if (Node.getKind() == Factor::Ident) {
      // Check if identifier is in the scope
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

        // if (intval == 0) {
        //   llvm::errs() << "Division by zero is not allowed." << "\n";
        //   HasError = true;
        // }
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

    if (dest->getKind() == Factor::Ident) {
      if (ResultFinder && !OnCheck)
        if (dest->getVal().str().find(VariableName[VariableNameLastIndex]) != std::string::npos) {
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
      if (DeleteTime)
      {
        AsertDelete = true;
        for(int i=0; i<VariableNameNextIndex; i++){
          if (dest->getVal().str().find(VariableName[i]) != std::string::npos){
            llvm::errs() << "Variable " << dest->getVal().str() << " is not dead\n";
            AsertDelete = false;
          }
        }
        if (AsertDelete)
        {
          llvm::errs() << "Variable " << dest->getVal().str() << " is dead\n";
        }
      }
      // Check if the identifier is in the scope
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
      if (DeleteTime)
      {
        AsertDelete = true;
        for(int i=0; i<VariableNameNextIndex; i++)
        {
          if (((llvm::StringRef *)I)->str().find(VariableName[i]) != std::string::npos){
            llvm::errs() << "Variable " << ((llvm::StringRef *)I)->str() << " is not dead\n";
            AsertDelete = false;
            break;
          }
        }
        if (AsertDelete)
        {
          llvm::errs() << "Variable " << ((llvm::StringRef *)I)->str() << " is dead\n";
        }
      }
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

  DeleteTime = false;

  InputCheck Checks[100];
  int CheckIndex = 0;

  AsertDelete = false;

  VariableName[0] = "Result";

  // InputCheck Check; // Create an instance of the InputCheck class for semantic analysis
  ResultFinder = true;
  OnCheck = false;

  GSM * tmp = (GSM *)Tree;
  llvm::SmallVector<Expr *> commands = tmp->getExprs();

  Tree->accept(Checks[CheckIndex++]); // Find the last Assignment to the Result value by traversing the AST using the accept function
  commands.erase(commands.begin() + ResoltPointer , commands.end());

  llvm::errs() << "Resolt Fount on " << ResoltPointer << "th command: \n";
  GSM * alter = new GSM(commands);
  Tree = (AST *) alter;

  ResultFinder = false;
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

  tmp = (GSM *)Tree;
  commands = tmp->getExprs();

  llvm::errs() << "Live variables: ";
  for (int i=0; i<VariableNameNextIndex; i++)
    llvm::errs() << VariableName[i] << ", ";
  llvm::errs() << "\n";

  ResultFinder = false;
  OnCheck = false;
  int length = commands.size();
  DeleteTime = true;

  for (int j=0; j<length; j++)
  {
    llvm::errs() << "Checking " << j << "\n";
    AsertDelete = false;
    commands[j]->accept(Checks[CheckIndex++]);
    if (AsertDelete)
    {
      llvm::errs() << "DELETE " << j << "\n";
      commands.erase(commands.begin() + j, commands.begin() + j + 1);
      length--;
    }
  }
  
  alter = new GSM(commands);
  Tree = (AST *) alter;

  Opt::Tree = Tree;
  return;
}
