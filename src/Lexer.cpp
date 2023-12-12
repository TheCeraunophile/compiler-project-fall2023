#include "Lexer.h"

// classifying characters
namespace charinfo
{
    // ignore whitespaces
    LLVM_READNONE inline bool isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\f' || c == '\v' ||
               c == '\r' || c == '\n';
    }

    LLVM_READNONE inline bool isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    LLVM_READNONE inline bool isLetter(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
}

void Lexer::next(Token &token)
{
    while (*BufferPtr && charinfo::isWhitespace(*BufferPtr))
    {
        ++BufferPtr;
    }
    // make sure we didn't reach the end of input
    if (!*BufferPtr)
    {
        token.Kind = Token::eoi;
        return;
    }
    // collect characters and check for keywords or ident
    if (charinfo::isLetter(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isLetter(*end))
            ++end;
        llvm::StringRef Name(BufferPtr, end - BufferPtr);
        Token::TokenKind kind;
        // if (Name == "type")
        //     kind = Token::KW_type;
        if (Name == "int")
            kind = Token::KW_int;
        else if (Name == "loopc")
            kind = Token::KW_loopc;
        else if (Name == "if")
            kind = Token::KW_if;
        else if (Name == "begin")
            kind = Token::KW_begin;
        else if (Name == "end")
            kind = Token::KW_end;
        else if (Name == "else")
            kind = Token::KW_else;
        else if (Name == "elif")
            kind = Token::KW_elif;       
        else
            kind = Token::ident;
        // generate the token
        formToken(token, end, kind);
        return;
    }
    // check for numbers
    else if (charinfo::isDigit(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isDigit(*end))
            ++end;
        formToken(token, end, Token::number);
        return;
    }
    else
    {
        if (*BufferPtr == '+' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::plus_equal);
        else if (*BufferPtr == '-' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::minus_equal);
        else if (*BufferPtr == '*' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::star_equal);
        else if (*BufferPtr == '/' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::slash_equal);
        else if (*BufferPtr == '%' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::mod_equal);
        else if (*BufferPtr == '^' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::power_equal);
        else if (*BufferPtr == '>' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::greater_equal);
        else if (*BufferPtr == '<' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::lower_equal);
        else if (*BufferPtr == '=' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::equal_equal);
        else if (*BufferPtr == '!' && *(BufferPtr+1) == '=')
            formToken(token, BufferPtr + 2, Token::not_equal);
        else if (*BufferPtr == '+')
            formToken(token, BufferPtr + 1, Token::plus);
        else if (*BufferPtr == '-')
            formToken(token, BufferPtr + 1, Token::minus);
        else if (*BufferPtr == '*')
            formToken(token, BufferPtr + 1, Token::star);
        else if (*BufferPtr == '/')
            formToken(token, BufferPtr + 1, Token::slash);
        else if (*BufferPtr == '%')
            formToken(token, BufferPtr + 1, Token::mode);
        else if (*BufferPtr == '^')
            formToken(token, BufferPtr + 1, Token::power);
        else if (*BufferPtr == '(')
            formToken(token, BufferPtr + 1, Token::l_paren);
        else if (*BufferPtr == ')')
            formToken(token, BufferPtr + 1, Token::r_paren);
        else if (*BufferPtr == ';')
            formToken(token, BufferPtr + 1, Token::semicolon);
        else if (*BufferPtr == ',')
            formToken(token, BufferPtr + 1, Token::comma);
        else if (*BufferPtr == '=')
            formToken(token, BufferPtr + 1, Token::equal);
        else if (*BufferPtr == '>')
            formToken(token, BufferPtr + 1, Token::greater);
        else if (*BufferPtr == '<')
            formToken(token, BufferPtr + 1, Token::lower);
        else if (*BufferPtr == ':')
            formToken(token, BufferPtr + 1, Token::KW_colon);
        return;
    }
}

void Lexer::formToken(Token &Tok, const char *TokEnd,
                      Token::TokenKind Kind)
{
    Tok.Kind = Kind;
    Tok.Text = llvm::StringRef(BufferPtr, TokEnd - BufferPtr);
    BufferPtr = TokEnd;
}
