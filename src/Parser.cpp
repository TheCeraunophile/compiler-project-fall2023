#include "Parser.h"
// #include "llvm/IR/BasicBlock.h"
#include "/home/soheil/llvm-build/llvm-install/include/llvm/IR/IRBuilder.h"


// main point is that the whole input has been consumed
AST *Parser::parse()
{
    AST *Res = parseGSM();
    return Res;
}

AST *Parser::parseGSM()
{
    llvm::SmallVector<Expr *> exprs;
    Expr *a;
    while (!Tok.is(Token::eoi))
    {
        switch (Tok.getKind())
        {
        case Token::KW_int:
            if (!parseDec(exprs))
                goto _error2;            
            break;
        case Token::ident:
            a = parseAssign();
            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error2;
            }
            if (a)
                exprs.push_back(a);
            else
                goto _error2;
            break;
        case Token::KW_if:
            if(!parseIf(exprs))
                goto _error2;
            break;
        case Token::KW_loopc:
            a = parseLoop();
            if(a)
                exprs.push_back(a);
            else
                goto _error2;
            break;
        default:
            goto _error2;
            break;
        }
        advance(); // TODO: watch this part
    }
    return new GSM(exprs);
_error2:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

bool Parser::parseDec(llvm::SmallVector<Expr *> &exprs)
{
    llvm::SmallVector<Expr *> middle_exprs;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    
    int repetation = 0;

    if (expect(Token::KW_int))
        goto _error;

    advance();

    if (expect(Token::ident))
        goto _error;
    Vars.push_back(Tok.getText());
    advance();

    while (Tok.is(Token::comma))
    {
        repetation += 1;
        advance();
        if (expect(Token::ident))
            goto _error;
        Vars.push_back(Tok.getText());
        advance();
    }

    if (Tok.is(Token::equal))
    {
        // Read the first expression
        advance();
        middle_exprs.push_back(parseExpr());
        // 
        while (Tok.is(Token::comma))
        {
            repetation -= 1;
            advance();
            middle_exprs.push_back(parseExpr());
        }
    }

    if (expect(Token::semicolon))
        goto _error;
    
    if(repetation < 0)
        goto _error;

    for (size_t i = 0; i < Vars.size(); ++i) {
        llvm::SmallVector<llvm::StringRef, 8> var;
        var.push_back(Vars[i]);

        Expr *expr = nullptr;
        if (i < middle_exprs.size()) {
            expr = middle_exprs[i];
        } else {
            expr = new Factor(Factor::Number, "0");
        }

        exprs.push_back(new Declaration(var, expr));
    }

    return true;
_error: // TODO: Check this later in case of error :)
    while (Tok.getKind() != Token::eoi)
        advance();
    return false;
}

bool Parser::parseIf(llvm::SmallVector<Expr *> &exprs)
{
    Expr *a;
    advance();

    Expr * Con;
    Con = parseCon();
    if (!Con)
        goto _error4;

    exprs.push_back(Con);

    if (!Tok.is(Token::comma))
            goto _error4;
    advance();

    if (!Tok.is(Token::KW_begin))
            goto _error4;
    advance();

    while ((!Tok.is(Token::KW_end)) && (Tok.getKind() != Token::eoi))
    {
        switch (Tok.getKind())
        {
        case Token::ident:
            a = parseAssign();
            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error4;
            }
            if (a)
                exprs.push_back(a);
            else
                goto _error4;
            break;
        default:
            goto _error4;
        }
        advance();
    }

    if (!Tok.is(Token::KW_end))
        goto _error4;
    
    advance();

    if(Tok.getKind() == Token::Token::KW_elif)
    {
        while ((Tok.getKind() == Token::KW_elif) && (Tok.getKind() != Token::eoi))
        {
            advance();
            Expr * Con;
            Con = parseCon();
            if (!Con)
                goto _error4;
            exprs.push_back(Con);

            if (!Tok.is(Token::comma))
                    goto _error4;
            advance();

            if (!Tok.is(Token::KW_begin))
                    goto _error4;
            advance();

            while ((!Tok.is(Token::KW_end)) && (Tok.getKind() != Token::eoi))
            {
                switch (Tok.getKind())
                {
                case Token::ident:
                    a = parseAssign();
                    if (!Tok.is(Token::semicolon))
                    {
                        error();
                        goto _error4;
                    }
                    if (a)
                        exprs.push_back(a);
                    else
                        goto _error4;
                    break;
                default:
                    goto _error4;
                }
                advance();
            }
            if (!Tok.is(Token::KW_end))
                goto _error4;
            advance();
        }
    }

    if(Tok.getKind() == Token::Token::KW_else)
    {
        advance();
        if (!Tok.is(Token::KW_begin))
            goto _error4;
        advance();
        
        while ((!Tok.is(Token::KW_end)) && (Tok.getKind() != Token::eoi))
        {
            switch (Tok.getKind())
            {
            case Token::ident:
                a = parseAssign();
                if (!Tok.is(Token::semicolon))
                {
                    error();
                    goto _error4;
                }
                if (a)
                    exprs.push_back(a);
                else
                    goto _error4;
                break;
            default:
                goto _error4;
            }
            advance();
        }
        if (!Tok.is(Token::KW_end))
            goto _error4;
    }

    //   return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
    //                                   std::move(Else));

    return true;

