#include "CodeGen.h"
#include "/home/soheil/llvm-build/llvm-install/include/llvm/ADT/StringMap.h"
#include "/home/soheil/llvm-build/llvm-install/include/llvm/IR/IRBuilder.h"
#include "/home/soheil/llvm-build/llvm-install/include/llvm/IR/LLVMContext.h"
#include "/home/soheil/llvm-build/llvm-install/include/llvm/Support/raw_ostream.h"
#include "/home/soheil/llvm-build/llvm-install/include/llvm/IR/Intrinsics.h"

using namespace llvm;

// Define a visitor class for generating LLVM IR from the AST.
namespace
{
  class ToIRVisitor : public ASTVisitor
  {
    Module *M;
    IRBuilder<> Builder;
    Type *VoidTy;
    Type *Int32Ty;
    Type *Int8PtrTy;
    Type *Int8PtrPtrTy;
    Constant *Int32Zero;
    FunctionType *MainFty;
    Function *MainFn;
    int loopId = 0;
    int ifId = 0;

    Value *V;
    StringMap<AllocaInst *> nameMap;

  public:
    // Constructor for the visitor class.
    ToIRVisitor(Module *M) : M(M), Builder(M->getContext())
    {
      // Initialize LLVM types and constants.
      VoidTy = Type::getVoidTy(M->getContext());
      Int32Ty = Type::getInt32Ty(M->getContext());
      Int8PtrTy = Type::getInt8PtrTy(M->getContext());
      Int8PtrPtrTy = Int8PtrTy->getPointerTo();
      Int32Zero = ConstantInt::get(Int32Ty, 0, true);
    }

    // Entry point for generating LLVM IR from the AST.
    void run(AST *Tree)
    {
      // Create the main function with the appropriate function type.
      MainFty = FunctionType::get(Int32Ty, {Int32Ty, Int8PtrPtrTy}, false);
      MainFn = Function::Create(MainFty, GlobalValue::ExternalLinkage, "main", M);

      // Create a basic block for the entry point of the main function.
      BasicBlock *BB = BasicBlock::Create(M->getContext(), "entry", MainFn);
      Builder.SetInsertPoint(BB);

      // Visit the root node of the AST to generate IR.
      Tree->accept(*this);

      // Create a return instruction at the end of the main function.
      Builder.CreateRet(Int32Zero);
    }

    // Visit function for the GSM node in the AST.
    virtual void visit(GSM &Node) override
    {
      // Iterate over the children of the GSM node and visit each child.
      for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
      {
        (*I)->accept(*this);
      }
    };

    virtual void visit(Assignment &Node) override
    {
      // Visit the right-hand side of the assignment and get its value.
      Node.getRight()->accept(*this);
      Value *val = V;

      // Get the name of the variable being assigned.
      auto varName = Node.getLeft()->getVal();

      // Create a store instruction to assign the value to the variable.
      Builder.CreateStore(val, nameMap[varName]);

      // Create a function type for the "gsm_write" function.
      FunctionType *CalcWriteFnTy = FunctionType::get(VoidTy, {Int32Ty}, false);

      // Create a function declaration for the "gsm_write" function.
      Function *CalcWriteFn = Function::Create(CalcWriteFnTy, GlobalValue::ExternalLinkage, "gsm_write", M);

      // Create a call instruction to invoke the "gsm_write" function with the value.
      CallInst *Call = Builder.CreateCall(CalcWriteFnTy, CalcWriteFn, {val});
    };

    virtual void visit(Factor &Node) override
    {
      if (Node.getKind() == Factor::Ident)
      {
        // If the factor is an identifier, load its value from memory.
        V = Builder.CreateLoad(Int32Ty, nameMap[Node.getVal()]);
      }
      else
      {
        // If the factor is a literal, convert it to an integer and create a constant.
        int intval;
        Node.getVal().getAsInteger(10, intval);
        V = ConstantInt::get(Int32Ty, intval, true);
      }
    };