_error4:
    while (Tok.getKind() != Token::eoi)
        advance();
    return false;
}

Expr *Parser::parseLoop()
{
    llvm::SmallVector<Expr *> exprs;

    Expr *a;
    advance();

    Expr * Con;
    Con = parseCon();
    if (!Con)
        goto _error5;

    exprs.push_back(Con);

    if (!Tok.is(Token::comma))
            goto _error5;
    advance();

    if (!Tok.is(Token::KW_begin))
            goto _error5;
    advance();

    while ((!Tok.is(Token::KW_end)) && (Tok.getKind() != Token::eoi))
    {
        switch (Tok.getKind())
        {
        case Token::ident:
            a = parseAssign();
            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error5;
            }
            if (a)
                exprs.push_back(a);
            else
                goto _error5;
            break;
        default:
            goto _error5;
        }
        advance();
    }

    if (!Tok.is(Token::KW_end))
        goto _error5;
    
    return new LoopStatement(exprs, Con);

_error5:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseCon()
{
    Expr *L;
    L = parseExpr();
    
    BinaryOp::Operator Op;

    switch (Tok.getKind())
    {
    case Token::greater:
        Op = BinaryOp::Greater;
        break;
    case Token::lower:
        Op = BinaryOp::Less;
        break;
    case Token::greater_equal:
        Op = BinaryOp::GreaterEqual;
        break;
    case Token::lower_equal:
        Op = BinaryOp::LessEqual;
        break;
    case Token::equal_equal:
        Op = BinaryOp::Equal;
        break;
    case Token::not_equal:
        Op = BinaryOp::NotEqual;
        break;
    }
    advance();

    Expr *R;
    R = parseExpr();
    if(!R)
        goto _error3;
    return  new BinaryOp(Op, L, R);

_error3:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseAssign()
{
    Expr *E;
    Factor *F;
    F = (Factor *)(parseFactor());

    Expr *d;
    BinaryOp::Operator Op;

    switch (Tok.getKind())
    {
    case Token::equal:
        advance();
        E = parseExpr();
        return new Assignment(F, E);
        break;
    case Token::plus_equal:
        advance();
        d = parseExpr();
        Op = BinaryOp::Plus;
        E =  new BinaryOp(Op, F, d);
        break;
    case Token::minus_equal:
        advance();
        d = parseExpr();
        Op = BinaryOp::Minus;
        E = new BinaryOp(Op, F, d);
        break;
    case Token::star_equal:
        advance();
        d = parseExpr();
        Op = BinaryOp::Mul;
        E = new BinaryOp(Op, F, d);
        break;
    case Token::slash_equal:
        advance();
        d = parseExpr();
        Op = BinaryOp::Div;
        E = new BinaryOp(Op, F, d);
        break;
    case Token::power_equal:
        advance();
        d = parseExpr();
        Op = BinaryOp::Power;
        E = new BinaryOp(Op, F, d);
        break;
    case Token::mod_equal:
        advance();
        d = parseExpr();
        Op = BinaryOp::Mode;
        E = new BinaryOp(Op, F, d);
        break;
    default:
        break;
    }
    return new Assignment(F, E);
}

Expr *Parser::parseExpr()
{
    Expr *Left = parseTerm();
    while (Tok.isOneOf(Token::plus, Token::minus))
    {
        BinaryOp::Operator Op =
            Tok.is(Token::plus) ? BinaryOp::Plus : BinaryOp::Minus;
        advance();
        Expr *Right = parseTerm();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseTerm()
{
    Expr *Left = parseNew();
    while (Tok.isOneOf(Token::star, Token::slash))
    {
        BinaryOp::Operator Op =
            Tok.is(Token::star) ? BinaryOp::Mul : BinaryOp::Div;
        advance();
        Expr *Right = parseNew();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseNew()
{
    Expr *Left = parseFactor();
    while (Tok.is(Token::power))
    {
        BinaryOp::Operator Op = BinaryOp::Power;
        advance();
        Expr *Right = parseFactor();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFactor()
{
    Expr *Res = nullptr;
    switch (Tok.getKind())
    {
    case Token::number:
        Res = new Factor(Factor::Number, Tok.getText());
        advance();
        break;
    case Token::ident:
        Res = new Factor(Factor::Ident, Tok.getText());
        advance();
        break;
    case Token::l_paren:
        advance();
        Res = parseExpr();
        if (!consume(Token::r_paren))
            break;
    default: // error handling
        if (!Res)
            error();
        while (!Tok.isOneOf(Token::r_paren, Token::star, Token::plus, Token::minus, Token::slash, Token::eoi, Token::power, Token::mode))
            advance();
        break;
    }
    return Res;
}