    virtual void visit(BinaryOp &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getLeft()->accept(*this);
      Value *Left = V;

      // Visit the right-hand side of the binary operation and get its value.
      Node.getRight()->accept(*this);
      Value *Right = V;

      // Perform the binary operation based on the operator type and create the corresponding instruction.
      switch (Node.getOperator())
      {
      case BinaryOp::Plus:
        V = Builder.CreateNSWAdd(Left, Right);
        break;
      case BinaryOp::Minus:
        V = Builder.CreateNSWSub(Left, Right);
        break;
      case BinaryOp::Mul:
        V = Builder.CreateNSWMul(Left, Right);
        break;
      case BinaryOp::Div:
        V = Builder.CreateSDiv(Left, Right);
        break;
      case BinaryOp::Greater:
        V = Builder.CreateICmpSGT(Left, Right);
        break;
      case BinaryOp::Less:
        V = Builder.CreateICmpSLT(Left, Right);
        break;
      case BinaryOp::GreaterEqual:
        V = Builder.CreateICmpSGE(Left, Right);
        break;
      case BinaryOp::LessEqual:
        V = Builder.CreateICmpSLE(Left, Right);
        break;
      case BinaryOp::Equal:
        V = Builder.CreateICmpEQ(Left, Right);
        break;
      case BinaryOp::NotEqual:
        V = Builder.CreateICmpNE(Left, Right);
        break;
      case BinaryOp::Power:
        llvm::Function *PowiFn = llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::powi, {Left->getType()});
        llvm::Value *Args[] = {Left, Right};
        llvm::CallInst *Call = llvm::CallInst::Create(PowiFn, Args, "", Builder.GetInsertBlock());
        V = Call;
        break;
      }
    };

    virtual void visit(Declaration &Node) override
    {
      Value *val = nullptr;

      if (Node.getExpr())
      {
        // If there is an expression provided, visit it and get its value.
        Node.getExpr()->accept(*this);
        val = V;
      }

      // Iterate over the variables declared in the declaration statement.
      for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
      {
        StringRef Var = *I;

        // Create an alloca instruction to allocate memory for the variable.
        nameMap[Var] = Builder.CreateAlloca(Int32Ty);

        // Store the initial value (if any) in the variable's memory location.
        if (val != nullptr)
        {
          Builder.CreateStore(val, nameMap[Var]);
        }
      }
    };

    virtual void visit(LoopStatement &Node) override
    {
      llvm::BasicBlock* ConditionLoop = llvm::BasicBlock::Create(M->getContext(), "LOOPC_CONDITION" + std::__cxx11::to_string(loopId), MainFn);
      llvm::BasicBlock* BodyLoop = llvm::BasicBlock::Create(M->getContext(), "START_LOOP" + std::__cxx11::to_string(loopId), MainFn);
      llvm::BasicBlock* MergeLoop = llvm::BasicBlock::Create(M->getContext(), "END_LOOP" + std::__cxx11::to_string(loopId), MainFn);

      Builder.CreateBr(ConditionLoop);
      Builder.SetInsertPoint(ConditionLoop);

      Node.getCon()->accept(*this);
      Value* cond = V;
      Builder.CreateCondBr(cond, BodyLoop, MergeLoop);

      Builder.SetInsertPoint(BodyLoop);

      llvm::SmallVector<Expr *> loop_expressions = Node.getExprs();

      for (auto I = loop_expressions.begin(), E=loop_expressions.end(); I !=E; I++)
      {
        (*I)->accept(*this);
      }

      Builder.CreateBr(ConditionLoop);
      Builder.SetInsertPoint(MergeLoop);
      loopId += 1;
    };

    virtual void visit(IfStatement &Node) override
    {
      llvm::BasicBlock* Condition = llvm::BasicBlock::Create(M->getContext(), "IF_CONDITION" + std::__cxx11::to_string(ifId), MainFn);
      llvm::BasicBlock* BodyIf = llvm::BasicBlock::Create(M->getContext(), "START_IF" + std::__cxx11::to_string(ifId), MainFn);
      llvm::BasicBlock* Merge = llvm::BasicBlock::Create(M->getContext(), "MERGE_AND_EXIT" + std::__cxx11::to_string(ifId), MainFn);

      Builder.CreateBr(Condition);
      Builder.SetInsertPoint(Condition);

      Node.getCon()->accept(*this);
      Value* cond = V;
      llvm::BasicBlock* ConditionElif = llvm::BasicBlock::Create(M->getContext(), "ELIF_CONDITION_1" + std::__cxx11::to_string(ifId), MainFn);
      llvm::BasicBlock* ElseStart = llvm::BasicBlock::Create(M->getContext(), "ELSE_START" + std::__cxx11::to_string(ifId), MainFn);

      if(Node.ElseIfsGet().size()!=0)
        Builder.CreateCondBr(cond, BodyIf, ConditionElif);
      else if (Node.BodyElseGet().size()!=0)
        Builder.CreateCondBr(cond, BodyIf, ElseStart);
      else
        Builder.CreateCondBr(cond, BodyIf, Merge);

      Builder.SetInsertPoint(BodyIf);

      llvm::SmallVector<Expr *> loop_expressions = Node.BodyIfGet();

      for (auto I = loop_expressions.begin(), E=loop_expressions.end(); I !=E; I++)
      {
        (*I)->accept(*this);
      }

      Builder.CreateBr(Merge);

      if(Node.ElseIfsGet().size()!=0)
      {
        int J = 0;
        while(J < Node.ElseIfsGet().size())
        {
          if(J > 0)
          {
            Builder.CreateBr(ConditionElif);
          }

          llvm::BasicBlock* BodyElif = llvm::BasicBlock::Create(M->getContext(), "START_ELIF_"  + std::__cxx11::to_string(ifId) + std::__cxx11::to_string(J+1), MainFn);
          llvm::BasicBlock* MergeElif = llvm::BasicBlock::Create(M->getContext(), "END_ELIF_" + std::__cxx11::to_string(ifId) + std::__cxx11::to_string(J+1), MainFn);

          ElseIf *BodyElseIf = Node.ElseIfsGet()[J];
          Builder.SetInsertPoint(ConditionElif);

          BodyElseIf->getCon()->accept(*this);
          cond = V;
          Builder.CreateCondBr(ConditionElif, BodyElif, MergeElif);
          Builder.SetInsertPoint(BodyElif);

          llvm::SmallVector<Expr *> elif_expressions = BodyElseIf->getExprs();
          for (auto I = elif_expressions.begin(), E=elif_expressions.end(); I !=E; I++)
          {
            (*I)->accept(*this);
          }
          Builder.CreateBr(Merge);
          J++;
          if (J == Node.ElseIfsGet().size())
          {
            Builder.SetInsertPoint(MergeElif);
            if(Node.BodyElseGet().size()!=0)
            {
              Builder.CreateBr(ElseStart);
              Builder.SetInsertPoint(ElseStart);
              llvm::SmallVector<Expr *> else_expressions = Node.BodyElseGet();
              for (auto I = else_expressions.begin(), E=else_expressions.end(); I !=E; I++)
              {
                (*I)->accept(*this);
              }
            }
            break;
          }
          ConditionElif = llvm::BasicBlock::Create(M->getContext(), "ELIF_CONDITION_" + std::__cxx11::to_string(ifId) + std::__cxx11::to_string(J+1), MainFn);
          Builder.SetInsertPoint(MergeElif);
        }
      }

      if(Node.BodyElseGet().size()!=0 && Node.ElseIfsGet().size()==0)
      {
        // Builder.CreateBr(ElseStart);
        Builder.SetInsertPoint(ElseStart);
        llvm::SmallVector<Expr *> else_expressions = Node.BodyElseGet();
        for (auto I = else_expressions.begin(), E=else_expressions.end(); I !=E; I++)
        {
          (*I)->accept(*this);
        }
      }

      Builder.CreateBr(Merge);
      Builder.SetInsertPoint(Merge);
      ifId += 1;
    };


    virtual void visit(ElseIf &Node) override {}
  };
}; // namespace

void CodeGen::compile(AST *Tree)
{
  // Create an LLVM context and a module.
  LLVMContext Ctx;
  Module *M = new Module("calc.expr", Ctx);

  // Create an instance of the ToIRVisitor and run it on the AST to generate LLVM IR.
  ToIRVisitor ToIR(M);
  ToIR.run(Tree);

  // Print the generated module to the standard output.
  M->print(outs(), nullptr);
}
