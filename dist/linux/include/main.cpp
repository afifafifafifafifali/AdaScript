/*
AdaScript
(C) 2025 Afif Ali Saadman, Chief Author of AdaScript
License: LGPL
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <optional>
#include <functional>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <unordered_set>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <chrono>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifndef _WIN32
  #ifndef ADASCRIPT_NO_CURL
    #include <curl/curl.h>
  #endif
#endif
#include "AdaScript.h"

// Forward declarations
struct Value;
struct Environment;
struct Expr;
struct Stmt;
class Interpreter;

using Ptr = std::shared_ptr<void>;

// Value type
using List = std::vector<Value>;
using Dict = std::unordered_map<std::string, Value>;

struct Function; // user-defined
struct NativeFunction; // builtin
struct Class;
struct Instance;

using ValueData = std::variant<std::monostate, bool, double, std::string, List, Dict,
                               std::shared_ptr<Function>, std::shared_ptr<NativeFunction>,
                               std::shared_ptr<Class>, std::shared_ptr<Instance>>;

struct Value {
    ValueData data;

    Value() : data(std::monostate{}) {}
    template<typename T>
    Value(T v) : data(std::move(v)) {}

    bool isNull() const { return std::holds_alternative<std::monostate>(data); }
    std::string typeName() const {
        if (std::holds_alternative<std::monostate>(data)) return "null";
        if (std::holds_alternative<bool>(data)) return "bool";
        if (std::holds_alternative<double>(data)) return "number";
        if (std::holds_alternative<std::string>(data)) return "string";
        if (std::holds_alternative<List>(data)) return "list";
        if (std::holds_alternative<Dict>(data)) return "dict";
        if (std::holds_alternative<std::shared_ptr<Function>>(data)) return "function";
        if (std::holds_alternative<std::shared_ptr<NativeFunction>>(data)) return "native";
        if (std::holds_alternative<std::shared_ptr<Class>>(data)) return "class";
        if (std::holds_alternative<std::shared_ptr<Instance>>(data)) return "instance";
        return "unknown";
    }
};

struct RuntimeError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Lexer
enum class TokenType {
    // Single-char
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, DOT, SEMICOLON, COLON,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    BANG, EQUAL, LESS, GREATER,

    // Two-char
    BANG_EQUAL, EQUAL_EQUAL, LESS_EQUAL, GREATER_EQUAL,
    AND_AND, OR_OR,

    // Literals
    IDENTIFIER, STRING, NUMBER,

    // Keywords
    LET, FUNC, CLASS, RETURN, IF, ELSE, WHILE, FOR, TRUE, FALSE, NULL_T,
    THIS, STRUCT, UNION, NEW, IMPORT, IN,
    // Textual operators
    NOT_KW, AND_KW, OR_KW, EQUALS_KW,

    END_OF_FILE
};

struct Token { TokenType type; std::string lexeme; int line; int col; };

struct Lexer {
    std::string src; size_t start=0, current=0; int line=1, col=1;
    std::vector<Token> tokens;

    explicit Lexer(std::string s): src(std::move(s)) {}

    static bool isAlpha(char c){return std::isalpha((unsigned char)c)||c=='_';}
    static bool isAlnum(char c){return isAlpha(c)||std::isdigit((unsigned char)c);}    
    bool isAtEnd() const { return current >= src.size(); }
    char advance(){ char c=src[current++]; if(c=='\n'){line++; col=1;} else col++; return c;}
    bool match(char expected){ if(isAtEnd()||src[current]!=expected) return false; current++; col++; return true; }
    char peek() const { return isAtEnd()? '\0': src[current];}
    char peekNext() const { return (current+1>=src.size())? '\0': src[current+1];}
    void add(TokenType t){ tokens.push_back({t, src.substr(start, current-start), line, col}); }

    void string(){ while(!isAtEnd() && peek()!='"'){ advance(); }
        if(isAtEnd()) throw RuntimeError("Unterminated string"); advance(); // closing quote
        std::string value = src.substr(start+1, (current-1)-(start+1));
        tokens.push_back({TokenType::STRING, value, line, col}); }
    void number(){ while(std::isdigit((unsigned char)peek())) advance(); if(peek()=='.' && std::isdigit((unsigned char)peekNext())){ advance(); while(std::isdigit((unsigned char)peek())) advance(); }
        tokens.push_back({TokenType::NUMBER, src.substr(start, current-start), line, col}); }
void identifier(){ while(isAlnum(peek())) advance(); std::string text = src.substr(start, current-start);
        static std::unordered_map<std::string, TokenType> kw = {
            {"let",TokenType::LET},{"func",TokenType::FUNC},{"class",TokenType::CLASS},{"return",TokenType::RETURN},
            {"if",TokenType::IF},{"else",TokenType::ELSE},{"while",TokenType::WHILE},{"for",TokenType::FOR},
            {"true",TokenType::TRUE},{"false",TokenType::FALSE},{"null",TokenType::NULL_T},{"this",TokenType::THIS},
            {"struct",TokenType::STRUCT},{"union",TokenType::UNION},{"new",TokenType::NEW},
            {"import",TokenType::IMPORT},{"in",TokenType::IN},
            // textual operators mapped to dedicated token types
            {"not",TokenType::NOT_KW},{"and",TokenType::AND_KW},{"or",TokenType::OR_KW},{"equals",TokenType::EQUALS_KW}
        };
        auto it=kw.find(text); if(it!=kw.end()) tokens.push_back({it->second,text,line,col}); else tokens.push_back({TokenType::IDENTIFIER,text,line,col}); }

    std::vector<Token> scan(){
        while(!isAtEnd()){
            start=current; char c=advance();
            switch(c){
                case '(': add(TokenType::LEFT_PAREN); break;
                case ')': add(TokenType::RIGHT_PAREN); break;
                case '{': add(TokenType::LEFT_BRACE); break;
                case '}': add(TokenType::RIGHT_BRACE); break;
                case '[': add(TokenType::LEFT_BRACKET); break;
                case ']': add(TokenType::RIGHT_BRACKET); break;
                case ',': add(TokenType::COMMA); break;
                case '.': add(TokenType::DOT); break;
                case ';': add(TokenType::SEMICOLON); break;
                case ':': add(TokenType::COLON); break;
                case '+': add(TokenType::PLUS); break;
                case '-': add(TokenType::MINUS); break;
                case '*': add(TokenType::STAR); break;
                case '%': add(TokenType::PERCENT); break;
                case '!': add(match('=')?TokenType::BANG_EQUAL:TokenType::BANG); break;
                case '=': add(match('=')?TokenType::EQUAL_EQUAL:TokenType::EQUAL); break;
                case '<': add(match('=')?TokenType::LESS_EQUAL:TokenType::LESS); break;
                case '>': add(match('=')?TokenType::GREATER_EQUAL:TokenType::GREATER); break;
                case '/': if(match('/')){ while(peek()!='\n' && !isAtEnd()) advance(); } else add(TokenType::SLASH); break;
                case '&': if(match('&')) add(TokenType::AND_AND); else throw RuntimeError("Unexpected '&'"); break;
                case '|': if(match('|')) add(TokenType::OR_OR); else throw RuntimeError("Unexpected '|'"); break;
                case ' ': case '\r': case '\t': /* skip */ break;
                case '\n': /* handled in advance */ break;
                case '"': string(); break;
                default:
                    if(std::isdigit((unsigned char)c)) number();
                    else if(isAlpha(c)) identifier();
                    else throw RuntimeError("Unexpected character");
            }
        }
        tokens.push_back({TokenType::END_OF_FILE, "", line, col});
        return tokens;
    }
};

// AST definitions (minimal)
struct Expr { virtual ~Expr()=default; };
struct Stmt { virtual ~Stmt()=default; };
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct LiteralExpr : Expr { Value value; explicit LiteralExpr(Value v): value(std::move(v)){} };
struct VarExpr : Expr { std::string name; explicit VarExpr(std::string n): name(std::move(n)){} };
struct AssignExpr : Expr { std::string name; ExprPtr value; AssignExpr(std::string n, ExprPtr v): name(std::move(n)), value(std::move(v)){} };
struct BinaryExpr : Expr { ExprPtr left; Token op; ExprPtr right; BinaryExpr(ExprPtr l, Token o, ExprPtr r): left(std::move(l)), op(std::move(o)), right(std::move(r)){} };
struct UnaryExpr : Expr { Token op; ExprPtr right; UnaryExpr(Token o, ExprPtr r): op(std::move(o)), right(std::move(r)){} };
struct GroupingExpr : Expr { ExprPtr expr; explicit GroupingExpr(ExprPtr e): expr(std::move(e)){} };
struct CallExpr : Expr { ExprPtr callee; std::vector<ExprPtr> args; CallExpr(ExprPtr c, std::vector<ExprPtr>a): callee(std::move(c)), args(std::move(a)){} };
struct GetExpr : Expr { ExprPtr object; std::string name; GetExpr(ExprPtr o, std::string n): object(std::move(o)), name(std::move(n)){} };
struct SetExpr : Expr { ExprPtr object; std::string name; ExprPtr value; SetExpr(ExprPtr o,std::string n,ExprPtr v):object(std::move(o)),name(std::move(n)),value(std::move(v)){} };
struct IndexExpr : Expr { ExprPtr object; ExprPtr index; IndexExpr(ExprPtr o, ExprPtr i): object(std::move(o)), index(std::move(i)){} };
struct SetIndexExpr : Expr { ExprPtr object; ExprPtr index; ExprPtr value; SetIndexExpr(ExprPtr o, ExprPtr i, ExprPtr v): object(std::move(o)), index(std::move(i)), value(std::move(v)){} };

struct ExprStmt : Stmt { ExprPtr expr; explicit ExprStmt(ExprPtr e): expr(std::move(e)){} };
struct LetStmt : Stmt { std::string name; ExprPtr initializer; LetStmt(std::string n, ExprPtr i): name(std::move(n)), initializer(std::move(i)){} };
struct BlockStmt : Stmt { std::vector<StmtPtr> stmts; explicit BlockStmt(std::vector<StmtPtr> s): stmts(std::move(s)){} };
struct IfStmt : Stmt { ExprPtr cond; StmtPtr thenB; std::optional<StmtPtr> elseB; IfStmt(ExprPtr c, StmtPtr t, std::optional<StmtPtr> e): cond(std::move(c)), thenB(std::move(t)), elseB(std::move(e)){} };
struct WhileStmt : Stmt { ExprPtr cond; StmtPtr body; WhileStmt(ExprPtr c, StmtPtr b): cond(std::move(c)), body(std::move(b)){} };
struct ReturnStmt : Stmt { std::optional<ExprPtr> value; explicit ReturnStmt(std::optional<ExprPtr> v): value(std::move(v)){} };

struct FunctionStmt : Stmt { std::string name; std::vector<std::string> params; std::shared_ptr<BlockStmt> body; };
struct ClassStmt : Stmt { std::string name; std::unordered_map<std::string, std::shared_ptr<FunctionStmt>> methods; };
struct StructStmt : Stmt { std::string name; std::vector<std::string> fields; };
struct UnionStmt : Stmt { std::string name; std::vector<std::string> tags; };
struct ForStmt : Stmt { std::string var; ExprPtr iterable; StmtPtr body; ForStmt(std::string v, ExprPtr it, StmtPtr b): var(std::move(v)), iterable(std::move(it)), body(std::move(b)){} };
struct ImportStmt : Stmt { std::string path; explicit ImportStmt(std::string p): path(std::move(p)){} };
struct MultiAssignStmt : Stmt { std::vector<std::string> names; ExprPtr value; MultiAssignStmt(std::vector<std::string> n, ExprPtr v): names(std::move(n)), value(std::move(v)){} };
struct MultiLetStmt : Stmt { std::vector<std::string> names; explicit MultiLetStmt(std::vector<std::string> n): names(std::move(n)){} };

// Parser (simplified)
struct Parser {
    const std::vector<Token>& tokens; size_t current=0;
    explicit Parser(const std::vector<Token>& ts): tokens(ts) {}

    bool isAtEnd() const { return peek().type==TokenType::END_OF_FILE; }
    const Token& peek() const { return tokens[current]; }
    const Token& previous() const { return tokens[current-1]; }
    bool check(TokenType t) const { return !isAtEnd() && peek().type==t; }
    const Token& advance(){ if(!isAtEnd()) current++; return previous(); }
    bool match(std::initializer_list<TokenType> types){ for(auto t: types){ if(check(t)){ advance(); return true; } } return false; }
const Token& consume(TokenType t, const char* msg){ if(check(t)) return advance();
        std::ostringstream emsg; emsg<<msg<<" at line "<<peek().line<<", col "<<peek().col; throw RuntimeError(emsg.str()); }

    std::vector<StmtPtr> parse(){ std::vector<StmtPtr> stmts; while(!isAtEnd()) stmts.push_back(declaration()); return stmts; }

    StmtPtr declaration(){
        if(match({TokenType::LET})) return letDecl();
        if(match({TokenType::FUNC})) return funcDecl();
        if(match({TokenType::CLASS})) return classDecl();
        if(match({TokenType::STRUCT})) return structDecl();
        if(match({TokenType::UNION})) return unionDecl();
        if(match({TokenType::IMPORT})) return importStmt();
        return statement();
    }

    StmtPtr letDecl(){
        // Support: let a;  let a = expr;  let a,b,c = expr;
        std::vector<std::string> names;
        names.push_back(consume(TokenType::IDENTIFIER, "Expected variable name").lexeme);
        while(match({TokenType::COMMA})){
            names.push_back(consume(TokenType::IDENTIFIER, "Expected variable name").lexeme);
        }
        if(match({TokenType::EQUAL})){
            auto init = expression();
            consume(TokenType::SEMICOLON, "Expected ';'");
            if(names.size()==1) return std::make_shared<LetStmt>(names[0], init);
            return std::make_shared<MultiAssignStmt>(std::move(names), init);
        }
        // No initializer -> define as null
        consume(TokenType::SEMICOLON, "Expected ';'");
        if(names.size()==1) return std::make_shared<LetStmt>(names[0], std::make_shared<LiteralExpr>(Value()));
        return std::make_shared<MultiLetStmt>(std::move(names));
    }

    std::shared_ptr<FunctionStmt> functionBody(std::string name){
        consume(TokenType::LEFT_PAREN, "Expected '('"); std::vector<std::string> params; if(!check(TokenType::RIGHT_PAREN)){ do{ params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name").lexeme);} while(match({TokenType::COMMA})); }
        consume(TokenType::RIGHT_PAREN, "Expected ')'");
        auto body = block();
        auto f = std::make_shared<FunctionStmt>(); f->name = std::move(name); f->params = std::move(params); f->body = body; return f;
    }

    StmtPtr funcDecl(){ auto name = consume(TokenType::IDENTIFIER, "Expected function name").lexeme; auto f = functionBody(name); return std::static_pointer_cast<Stmt>(f); }

    StmtPtr classDecl(){ auto name = consume(TokenType::IDENTIFIER, "Expected class name").lexeme; consume(TokenType::LEFT_BRACE, "Expected '{'"); std::unordered_map<std::string, std::shared_ptr<FunctionStmt>> methods; while(!check(TokenType::RIGHT_BRACE)){
            consume(TokenType::FUNC, "Expected method"); auto mname = consume(TokenType::IDENTIFIER, "Expected method name").lexeme; auto m = functionBody(mname); methods[mname]=m; }
        consume(TokenType::RIGHT_BRACE, "Expected '}'"); auto cls = std::make_shared<ClassStmt>(); cls->name = name; cls->methods = std::move(methods); return cls; }

    StmtPtr structDecl(){ auto name = consume(TokenType::IDENTIFIER, "Expected struct name").lexeme; consume(TokenType::LEFT_BRACE, "Expected '{'"); std::vector<std::string> fields; while(!check(TokenType::RIGHT_BRACE)){
            fields.push_back(consume(TokenType::IDENTIFIER, "Expected field name").lexeme); consume(TokenType::SEMICOLON, "Expected ';' after field"); }
        consume(TokenType::RIGHT_BRACE, "Expected '}'"); auto st = std::make_shared<StructStmt>(); st->name = name; st->fields = std::move(fields); return st; }

    StmtPtr unionDecl(){ auto name = consume(TokenType::IDENTIFIER, "Expected union name").lexeme; consume(TokenType::LEFT_BRACE, "Expected '{'"); std::vector<std::string> tags; while(!check(TokenType::RIGHT_BRACE)){
            tags.push_back(consume(TokenType::IDENTIFIER, "Expected tag name").lexeme); consume(TokenType::SEMICOLON, "Expected ';' after tag"); }
        consume(TokenType::RIGHT_BRACE, "Expected '}'"); auto un = std::make_shared<UnionStmt>(); un->name = name; un->tags = std::move(tags); return un; }

StmtPtr statement(){
        if(match({TokenType::IF})) return ifStmt();
        if(match({TokenType::WHILE})) return whileStmt();
        if(match({TokenType::FOR})) return forStmt();
        if(match({TokenType::LEFT_BRACE})) return block();
        if(match({TokenType::RETURN})){
            std::optional<ExprPtr> val;
            if(!check(TokenType::SEMICOLON)) val = expression();
            consume(TokenType::SEMICOLON, "Expected ';'");
            return std::make_shared<ReturnStmt>(val);
        }
        // Multi-assign like: a, b, c = expr;
        if(check(TokenType::IDENTIFIER)){
            size_t save = current;
            std::vector<std::string> names;
            names.push_back(advance().lexeme);
            if(check(TokenType::COMMA)){
                while(match({TokenType::COMMA})){
                    names.push_back(consume(TokenType::IDENTIFIER, "Expected identifier in multi-assign").lexeme);
                }
                consume(TokenType::EQUAL, "Expected '=' in multi-assign");
                auto rhs = expression();
                consume(TokenType::SEMICOLON, "Expected ';'");
                return std::make_shared<MultiAssignStmt>(std::move(names), rhs);
            }
            current = save; // rollback if not actually a multi-assign
        }
        auto e = expression(); consume(TokenType::SEMICOLON, "Expected ';'"); return std::make_shared<ExprStmt>(e);
    }

    std::shared_ptr<BlockStmt> block(){
        if(previous().type!=TokenType::LEFT_BRACE) consume(TokenType::LEFT_BRACE, "Expected '{'");
        std::vector<StmtPtr> stmts; while(!check(TokenType::RIGHT_BRACE)) stmts.push_back(declaration());
        consume(TokenType::RIGHT_BRACE, "Expected '}'"); return std::make_shared<BlockStmt>(std::move(stmts));
    }

    StmtPtr ifStmt(){ consume(TokenType::LEFT_PAREN, "Expected '('"); auto cond = expression(); consume(TokenType::RIGHT_PAREN, "Expected ')'"); auto thenB = statement(); std::optional<StmtPtr> elseB; if(match({TokenType::ELSE})) elseB = statement(); return std::make_shared<IfStmt>(cond, thenB, elseB); }
    StmtPtr whileStmt(){ consume(TokenType::LEFT_PAREN, "Expected '('"); auto cond = expression(); consume(TokenType::RIGHT_PAREN, "Expected ')'"); auto body = statement(); return std::make_shared<WhileStmt>(cond, body); }
    StmtPtr forStmt(){ consume(TokenType::LEFT_PAREN, "Expected '('"); auto nameTok = consume(TokenType::IDENTIFIER, "Expected loop variable"); consume(TokenType::IN, "Expected 'in'"); auto iter = expression(); consume(TokenType::RIGHT_PAREN, "Expected ')'"); auto body = statement(); return std::make_shared<ForStmt>(nameTok.lexeme, iter, body); }
    StmtPtr importStmt(){ auto pathTok = consume(TokenType::STRING, "Expected string path after import"); consume(TokenType::SEMICOLON, "Expected ';'"); return std::make_shared<ImportStmt>(pathTok.lexeme); }

    ExprPtr expression(){ return assignment(); }

    ExprPtr assignment(){ auto expr = orExpr(); if(match({TokenType::EQUAL})){
            auto equals = previous(); auto value = assignment(); if(auto v = std::dynamic_pointer_cast<VarExpr>(expr)) return std::make_shared<AssignExpr>(v->name, value);
            if(auto g = std::dynamic_pointer_cast<GetExpr>(expr)) return std::make_shared<SetExpr>(g->object, g->name, value);
            if(auto ix = std::dynamic_pointer_cast<IndexExpr>(expr)) return std::make_shared<SetIndexExpr>(ix->object, ix->index, value);
            throw RuntimeError("Invalid assignment target"); }
        return expr; }

ExprPtr orExpr(){ auto expr = andExpr(); while(match({TokenType::OR_OR, TokenType::OR_KW})){
            Token op = previous(); if(op.type==TokenType::OR_KW){ Token norm = op; norm.type = TokenType::OR_OR; op = norm; }
            auto right = andExpr(); expr = std::make_shared<BinaryExpr>(expr, op, right); }
        return expr; }
ExprPtr andExpr(){ auto expr = equality(); while(match({TokenType::AND_AND, TokenType::AND_KW})){
            Token op = previous(); if(op.type==TokenType::AND_KW){ Token norm = op; norm.type = TokenType::AND_AND; op = norm; }
            auto right = equality(); expr = std::make_shared<BinaryExpr>(expr, op, right); }
        return expr; }
ExprPtr equality(){ auto expr = comparison(); while(match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL, TokenType::EQUALS_KW})){
            Token op = previous(); if(op.type==TokenType::EQUALS_KW){ Token norm = op; norm.type = TokenType::EQUAL_EQUAL; op = norm; }
            auto right = comparison(); expr = std::make_shared<BinaryExpr>(expr, op, right); }
        return expr; }
    ExprPtr comparison(){ auto expr = term(); while(match({TokenType::LESS,TokenType::LESS_EQUAL,TokenType::GREATER,TokenType::GREATER_EQUAL})){
            Token op = previous(); auto right = term(); expr = std::make_shared<BinaryExpr>(expr, op, right); }
        return expr; }
    ExprPtr term(){ auto expr = factor(); while(match({TokenType::PLUS,TokenType::MINUS})){
            Token op = previous(); auto right = factor(); expr = std::make_shared<BinaryExpr>(expr, op, right); }
        return expr; }
    ExprPtr factor(){ auto expr = unary(); while(match({TokenType::STAR,TokenType::SLASH,TokenType::PERCENT})){
            Token op = previous(); auto right = unary(); expr = std::make_shared<BinaryExpr>(expr, op, right); }
        return expr; }
ExprPtr unary(){ if(match({TokenType::BANG, TokenType::MINUS, TokenType::NOT_KW})){
            Token op = previous(); // normalize NOT_KW to BANG semantics
            if(op.type==TokenType::NOT_KW){ Token norm = op; norm.type = TokenType::BANG; op = norm; }
            auto right = unary(); return std::make_shared<UnaryExpr>(op, right); }
        return call(); }

    ExprPtr call(){ auto expr = primary(); while(true){ if(match({TokenType::LEFT_PAREN})){
                std::vector<ExprPtr> args; if(!check(TokenType::RIGHT_PAREN)){ do{ args.push_back(expression()); } while(match({TokenType::COMMA})); }
                consume(TokenType::RIGHT_PAREN, "Expected ')'"); expr = std::make_shared<CallExpr>(expr, args);
            } else if(match({TokenType::DOT})){
                auto name = consume(TokenType::IDENTIFIER, "Expected property name after '.'").lexeme; expr = std::make_shared<GetExpr>(expr, name);
            } else if(match({TokenType::LEFT_BRACKET})){
                auto idx = expression(); consume(TokenType::RIGHT_BRACKET, "Expected ']'"); expr = std::make_shared<IndexExpr>(expr, idx);
            } else break; }
        return expr; }

    ExprPtr primary(){ if(match({TokenType::FALSE})) return std::make_shared<LiteralExpr>(Value(false));
        if(match({TokenType::TRUE})) return std::make_shared<LiteralExpr>(Value(true));
        if(match({TokenType::NULL_T})) return std::make_shared<LiteralExpr>(Value());
        if(match({TokenType::THIS})) return std::make_shared<VarExpr>("this");
        if(match({TokenType::NUMBER})) return std::make_shared<LiteralExpr>(Value(std::stod(previous().lexeme)));
        if(match({TokenType::STRING})) return std::make_shared<LiteralExpr>(Value(previous().lexeme));
        if(match({TokenType::LEFT_PAREN})) { auto e = expression(); consume(TokenType::RIGHT_PAREN, "Expected ')'"); return std::make_shared<GroupingExpr>(e);}        
        if(match({TokenType::LEFT_BRACKET})){
            std::vector<ExprPtr> elems; if(!check(TokenType::RIGHT_BRACKET)){ do{ elems.push_back(expression()); } while(match({TokenType::COMMA})); }
            consume(TokenType::RIGHT_BRACKET, "Expected ']'");
            // Represent list literal as CallExpr on a special marker handled in interpreter
            auto marker = std::make_shared<VarExpr>("__list_literal__"); return std::make_shared<CallExpr>(marker, elems);
        }
        if(match({TokenType::LEFT_BRACE})){
            // dict literal: { "k": v, ... }
            std::vector<ExprPtr> kv; if(!check(TokenType::RIGHT_BRACE)){
                do{ auto keyTok = consume(TokenType::STRING, "Expected string key in dict literal"); consume(TokenType::COLON, "Expected ':'"); kv.push_back(std::make_shared<LiteralExpr>(Value(keyTok.lexeme))); kv.push_back(expression()); } while(match({TokenType::COMMA}));
            }
            consume(TokenType::RIGHT_BRACE, "Expected '}'"); auto marker = std::make_shared<VarExpr>("__dict_literal__"); return std::make_shared<CallExpr>(marker, kv);
        }
        if(match({TokenType::IDENTIFIER})) return std::make_shared<VarExpr>(previous().lexeme);
        throw RuntimeError("Expected expression"); }
};

// Environments
struct Environment {
    std::shared_ptr<Environment> parent;
    std::unordered_map<std::string, Value> values;
    explicit Environment(std::shared_ptr<Environment> p=nullptr): parent(std::move(p)) {}
    void define(const std::string& name, Value v){ values[name]=std::move(v); }
    bool assign(const std::string& name, Value v){ if(values.count(name)){ values[name]=std::move(v); return true; } if(parent) return parent->assign(name, std::move(v)); return false; }
    Value get(const std::string& name){ if(values.count(name)) return values[name]; if(parent) return parent->get(name); throw RuntimeError("Undefined variable: "+name); }
    Value* getPtr(const std::string& name){ auto it=values.find(name); if(it!=values.end()) return &it->second; if(parent) return parent->getPtr(name); return nullptr; }
};

// Callable types
struct Callable { virtual ~Callable()=default; virtual int arity() const =0; virtual Value call(Interpreter&, const std::vector<Value>&)=0; };

struct Function : Callable { std::vector<std::string> params; std::shared_ptr<BlockStmt> body; std::shared_ptr<Environment> closure; bool isInit=false; std::string name;
    Function(std::string n,std::vector<std::string> p,std::shared_ptr<BlockStmt> b,std::shared_ptr<Environment> c,bool init=false): params(std::move(p)), body(std::move(b)), closure(std::move(c)), isInit(init), name(std::move(n)) {}
    int arity() const override { return (int)params.size(); }
    Value call(Interpreter&, const std::vector<Value>&) override; };

struct NativeFunction : Callable { std::string name; int fixedArity; std::function<Value(Interpreter&, const std::vector<Value>&)> fn; NativeFunction(std::string n,int a,std::function<Value(Interpreter&,const std::vector<Value>&)> f): name(std::move(n)), fixedArity(a), fn(std::move(f)){} int arity() const override { return fixedArity; } Value call(Interpreter& ip, const std::vector<Value>& args) override { return fn(ip,args);} };

struct Class : Callable, std::enable_shared_from_this<Class> { std::string name; std::unordered_map<std::string, std::shared_ptr<Function>> methods; int ar= -1; Class(std::string n, std::unordered_map<std::string, std::shared_ptr<Function>> m): name(std::move(n)), methods(std::move(m)){} int arity() const override { return ar; } Value call(Interpreter&, const std::vector<Value>&) override; std::shared_ptr<Function> findMethod(const std::string& n){ auto it=methods.find(n); if(it!=methods.end()) return it->second; return nullptr; } };

struct Instance { std::shared_ptr<Class> klass; std::unordered_map<std::string, Value> fields; explicit Instance(std::shared_ptr<Class> k): klass(std::move(k)){} };

// Interpreter
struct ReturnSignal { Value value; };

class Interpreter {
public:
    std::shared_ptr<Environment> globals = std::make_shared<Environment>();
    std::shared_ptr<Environment> env = globals;
    std::filesystem::path current_dir;
    std::filesystem::path builtins_dir; // optional root for builtins
    std::unordered_set<std::string> loaded_files;

    explicit Interpreter(const std::filesystem::path& entry_dir);
    void interpret(const std::vector<StmtPtr>& stmts){ try{ for(auto&s: stmts) execute(s); } catch(const RuntimeError& e){ std::cerr << "Runtime error: " << e.what() << "\n"; }}

    // exec
void execute(const StmtPtr& stmt){ if(auto p=std::dynamic_pointer_cast<BlockStmt>(stmt)) execBlock(p, std::make_shared<Environment>(env));
        else if(auto p=std::dynamic_pointer_cast<LetStmt>(stmt)){ auto v = evaluate(p->initializer); env->define(p->name, v); }
        else if(auto p=std::dynamic_pointer_cast<ExprStmt>(stmt)){ (void)evaluate(p->expr); }
        else if(auto p=std::dynamic_pointer_cast<IfStmt>(stmt)){ if(isTruthy(evaluate(p->cond))) execute(p->thenB); else if(p->elseB) execute(*p->elseB); }
        else if(auto p=std::dynamic_pointer_cast<WhileStmt>(stmt)){ while(isTruthy(evaluate(p->cond))) execute(p->body); }
        else if(auto p=std::dynamic_pointer_cast<ForStmt>(stmt)){ execFor(p); }
        else if(auto p=std::dynamic_pointer_cast<ReturnStmt>(stmt)){ Value v; if(p->value) v = evaluate(*p->value); throw ReturnSignal{v}; }
        else if(auto p=std::dynamic_pointer_cast<FunctionStmt>(stmt)){ auto f = std::make_shared<Function>(p->name, p->params, p->body, env, false); env->define(p->name, Value(f)); }
        else if(auto p=std::dynamic_pointer_cast<ClassStmt>(stmt)){ std::unordered_map<std::string, std::shared_ptr<Function>> methods; for(auto& kv: p->methods){ auto f = std::make_shared<Function>(kv.first, kv.second->params, kv.second->body, env, kv.first=="init"); methods[kv.first]=f; } auto k = std::make_shared<Class>(p->name, methods); env->define(p->name, Value(k)); }
        else if(auto p=std::dynamic_pointer_cast<StructStmt>(stmt)){ // store struct metadata as a Class without methods; instances created via Class call
            auto k = std::make_shared<Class>(p->name, std::unordered_map<std::string, std::shared_ptr<Function>>{}); env->define(p->name, Value(k)); }
        else if(auto p=std::dynamic_pointer_cast<UnionStmt>(stmt)){ auto k = std::make_shared<Class>(p->name, std::unordered_map<std::string, std::shared_ptr<Function>>{}); env->define(p->name, Value(k)); }
        else if(auto p=std::dynamic_pointer_cast<ImportStmt>(stmt)){ execImport(p->path); }
        else if(auto q = std::dynamic_pointer_cast<MultiAssignStmt>(stmt)){
            Value rv = evaluate(q->value);
            auto lst = std::get_if<List>(&rv.data);
            if(!lst) throw RuntimeError("Right-hand side of multi-assign must be a list");
            if(lst->size() != q->names.size()) throw RuntimeError("Multi-assign length mismatch");
            for(size_t i=0;i<q->names.size();++i){
                const auto& name = q->names[i];
                if(env->values.count(name)) env->assign(name, (*lst)[i]); else env->define(name, (*lst)[i]);
            }
        }
        else if(auto ml = std::dynamic_pointer_cast<MultiLetStmt>(stmt)){
            for(const auto& name : ml->names){ env->define(name, Value()); }
        }
        else { throw RuntimeError("Unknown statement type"); } }

    void execBlock(const std::shared_ptr<BlockStmt>& block, std::shared_ptr<Environment> newEnv){ auto prev = env; env = newEnv; try{ for(auto&s: block->stmts) execute(s); } catch(...) { env = prev; throw; } env = prev; }

    void execFor(const std::shared_ptr<ForStmt>& fs){ Value it = evaluate(fs->iterable); auto setVar = [&](const Value& v){ if(env->values.count(fs->var)) env->assign(fs->var, v); else env->define(fs->var, v); };
        if(auto l = std::get_if<List>(&it.data)){ for(const auto& v : *l){ setVar(v); execute(fs->body); } return; }
        if(auto d = std::get_if<Dict>(&it.data)){ for(const auto& kv : *d){ setVar(Value(kv.first)); execute(fs->body); } return; }
        if(auto s = std::get_if<std::string>(&it.data)){ for(char ch: *s){ std::string one(1, ch); setVar(Value(one)); execute(fs->body);} return; }
        throw RuntimeError("for 'in' expects list, dict, or string"); }

void execImport(const std::string& rawPath){ using namespace std::filesystem; path p(rawPath);
        if(p.extension().empty()) p.replace_extension(".ad");
        path full;
        // Special handling for builtins location, e.g. import "builtins/..."
        std::string raw = rawPath;
        if(!builtins_dir.empty() && (raw.rfind("builtins/", 0)==0 || raw.rfind("builtins\\", 0)==0)){
            path sub = raw.substr(raw.find('/')!=std::string::npos? raw.find('/')+1 : raw.find('\\')+1);
            full = (builtins_dir / sub).lexically_normal();
        } else {
            full = (current_dir / p).lexically_normal();
        }
        std::string key = full.string(); if(loaded_files.count(key)) return; loaded_files.insert(key);
        std::ifstream in(full, std::ios::binary); if(!in) throw RuntimeError(std::string("import: cannot open ")+ key);
        std::ostringstream ss; ss<<in.rdbuf(); std::string src = ss.str(); Lexer lx(src); auto toks = lx.scan(); Parser ps(toks); auto stmts = ps.parse(); auto prevDir = current_dir; current_dir = full.parent_path(); try{ for(auto& s: stmts) execute(s); } catch(...) { current_dir = prevDir; throw; } current_dir = prevDir; }

    Value evaluate(const ExprPtr& expr){
        if(auto p=std::dynamic_pointer_cast<LiteralExpr>(expr)) return p->value;
        if(auto p=std::dynamic_pointer_cast<VarExpr>(expr)) { if(auto ptr = env->getPtr(p->name)) return *ptr; return Value(); }
        if(auto p=std::dynamic_pointer_cast<AssignExpr>(expr)){ auto v = evaluate(p->value); if(!env->assign(p->name, v)) throw RuntimeError("Undefined variable: "+p->name); return v; }
        if(auto p=std::dynamic_pointer_cast<GroupingExpr>(expr)) return evaluate(p->expr);
        if(auto p=std::dynamic_pointer_cast<UnaryExpr>(expr)) return evalUnary(p->op, evaluate(p->right));
        if(auto p=std::dynamic_pointer_cast<BinaryExpr>(expr)){
            // Implement short-circuit for logical AND/OR
            if(p->op.type==TokenType::AND_AND || p->op.type==TokenType::AND_KW){
                Token op = p->op; if(op.type==TokenType::AND_KW){ Token norm=op; norm.type=TokenType::AND_AND; op=norm; }
                Value left = evaluate(p->left);
                if(!isTruthy(left)) return Value(false);
                Value right = evaluate(p->right);
                return evalBinary(left, op, right);
            }
            if(p->op.type==TokenType::OR_OR || p->op.type==TokenType::OR_KW){
                Token op = p->op; if(op.type==TokenType::OR_KW){ Token norm=op; norm.type=TokenType::OR_OR; op=norm; }
                Value left = evaluate(p->left);
                if(isTruthy(left)) return Value(true);
                Value right = evaluate(p->right);
                return evalBinary(left, op, right);
            }
            return evalBinary(evaluate(p->left), p->op, evaluate(p->right));
        }
        if(auto p=std::dynamic_pointer_cast<CallExpr>(expr)) return evalCall(p);
        if(auto p=std::dynamic_pointer_cast<GetExpr>(expr)) return evalGet(p);
        if(auto p=std::dynamic_pointer_cast<SetExpr>(expr)) return evalSet(p);
        if(auto p=std::dynamic_pointer_cast<IndexExpr>(expr)) return evalIndex(p);
        if(auto p=std::dynamic_pointer_cast<SetIndexExpr>(expr)) return evalSetIndex(p);
        throw RuntimeError("Unknown expression");
    }

    static bool isTruthy(const Value& v){ if(std::holds_alternative<std::monostate>(v.data)) return false; if(auto b=std::get_if<bool>(&v.data)) return *b; if(auto n=std::get_if<double>(&v.data)) return *n!=0; return true; }

    Value evalUnary(const Token& op, const Value& r){ switch(op.type){ case TokenType::BANG: return Value(!isTruthy(r)); case TokenType::MINUS: {
                if(auto n=std::get_if<double>(&r.data)) return Value(-*n); throw RuntimeError("Unary '-' on non-number"); }
            default: throw RuntimeError("Invalid unary op"); }}

    Value evalBinary(const Value& l, const Token& op, const Value& r){ auto num = [&](const Value& v)->double{ if(auto n=std::get_if<double>(&v.data)) return *n; throw RuntimeError("Expected number"); };
        auto str = [&](const Value& v)->std::string{ if(auto s=std::get_if<std::string>(&v.data)) return *s; throw RuntimeError("Expected string"); };
        switch(op.type){
            case TokenType::PLUS: {
                if(std::holds_alternative<double>(l.data) && std::holds_alternative<double>(r.data)) return Value(num(l)+num(r));
                if(std::holds_alternative<std::string>(l.data) || std::holds_alternative<std::string>(r.data)) {
                    std::ostringstream oss; if(auto ls=std::get_if<std::string>(&l.data)) oss<<*ls; else if(auto ln=std::get_if<double>(&l.data)) oss<<*ln; else oss<<"[obj]";
                    if(auto rs=std::get_if<std::string>(&r.data)) oss<<*rs; else if(auto rn=std::get_if<double>(&r.data)) oss<<*rn; else oss<<"[obj]";
                    return Value(oss.str());
                }
                throw RuntimeError("'+' needs numbers or strings"); }
            case TokenType::MINUS: return Value(num(l)-num(r));
            case TokenType::STAR: return Value(num(l)*num(r));
            case TokenType::SLASH: { double d=num(r); if(d==0) throw RuntimeError("Division by zero"); return Value(num(l)/d);}            
            case TokenType::PERCENT: { double d=num(r); if(d==0) throw RuntimeError("Modulo by zero"); return Value(fmod(num(l), d)); }
            case TokenType::EQUAL_EQUAL: return Value(equal(l,r));
            case TokenType::BANG_EQUAL: return Value(!equal(l,r));
            case TokenType::LESS: return Value(num(l)<num(r));
            case TokenType::LESS_EQUAL: return Value(num(l)<=num(r));
            case TokenType::GREATER: return Value(num(l)>num(r));
            case TokenType::GREATER_EQUAL: return Value(num(l)>=num(r));
            case TokenType::AND_AND: return Value(isTruthy(l) && isTruthy(r));
            case TokenType::OR_OR: return Value(isTruthy(l) || isTruthy(r));
            default: throw RuntimeError("Invalid binary op");
        }
    }

    static bool equal(const Value& a, const Value& b){ if(a.data.index()!=b.data.index()) return false; if(auto pa=std::get_if<std::monostate>(&a.data)) return true; if(auto pb=std::get_if<bool>(&a.data)) return *pb==std::get<bool>(b.data); if(auto pn=std::get_if<double>(&a.data)) return *pn==std::get<double>(b.data); if(auto ps=std::get_if<std::string>(&a.data)) return *ps==std::get<std::string>(b.data); return &a==&b; }

    Value evalCall(const std::shared_ptr<CallExpr>& c){
        // list and dict literal markers
        if(auto v=std::dynamic_pointer_cast<VarExpr>(c->callee)){
            if(v->name=="__list_literal__"){ List lst; for(auto &e: c->args) lst.push_back(evaluate(e)); return Value(lst);}            
            if(v->name=="__dict_literal__"){ Dict d; for(size_t i=0;i<c->args.size();i+=2){ auto k = evaluate(c->args[i]); auto val = evaluate(c->args[i+1]); d[std::get<std::string>(k.data)] = val; } return Value(d);}        }
        Value cal = evaluate(c->callee);
        if(auto nf = std::get_if<std::shared_ptr<NativeFunction>>(&cal.data)){
            std::vector<Value> evaluated; evaluated.reserve(c->args.size()); for(auto &a: c->args) evaluated.push_back(evaluate(a)); return (*nf)->call(*this, evaluated);
        }
        if(auto uf = std::get_if<std::shared_ptr<Function>>(&cal.data)){
            std::vector<Value> evaluated; evaluated.reserve(c->args.size()); for(auto &a: c->args) evaluated.push_back(evaluate(a)); return (*uf)->call(*this, evaluated);
        }
        if(auto kc = std::get_if<std::shared_ptr<Class>>(&cal.data)){
            std::vector<Value> evaluated; evaluated.reserve(c->args.size()); for(auto &a: c->args) evaluated.push_back(evaluate(a)); return (*kc)->call(*this, evaluated);
        }
        throw RuntimeError("Can only call functions/classes");
    }

    Value evalGet(const std::shared_ptr<GetExpr>& g){ auto obj = evaluate(g->object); if(auto inst = std::get_if<std::shared_ptr<Instance>>(&obj.data)){
            auto it = (*inst)->fields.find(g->name); if(it!=(*inst)->fields.end()) return it->second; if(auto m = (*inst)->klass->findMethod(g->name)){
                // bind this
                auto bound = std::make_shared<Function>(m->name, m->params, m->body, std::make_shared<Environment>(m->closure), m->isInit);
                bound->closure->define("this", obj); return Value(bound);
            }
            throw RuntimeError("Undefined property: "+g->name);
        }
        if(auto d = std::get_if<Dict>(&obj.data)){
            auto it = d->find(g->name); if(it!=d->end()) return it->second; throw RuntimeError("Dict has no key: "+g->name);
        }
        if(auto s = std::get_if<std::string>(&obj.data)){
            // Provide string methods: split
            if(g->name == "split"){
                std::string base = *s;
                auto fn = std::make_shared<NativeFunction>("string.split", -1, [base](Interpreter&, const std::vector<Value>& args)->Value{
                    // emulate split(base, sep?)
                    std::string sep;
                    if(args.size()==1){ if(std::holds_alternative<std::string>(args[0].data)) sep = std::get<std::string>(args[0].data); else throw RuntimeError("split sep must be string"); }
                    else if(args.size()>1) throw RuntimeError("split expects at most 1 arg");
                    // Reuse split logic
                    if(sep.empty()){
                        std::istringstream iss(base); std::string tok; List out; while(iss>>tok){ out.push_back(Value(tok)); } return Value(out);
                    } else {
                        List out; size_t pos=0; while(true){ size_t n=base.find(sep, pos); if(n==std::string::npos){ out.push_back(Value(base.substr(pos))); break; } out.push_back(Value(base.substr(pos, n-pos))); pos = n + sep.size(); } return Value(out);
                    }
                });
                return Value(fn);
            }
            throw RuntimeError("String has no property: "+g->name);
        }
        throw RuntimeError("Only instances, dicts, or strings have properties");
    }

    Value evalSet(const std::shared_ptr<SetExpr>& s){ auto obj = evaluate(s->object); Value v = evaluate(s->value); if(auto inst = std::get_if<std::shared_ptr<Instance>>(&obj.data)){
            (*inst)->fields[s->name]=v; return v; }
        if(auto d = std::get_if<Dict>(&obj.data)){
            (*d)[s->name]=v; return v; }
        throw RuntimeError("Only instances or dicts support set");
    }

    Value evalIndex(const std::shared_ptr<IndexExpr>& ix){ auto obj = evaluate(ix->object); auto idx = evaluate(ix->index); if(auto lst = std::get_if<List>(&obj.data)){
            int i = (int)std::get<double>(idx.data); if(i<0 || i>=(int)lst->size()) throw RuntimeError("List index out of range"); return (*lst)[i]; }
        if(auto d = std::get_if<Dict>(&obj.data)){
            auto key = std::get<std::string>(idx.data); auto it=d->find(key); if(it==d->end()) throw RuntimeError("Key not found"); return it->second; }
        throw RuntimeError("Indexing supported on list/dict"); }

Value evalSetIndex(const std::shared_ptr<SetIndexExpr>& sx){ auto idxv = evaluate(sx->index); auto val = evaluate(sx->value);
        // Handle instance field or dict property: this.field[i] = v
        if(auto ge = std::dynamic_pointer_cast<GetExpr>(sx->object)){
            Value base = evaluate(ge->object);
            if(auto inst = std::get_if<std::shared_ptr<Instance>>(&base.data)){
                Value &slot = (*inst)->fields[ge->name];
                if(auto lst = std::get_if<List>(&slot.data)){
                    int i = (int)std::get<double>(idxv.data); if(i<0) throw RuntimeError("List index out of range"); if(i==(int)lst->size()) { lst->push_back(val); return val; } if(i>=(int)lst->size()) throw RuntimeError("List index out of range"); (*lst)[i]=val; return val;
                }
                if(auto d = std::get_if<Dict>(&slot.data)){
                    auto key = std::get<std::string>(idxv.data); (*d)[key]=val; return val;
                }
                throw RuntimeError("Index assignment on non-indexable field");
            }
            if(auto d = std::get_if<Dict>(&base.data)){
                Value &slot = (*d)[ge->name];
                if(auto lst = std::get_if<List>(&slot.data)){
                    int i = (int)std::get<double>(idxv.data); if(i<0) throw RuntimeError("List index out of range"); if(i==(int)lst->size()) { lst->push_back(val); return val; } if(i>=(int)lst->size()) throw RuntimeError("List index out of range"); (*lst)[i]=val; return val;
                }
                if(auto d2 = std::get_if<Dict>(&slot.data)){
                    auto key = std::get<std::string>(idxv.data); (*d2)[key]=val; return val;
                }
                throw RuntimeError("Index assignment on non-indexable dict property");
            }
        }
        // Handle variable list: xs[i] = v
        if(auto ve = std::dynamic_pointer_cast<VarExpr>(sx->object)){
            Value* slot = env->getPtr(ve->name);
            if(!slot) throw RuntimeError("Undefined variable: "+ve->name);
            if(auto lst = std::get_if<List>(&slot->data)){
                int i = (int)std::get<double>(idxv.data); if(i<0) throw RuntimeError("List index out of range"); if(i==(int)lst->size()) { lst->push_back(val); return val; } if(i>=(int)lst->size()) throw RuntimeError("List index out of range"); (*lst)[i]=val; return val;
            }
            if(auto d = std::get_if<Dict>(&slot->data)){
                auto key = std::get<std::string>(idxv.data); (*d)[key]=val; return val;
            }
            throw RuntimeError("Index assignment on non-indexable variable");
        }
        // Fallback: evaluate object value and attempt to modify; may not persist if temporary
        Value obj = evaluate(sx->object);
        if(auto lst = std::get_if<List>(&obj.data)){
            int i = (int)std::get<double>(idxv.data); if(i<0) throw RuntimeError("List index out of range"); if(i==(int)lst->size()) { lst->push_back(val); return val; } if(i>=(int)lst->size()) throw RuntimeError("List index out of range"); (*lst)[i] = val; return val; }
        if(auto d = std::get_if<Dict>(&obj.data)){
            auto key = std::get<std::string>(idxv.data); (*d)[key] = val; return val; }
        throw RuntimeError("Index assignment supported on list/dict"); }
};

// Function call impl
Value Function::call(Interpreter& ip, const std::vector<Value>& args){ if((int)args.size()!=arity()) throw RuntimeError("Arity mismatch"); auto local = std::make_shared<Environment>(closure); for(size_t i=0;i<params.size();++i) local->define(params[i], args[i]);
    // if method with 'this' in closure, keep it
    try{ ip.execBlock(body, local); if(isInit) return local->get("this"); return Value(); } catch(const ReturnSignal& r){ if(isInit) return local->get("this"); return r.value; } }

// Class call creates instance and invokes init if exists
Value Class::call(Interpreter& ip, const std::vector<Value>& args){ auto inst = std::make_shared<Instance>(std::static_pointer_cast<Class>(shared_from_this())); auto init = findMethod("init"); if(init){ auto bound = std::make_shared<Function>(init->name, init->params, init->body, std::make_shared<Environment>(init->closure), true); bound->closure->define("this", Value(inst)); if((int)args.size()!=bound->arity()) throw RuntimeError("Arity mismatch in init"); (void)bound->call(ip, args); }
    return Value(inst); }

// Builtins
static Value builtin_print(Interpreter&, const std::vector<Value>& args){ std::ostringstream oss; for(size_t i=0;i<args.size();++i){ const Value& v=args[i]; if(i) oss<<" "; if(auto n=std::get_if<double>(&v.data)) oss<<*n; else if(auto s=std::get_if<std::string>(&v.data)) oss<<*s; else if(auto b=std::get_if<bool>(&v.data)) oss<<(*b?"true":"false"); else if(std::holds_alternative<std::monostate>(v.data)) oss<<"null"; else if(auto l=std::get_if<List>(&v.data)){ oss<<"["; for(size_t j=0;j<l->size();++j){ if(j) oss<<", "; const Value& e=(*l)[j]; if(auto en=std::get_if<double>(&e.data)) oss<<*en; else if(auto es=std::get_if<std::string>(&e.data)) oss<<'"'<<*es<<'"'; else oss<<"...";} oss<<"]"; } else if(auto d=std::get_if<Dict>(&v.data)){ oss<<"{"; size_t j=0; for(auto& kv:*d){ if(j++) oss<<", "; oss<<kv.first<<": "; const Value& e=kv.second; if(auto en=std::get_if<double>(&e.data)) oss<<*en; else if(auto es=std::get_if<std::string>(&e.data)) oss<<'"'<<*es<<'"'; else oss<<"...";} oss<<"}"; } else { oss<<"<"<<v.typeName()<<">";} }
    std::cout<<oss.str()<<"\n"; std::cout.flush(); return Value(); }

static Value builtin_len(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("len expects 1 arg"); if(auto l=std::get_if<List>(&args[0].data)) return Value((double)l->size()); if(auto s=std::get_if<std::string>(&args[0].data)) return Value((double)s->size()); if(auto d=std::get_if<Dict>(&args[0].data)) return Value((double)d->size()); throw RuntimeError("len on unsupported type"); }

static Value builtin_input(Interpreter&, const std::vector<Value>& args){ if(args.size()>1) throw RuntimeError("input expects 0 or 1 arg"); if(args.size()==1){ if(auto s=std::get_if<std::string>(&args[0].data)) { std::cout<<*s; std::cout.flush(); } else throw RuntimeError("input prompt must be string"); }
    else { std::cout.flush(); }
    std::string line; std::getline(std::cin, line); return Value(line); }

static Value builtin_map(Interpreter& ip, const std::vector<Value>& args){ if(args.size()!=2) throw RuntimeError("map expects (func, list)"); auto fptr = std::get_if<std::shared_ptr<Function>>(&args[0].data); auto nptr = std::get_if<std::shared_ptr<NativeFunction>>(&args[0].data); auto lptr = std::get_if<List>(&args[1].data); if(!lptr) throw RuntimeError("map arg2 must be list"); List out; out.reserve(lptr->size()); for(const auto& v: *lptr){ if(fptr){ out.push_back((*fptr)->call(ip, {v})); } else if(nptr){ out.push_back((*nptr)->call(ip, {v})); } else throw RuntimeError("map arg1 must be callable"); } return Value(out); }

static Value builtin_sqrt_bs(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("sqrt_bs expects 1 arg"); double x; if(auto n=std::get_if<double>(&args[0].data)) x=*n; else throw RuntimeError("sqrt_bs needs number"); if(x<0) throw RuntimeError("sqrt_bs domain error"); if(x==0) return Value(0.0); double lo=0, hi=std::max(1.0, x), mid; for(int i=0;i<100;i++){ mid=(lo+hi)/2; if(mid*mid>=x) hi=mid; else lo=mid; } return Value((lo+hi)/2); }

static Value builtin_range(Interpreter&, const std::vector<Value>& args){ auto asInt=[&](const Value& v)->long long{ if(auto n=std::get_if<double>(&v.data)) return (long long)*n; throw RuntimeError("range expects numbers");}; long long start=0, stop=0, step=1; if(args.size()==1){ stop=asInt(args[0]); } else if(args.size()==2){ start=asInt(args[0]); stop=asInt(args[1]); } else if(args.size()==3){ start=asInt(args[0]); stop=asInt(args[1]); step=asInt(args[2]); if(step==0) throw RuntimeError("range step cannot be 0"); } else throw RuntimeError("range expects 1..3 args"); List out; if(step>0){ for(long long i=start;i<stop;i+=step) out.push_back(Value((double)i)); } else { for(long long i=start;i>stop;i+=step) out.push_back(Value((double)i)); } return Value(out); }

// Additional builtins for casting and string/list operations
static Value builtin_int(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("int expects 1 arg"); if(auto n=std::get_if<double>(&args[0].data)) return Value((double)(long long)(*n)); if(auto s=std::get_if<std::string>(&args[0].data)) return Value((double)std::stoll(*s)); if(auto b=std::get_if<bool>(&args[0].data)) return Value(*b?1.0:0.0); throw RuntimeError("int() unsupported type"); }
static Value builtin_float(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("float expects 1 arg"); if(auto n=std::get_if<double>(&args[0].data)) return Value(*n); if(auto s=std::get_if<std::string>(&args[0].data)) return Value(std::stod(*s)); if(auto b=std::get_if<bool>(&args[0].data)) return Value(*b?1.0:0.0); throw RuntimeError("float() unsupported type"); }
static Value builtin_str(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("str expects 1 arg"); const Value& v=args[0]; std::ostringstream oss; if(auto n=std::get_if<double>(&v.data)) oss<<*n; else if(auto s=std::get_if<std::string>(&v.data)) oss<<*s; else if(auto b=std::get_if<bool>(&v.data)) oss<<(*b?"true":"false"); else if(std::holds_alternative<std::monostate>(v.data)) oss<<"null"; else oss<<"<"<<v.typeName()<<">"; return Value(oss.str()); }
static Value builtin_split(Interpreter&, const std::vector<Value>& args){ if(args.size()<1||args.size()>2) throw RuntimeError("split expects (string[, sep])"); if(!std::holds_alternative<std::string>(args[0].data)) throw RuntimeError("split first arg must be string"); std::string s=std::get<std::string>(args[0].data); std::string sep = (args.size()==2)? std::get<std::string>(args[1].data) : std::string(); List out; if(sep.empty()){ std::istringstream iss(s); std::string part; while(iss>>part) out.push_back(Value(part)); } else { size_t pos=0; while(true){ size_t n=s.find(sep, pos); if(n==std::string::npos){ out.push_back(Value(s.substr(pos))); break; } out.push_back(Value(s.substr(pos, n-pos))); pos = n+sep.size(); } } return Value(out); }
static Value builtin_join(Interpreter&, const std::vector<Value>& args){ if(args.size()!=2) throw RuntimeError("join expects (list, sep)"); auto lst = std::get_if<List>(&args[0].data); if(!lst) throw RuntimeError("join first arg must be list of strings"); std::string sep = std::get<std::string>(args[1].data); std::ostringstream oss; for(size_t i=0;i<lst->size();++i){ if(i) oss<<sep; oss<<std::get<std::string>((*lst)[i].data); } return Value(oss.str()); }
static Value builtin_has(Interpreter&, const std::vector<Value>& args){ if(args.size()!=2) throw RuntimeError("has expects (dict, key)"); auto d = std::get_if<Dict>(&args[0].data); if(!d) throw RuntimeError("has first arg must be dict"); auto key = std::get<std::string>(args[1].data); return Value((bool)(d->find(key)!=d->end())); }
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

// HTTP helpers using WinHTTP on Windows or libcurl elsewhere; supports http and https
static Dict http_request(const std::string& method, const std::string& url, const std::string& body, const std::unordered_map<std::string, std::string>& extra_headers){
    // file:// short-circuit on all platforms
    const std::string filePrefix = "file://";
    if(url.rfind(filePrefix, 0) == 0){
        std::string path = url.substr(filePrefix.size());
        std::ifstream in(path, std::ios::binary);
        if(!in) throw RuntimeError("requests."+method+": cannot open file");
        std::ostringstream ss; ss << in.rdbuf();
        Dict resp; resp["status"] = Value((double)200); resp["text"] = Value(ss.str()); return resp;
    }
#ifdef _WIN32
    // Windows: WinHTTP
    URL_COMPONENTS uc{}; ZeroMemory(&uc, sizeof(uc));
    wchar_t host[256]; wchar_t path[2048];
    wchar_t scheme[16];
    uc.dwStructSize = sizeof(uc);
    uc.lpszHostName = host; uc.dwHostNameLength = _countof(host);
    uc.lpszUrlPath = path; uc.dwUrlPathLength = _countof(path);
    uc.lpszScheme = scheme; uc.dwSchemeLength = _countof(scheme);
    if(!WinHttpCrackUrl(std::wstring(url.begin(), url.end()).c_str(), 0, 0, &uc)){
        throw RuntimeError("requests."+method+": invalid URL");
    }
    bool secure = (uc.nScheme == INTERNET_SCHEME_HTTPS);

    HINTERNET hSession = WinHttpOpen(L"AdaScript/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if(!hSession) throw RuntimeError("requests."+method+": WinHttpOpen failed");
    HINTERNET hConnect = WinHttpConnect(hSession, uc.lpszHostName, uc.nPort? uc.nPort : (secure? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT), 0);
    if(!hConnect){ WinHttpCloseHandle(hSession); throw RuntimeError("requests."+method+": WinHttpConnect failed"); }

    DWORD flags = secure? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, std::wstring(method.begin(), method.end()).c_str(), uc.lpszUrlPath, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if(!hRequest){ WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); throw RuntimeError("requests."+method+": WinHttpOpenRequest failed"); }

    // Build headers
    std::ostringstream hdrs;
    for(const auto& kv : extra_headers){ hdrs << kv.first << ": " << kv.second << "\r\n"; }
    std::string hdrStr = hdrs.str();
    std::wstring whdr(hdrStr.begin(), hdrStr.end());

    const void* bodyData = body.empty()? nullptr : body.data();
    DWORD bodySize = (DWORD)body.size();

    BOOL sent = WinHttpSendRequest(hRequest,
        whdr.empty()? WINHTTP_NO_ADDITIONAL_HEADERS : whdr.c_str(), (DWORD)-1L,
        (LPVOID)bodyData, bodySize, bodySize, 0);
    if(!sent){ WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); throw RuntimeError("requests."+method+": WinHttpSendRequest failed"); }
    if(!WinHttpReceiveResponse(hRequest, nullptr)){
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        throw RuntimeError("requests."+method+": WinHttpReceiveResponse failed");
    }

    // Status code
    DWORD statusCode = 0; DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);

    // Read body
    std::string out;
    for(;;){
        DWORD dwSize = 0;
        if(!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if(dwSize == 0) break;
        std::string chunk; chunk.resize(dwSize);
        DWORD dwRead = 0;
        if(!WinHttpReadData(hRequest, chunk.data(), dwSize, &dwRead)) break;
        chunk.resize(dwRead);
        out.append(chunk);
    }

    // Few headers
    Dict headers;
    wchar_t ctype[256]; DWORD ctypeLen = sizeof(ctype);
    if(WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX, &ctype, &ctypeLen, WINHTTP_NO_HEADER_INDEX)){
        std::wstring ws(ctype, (ctypeLen/sizeof(wchar_t))-1);
        headers["Content-Type"] = Value(std::string(ws.begin(), ws.end()));
    }
    wchar_t server[256]; DWORD serverLen = sizeof(server);
    if(WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_SERVER, WINHTTP_HEADER_NAME_BY_INDEX, &server, &serverLen, WINHTTP_NO_HEADER_INDEX)){
        std::wstring ws(server, (serverLen/sizeof(wchar_t))-1);
        headers["Server"] = Value(std::string(ws.begin(), ws.end()));
    }

    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);

    Dict resp; resp["status"] = Value((double)statusCode); resp["text"] = Value(out); resp["headers"] = Value(headers);
    return resp;
#else
    // Non-Windows: libcurl (optional)
    #ifndef ADASCRIPT_NO_CURL
      struct Buf { std::string s; };
      auto write_cb = [](char* ptr, size_t size, size_t nmemb, void* userdata)->size_t{
          Buf* b = (Buf*)userdata; b->s.append(ptr, size*nmemb); return size*nmemb;
      };
      CURL* curl = curl_easy_init();
      if(!curl) throw RuntimeError("requests."+method+": curl init failed");
      Buf buf; long code = 0;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
      if(!body.empty()){
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
          curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
      }
      struct curl_slist* hdrs = nullptr;
      for(const auto& kv : extra_headers){ std::string h = kv.first + ": " + kv.second; hdrs = curl_slist_append(hdrs, h.c_str()); }
      if(hdrs) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
      CURLcode rc = curl_easy_perform(curl);
      if(rc != CURLE_OK){ if(hdrs) curl_slist_free_all(hdrs); curl_easy_cleanup(curl); throw RuntimeError("requests."+method+": curl perform failed"); }
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
      if(hdrs) curl_slist_free_all(hdrs);
      curl_easy_cleanup(curl);
      Dict resp; resp["status"] = Value((double)code); resp["text"] = Value(buf.s); return resp;
    #else
      throw RuntimeError("HTTP disabled: libcurl not available in this build");
    #endif
#endif
}

// requests.get(url)
static Value builtin_requests_get(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("requests.get expects (url)"); std::string url = std::get<std::string>(args[0].data); auto resp = http_request("GET", url, std::string(), {}); return Value(resp); }
// requests.post(url, data, headers?)
static Value builtin_requests_post(Interpreter&, const std::vector<Value>& args){ if(args.size()<1||args.size()>3) throw RuntimeError("requests.post expects (url[, data[, headers]])"); std::string url = std::get<std::string>(args[0].data); std::string body; std::unordered_map<std::string,std::string> hdrs; if(args.size()>=2){ if(auto s=std::get_if<std::string>(&args[1].data)) body=*s; else throw RuntimeError("requests.post data must be string"); } if(args.size()==3){ auto d = std::get_if<Dict>(&args[2].data); if(!d) throw RuntimeError("requests.post headers must be dict"); for(const auto& kv : *d){ if(std::holds_alternative<std::string>(kv.second.data)) hdrs[kv.first] = std::get<std::string>(kv.second.data); }
    }
auto resp = http_request("POST", url, body, hdrs); return Value(resp); }
// requests.request(method, url[, data[, headers]])
static Value builtin_requests_request(Interpreter&, const std::vector<Value>& args){ if(args.size()<2||args.size()>4) throw RuntimeError("requests.request expects (method, url[, data[, headers]])"); std::string method = std::get<std::string>(args[0].data); std::string url = std::get<std::string>(args[1].data); std::string body; std::unordered_map<std::string,std::string> hdrs; if(args.size()>=3){ if(auto s=std::get_if<std::string>(&args[2].data)) body=*s; else throw RuntimeError("requests.request data must be string"); } if(args.size()==4){ auto d = std::get_if<Dict>(&args[3].data); if(!d) throw RuntimeError("requests.request headers must be dict"); for(const auto& kv : *d){ if(std::holds_alternative<std::string>(kv.second.data)) hdrs[kv.first] = std::get<std::string>(kv.second.data); } }
auto resp = http_request(method, url, body, hdrs); return Value(resp); }

// Parse a line of input into a list: list_input(prompt[, sep[, type]]) where type in {"auto","int","float","str"}
static Value builtin_list_input(Interpreter&, const std::vector<Value>& args){ if(args.size()<1||args.size()>3) throw RuntimeError("list_input expects (prompt[, sep[, type]])"); if(!std::holds_alternative<std::string>(args[0].data)) throw RuntimeError("list_input prompt must be string"); std::string prompt = std::get<std::string>(args[0].data); std::string sep; std::string typ = "auto"; if(args.size()>=2){ if(!std::holds_alternative<std::string>(args[1].data)) throw RuntimeError("list_input sep must be string"); sep = std::get<std::string>(args[1].data);} if(args.size()==3){ if(!std::holds_alternative<std::string>(args[2].data)) throw RuntimeError("list_input type must be string"); typ = std::get<std::string>(args[2].data);} std::cout<<prompt; std::cout.flush(); std::string line; std::getline(std::cin, line); // auto sep if empty
    if(sep.empty()){ if(line.find(',')!=std::string::npos) sep = ","; else sep = ""; }
    List out;
    auto trim = [](std::string s){ size_t i=0; while(i<s.size() && std::isspace((unsigned char)s[i])) i++; size_t j=s.size(); while(j>i && std::isspace((unsigned char)s[j-1])) j--; return s.substr(i,j-i); };
    auto castVal = [&](const std::string& s)->Value{
        if(typ=="str") return Value(s);
        if(typ=="int") { try{ return Value((double)std::stoll(s)); } catch(...) { throw RuntimeError("list_input: invalid int"); } }
        if(typ=="float") { try{ return Value(std::stod(s)); } catch(...) { throw RuntimeError("list_input: invalid float"); } }
        // auto
        try{ size_t pos=0; double v = std::stod(s, &pos); if(pos==s.size()) return Value(v); } catch(...){}
        return Value(s);
    };
    if(sep.empty()){
        std::istringstream iss(line); std::string tok; while(iss>>tok){ tok = trim(tok); out.push_back(castVal(tok)); }
    } else {
        size_t pos=0; while(true){ size_t n=line.find(sep, pos); if(n==std::string::npos){ std::string tok = trim(line.substr(pos)); if(!tok.empty()) out.push_back(castVal(tok)); break; } std::string tok = trim(line.substr(pos, n-pos)); if(!tok.empty()) out.push_back(castVal(tok)); pos = n + sep.size(); }
    }
    return Value(out);
}

// Filesystem builtins
static Value builtin_fs_read_text(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("fs.read_text expects (path)"); std::string p = std::get<std::string>(args[0].data); std::ifstream in(p, std::ios::binary); if(!in) throw RuntimeError("fs.read_text: cannot open file"); std::ostringstream ss; ss<<in.rdbuf(); return Value(ss.str()); }
static Value builtin_fs_write_text(Interpreter&, const std::vector<Value>& args){ if(args.size()!=2) throw RuntimeError("fs.write_text expects (path, text)"); std::string p = std::get<std::string>(args[0].data); std::string t = std::get<std::string>(args[1].data); std::ofstream out(p, std::ios::binary); if(!out) throw RuntimeError("fs.write_text: cannot open file"); out<<t; return Value(true); }
static Value builtin_fs_exists(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("fs.exists expects (path)"); std::string p = std::get<std::string>(args[0].data); return Value((bool)std::filesystem::exists(p)); }
static Value builtin_fs_listdir(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("fs.listdir expects (path)"); std::string p = std::get<std::string>(args[0].data); List out; for(auto& de: std::filesystem::directory_iterator(p)){ out.push_back(Value(de.path().filename().string())); } return Value(out); }
static Value builtin_fs_mkdirs(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("fs.mkdirs expects (path)"); std::string p = std::get<std::string>(args[0].data); std::filesystem::create_directories(p); return Value(true); }
static Value builtin_fs_remove(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("fs.remove expects (path)"); std::string p = std::get<std::string>(args[0].data); uintmax_t n=0; std::error_code ec; if(std::filesystem::is_directory(p, ec)) n = std::filesystem::remove_all(p, ec); else { bool ok = std::filesystem::remove(p, ec); n = ok?1:0; } if(ec) throw RuntimeError("fs.remove failed"); return Value((double)n); }

// Content.get: http(s) via WinHTTP; file:// or local path via filesystem
static Value builtin_content_get(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("content.get expects (source)"); std::string src = std::get<std::string>(args[0].data); Dict resp; resp["source"] = Value(src);
    auto starts_with = [](const std::string& s, const char* p){ return s.rfind(p,0)==0; };
    try{
        if(starts_with(src, "http://") || starts_with(src, "https://") || starts_with(src, "file://")){
Dict r = http_request("GET", src, std::string(), {});
            resp["ok"] = Value(true); resp["status"] = r["status"]; resp["text"] = r["text"]; resp["type"] = Value(starts_with(src, "file://")? std::string("file"): std::string("http"));
            return Value(resp);
        }
        // local path fallback
        if(std::filesystem::exists(src)){
            std::ifstream in(src, std::ios::binary); if(!in) throw RuntimeError("content.get: cannot open file"); std::ostringstream ss; ss<<in.rdbuf(); resp["ok"] = Value(true); resp["status"] = Value((double)200); resp["text"] = Value(ss.str()); resp["type"] = Value(std::string("file")); return Value(resp);
        }
        resp["ok"] = Value(false); resp["status"] = Value((double)404); resp["error"] = Value(std::string("not found")); return Value(resp);
    } catch(const RuntimeError& e){ resp["ok"] = Value(false); resp["status"] = Value((double)500); resp["error"] = Value(std::string(e.what())); return Value(resp); }
}

// C execution: compile and run C code with gcc (MinGW) on Windows
static Value builtin_c_run(Interpreter&, const std::vector<Value>& args){
    if(args.size()<1 || args.size()>2) throw RuntimeError("c.run expects (code[, args_list])");
    if(!std::holds_alternative<std::string>(args[0].data)) throw RuntimeError("c.run code must be string");
    std::string code = std::get<std::string>(args[0].data);
    std::vector<std::string> runArgs;
    if(args.size()==2){
        auto lst = std::get_if<List>(&args[1].data);
        if(!lst) throw RuntimeError("c.run args must be list of strings");
        for(const auto& v: *lst){ if(!std::holds_alternative<std::string>(v.data)) throw RuntimeError("c.run args must be strings"); runArgs.push_back(std::get<std::string>(v.data)); }
    }
    // Write temp file
    auto tmpdir = std::filesystem::temp_directory_path();
#ifdef _WIN32
    std::string base = (tmpdir / std::filesystem::path("adascript_c_" + std::to_string(::GetCurrentProcessId()) + "_" + std::to_string(::GetTickCount64()))).string();
    std::string exefile = base + ".exe";
#else
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    long long ticks = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    std::string base = (tmpdir / std::filesystem::path("adascript_c_" + std::to_string((long long)getpid()) + "_" + std::to_string(ticks))).string();
    std::string exefile = base; // no extension needed on Linux
#endif
    std::string cfile = base + ".c";
    {
        std::ofstream out(cfile, std::ios::binary);
        if(!out) throw RuntimeError("c.run: cannot create temp .c file");
        out<<code;
    }
    // Compile with gcc
    std::ostringstream cc;
    cc<<"gcc \""<<cfile<<"\" -O2 -s -o \""<<exefile<<"\"";
    int rc_compile = std::system(cc.str().c_str());
    Dict result; result["exe"] = Value(exefile); result["compile_status"] = Value((double)rc_compile);
    if(rc_compile!=0){ result["ok"] = Value(false); return Value(result); }
    // Run the exe; allow it to print to console directly
    std::ostringstream runCmd;
    runCmd<<'"'<<exefile<<'"';
    for(const auto& a: runArgs){ runCmd<<" \""<<a<<"\""; }
    int rc_run = std::system(runCmd.str().c_str());
    result["run_status"] = Value((double)rc_run); result["ok"] = Value(true);
    return Value(result);
}

// Server stub
static Value builtin_server_serve(Interpreter&, const std::vector<Value>& args){ throw RuntimeError("server.serve: not implemented in this build"); }

// Native module loader: native.load(path_to_dll_or_so) -> true
#ifdef _WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif
static Value builtin_native_load(Interpreter& ip, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("native.load expects (path)"); if(!std::holds_alternative<std::string>(args[0].data)) throw RuntimeError("native.load path must be string"); std::string path = std::get<std::string>(args[0].data);
    // Bridge: registration function that registers native string fns into ip.globals
    struct Thunk { static Value wrap(AdaScript_NativeStringFn f, void* u, Interpreter& ip, const std::vector<Value>& a){ std::vector<std::string> ss; ss.reserve(a.size()); for(auto& v: a){ std::ostringstream oss; if(auto s=std::get_if<std::string>(&v.data)) oss<<*s; else if(auto n=std::get_if<double>(&v.data)) oss<<*n; else if(auto b=std::get_if<bool>(&v.data)) oss<<(*b?"true":"false"); else if(std::holds_alternative<std::monostate>(v.data)) oss<<"null"; else oss<<"<"<<v.typeName()<<">"; ss.push_back(oss.str()); }
            std::vector<const char*> cargs; cargs.reserve(ss.size()); for(auto& s: ss) cargs.push_back(s.c_str()); char* out = f(u, cargs.data(), (int)cargs.size()); std::string res = out? std::string(out) : std::string(""); if(out) std::free(out); return Value(res); } };
    // Provide a C function pointer that forwards to the lambda above
    struct RegCtx { Interpreter* ip; } ctx{ &ip };
    auto reg_lambda = [&](const char* name, int arity, AdaScript_NativeStringFn fn, void* user){ auto nf = std::make_shared<NativeFunction>(std::string(name), arity, [fn,user](Interpreter& ip2, const std::vector<Value>& a)->Value{ return Thunk::wrap(fn, user, ip2, a); }); ip.globals->define(name, Value(nf)); };
    // Static trampoline to match AdaScript_RegisterFn
    struct RegBridge { static void call(void* vctx, const char* name, int arity, AdaScript_NativeStringFn fn, void* user){ RegCtx* c = (RegCtx*)vctx; Interpreter& ip3 = *c->ip; auto nf = std::make_shared<NativeFunction>(std::string(name), arity, [fn,user](Interpreter& ip2, const std::vector<Value>& a)->Value{ return Thunk::wrap(fn, user, ip2, a); }); ip3.globals->define(name, Value(nf)); } };
#ifdef _WIN32
    HMODULE h = LoadLibraryA(path.c_str()); if(!h) throw RuntimeError("native.load: failed to load library");
    auto init = (AdaScript_ModuleInitFn)GetProcAddress(h, "AdaScript_ModuleInit"); if(!init){ FreeLibrary(h); throw RuntimeError("native.load: AdaScript_ModuleInit not found"); }
#else
    void* h = dlopen(path.c_str(), RTLD_NOW); if(!h) throw RuntimeError(std::string("native.load: ")+ dlerror());
    auto init = (AdaScript_ModuleInitFn)dlsym(h, "AdaScript_ModuleInit"); if(!init){ dlclose(h); throw RuntimeError("native.load: AdaScript_ModuleInit not found"); }
#endif
    // Build a function pointer for registration
    auto c_reg = +[](const char* name, int arity, AdaScript_NativeStringFn fn, void* user){ /* placeholder, replaced by bound with context */ };
    // Instead, we pass a context via host_ctx and use a bridge that captures ctx
    // But since AdaScript_RegisterFn doesn't accept context, we provide a thunk via function-static storage
    // Simpler: provide a small global that stores the active context only during init
    // Use function-static pointer instead of nested static class for portability
    static RegCtx* g_active_regctx = nullptr;
    g_active_regctx = &ctx;
    auto reg_fn = +[](const char* name, int arity, AdaScript_NativeStringFn fn, void* user){ RegCtx* c = g_active_regctx; RegBridge::call(c, name, arity, fn, user); };
    int rc = init((AdaScript_RegisterFn)reg_fn, (void*)&ip); if(rc!=0) throw RuntimeError("native.load: init returned error");
    g_active_regctx = nullptr;
    return Value(true);
}

// Process execution: proc.exec(cmd) -> {status, out}
static Value builtin_proc_exec(Interpreter&, const std::vector<Value>& args){ if(args.size()!=1) throw RuntimeError("proc.exec expects (cmd)"); if(!std::holds_alternative<std::string>(args[0].data)) throw RuntimeError("proc.exec cmd must be string"); std::string cmd = std::get<std::string>(args[0].data);
#ifdef _WIN32
    std::string full = "cmd /C " + cmd + " 2>&1";
    FILE* pipe = _popen(full.c_str(), "rt");
    if(!pipe) throw RuntimeError("proc.exec: failed to start process");
#else
    std::string full = cmd + " 2>&1";
    FILE* pipe = popen(full.c_str(), "r");
    if(!pipe) throw RuntimeError("proc.exec: failed to start process");
#endif
    std::string out; char buf[4096]; while(true){ size_t n = fread(buf, 1, sizeof(buf), pipe); if(n==0) break; out.append(buf, n); }
#ifdef _WIN32
    int rc = _pclose(pipe);
#else
    int rc = pclose(pipe);
#endif
    Dict d; d["status"] = Value((double)rc); d["out"] = Value(out); return Value(d);
}

Interpreter::Interpreter(const std::filesystem::path& entry_dir){ current_dir = entry_dir; globals->define("print", Value(std::make_shared<NativeFunction>("print", -1, builtin_print))); globals->define("len", Value(std::make_shared<NativeFunction>("len", 1, builtin_len))); globals->define("input", Value(std::make_shared<NativeFunction>("input", 0, builtin_input))); globals->define("map", Value(std::make_shared<NativeFunction>("map", 2, builtin_map))); globals->define("sqrt_bs", Value(std::make_shared<NativeFunction>("sqrt_bs", 1, builtin_sqrt_bs))); globals->define("range", Value(std::make_shared<NativeFunction>("range", -1, builtin_range))); globals->define("int", Value(std::make_shared<NativeFunction>("int", 1, builtin_int))); globals->define("float", Value(std::make_shared<NativeFunction>("float", 1, builtin_float))); globals->define("str", Value(std::make_shared<NativeFunction>("str", 1, builtin_str))); globals->define("split", Value(std::make_shared<NativeFunction>("split", -1, builtin_split))); globals->define("join", Value(std::make_shared<NativeFunction>("join", 2, builtin_join)));
    // math helpers
    static auto builtin_abs = [](Interpreter&, const std::vector<Value>& args)->Value{ if(args.size()!=1) throw RuntimeError("abs expects 1 arg"); if(auto n=std::get_if<double>(&args[0].data)) return Value(std::abs(*n)); throw RuntimeError("abs expects number"); };
    globals->define("abs", Value(std::make_shared<NativeFunction>("abs", 1, builtin_abs)));
    // container helpers
    globals->define("has", Value(std::make_shared<NativeFunction>("has", 2, builtin_has)));
    // input helpers
    globals->define("list_input", Value(std::make_shared<NativeFunction>("list_input", -1, builtin_list_input)));
    // namespaced style requests get/post via dict
    Dict requests; requests["get"] = Value(std::make_shared<NativeFunction>("requests.get", 1, builtin_requests_get)); requests["post"] = Value(std::make_shared<NativeFunction>("requests.post", -1, builtin_requests_post)); requests["request"] = Value(std::make_shared<NativeFunction>("requests.request", -1, builtin_requests_request)); globals->define("requests", Value(requests));
    // filesystem namespace
    Dict fs; fs["read_text"] = Value(std::make_shared<NativeFunction>("fs.read_text", 1, builtin_fs_read_text)); fs["write_text"] = Value(std::make_shared<NativeFunction>("fs.write_text", 2, builtin_fs_write_text)); fs["exists"] = Value(std::make_shared<NativeFunction>("fs.exists", 1, builtin_fs_exists)); fs["listdir"] = Value(std::make_shared<NativeFunction>("fs.listdir", 1, builtin_fs_listdir)); fs["mkdirs"] = Value(std::make_shared<NativeFunction>("fs.mkdirs", 1, builtin_fs_mkdirs)); fs["remove"] = Value(std::make_shared<NativeFunction>("fs.remove", 1, builtin_fs_remove)); globals->define("fs", Value(fs));
    // content namespace
    Dict content; content["get"] = Value(std::make_shared<NativeFunction>("content.get", 1, builtin_content_get)); globals->define("content", Value(content));
    // c namespace (C execution)
    Dict cns; cns["run"] = Value(std::make_shared<NativeFunction>("c.run", -1, builtin_c_run)); globals->define("c", Value(cns));
Dict server; server["serve"] = Value(std::make_shared<NativeFunction>("server.serve", -1, builtin_server_serve)); globals->define("server", Value(server));
    // proc namespace (command execution)
    Dict proc; proc["exec"] = Value(std::make_shared<NativeFunction>("proc.exec", 1, builtin_proc_exec)); globals->define("proc", Value(proc));
    // native namespace (dynamic plugin loader)
    Dict native; native["load"] = Value(std::make_shared<NativeFunction>("native.load", 1, [](Interpreter& ip, const std::vector<Value>& a){ return builtin_native_load(ip,a);})); globals->define("native", Value(native)); }

// C API for embedding
extern "C" {
struct AdaScriptVM { Interpreter* ip; };

static char* adascript_strdup(const std::string& s){ char* p=(char*)std::malloc(s.size()+1); if(!p) return nullptr; std::memcpy(p, s.c_str(), s.size()+1); return p; }

ADASCRIPT_API AdaScriptVM* AdaScript_Create(const char* entry_dir){ try{ std::filesystem::path p = entry_dir? std::filesystem::path(entry_dir) : std::filesystem::current_path(); auto vm = new AdaScriptVM{ new Interpreter(p) }; return vm; } catch(...){ return nullptr; } }

ADASCRIPT_API void AdaScript_Destroy(AdaScriptVM* vm){ if(!vm) return; delete vm->ip; delete vm; }

static std::string value_to_string(const Value& v){ std::ostringstream oss; if(auto n=std::get_if<double>(&v.data)) oss<<*n; else if(auto s=std::get_if<std::string>(&v.data)) oss<<*s; else if(auto b=std::get_if<bool>(&v.data)) oss<<(*b?"true":"false"); else if(std::holds_alternative<std::monostate>(v.data)) oss<<"null"; else oss<<"<"<<v.typeName()<<">"; return oss.str(); }

ADASCRIPT_API int AdaScript_Eval(AdaScriptVM* vm, const char* source, const char* filename, char** error_message){ if(!vm||!source){ if(error_message) *error_message=adascript_strdup("invalid vm or source"); return 1; } try{ Lexer lx(source); auto tokens=lx.scan(); Parser ps(tokens); auto stmts=ps.parse(); if(filename){ vm->ip->current_dir = std::filesystem::path(filename).parent_path(); } vm->ip->interpret(stmts); return 0; } catch(const RuntimeError& e){ if(error_message) *error_message=adascript_strdup(e.what()); return 2; } catch(const std::exception& e){ if(error_message) *error_message=adascript_strdup(e.what()); return 3; } }

ADASCRIPT_API int AdaScript_RunFile(AdaScriptVM* vm, const char* path, char** error_message){ if(!vm||!path){ if(error_message) *error_message=adascript_strdup("invalid vm or path"); return 1; } try{ std::ifstream in(path, std::ios::binary); if(!in){ if(error_message) *error_message=adascript_strdup("failed to open file"); return 2; } std::ostringstream ss; ss<<in.rdbuf(); std::string src=ss.str(); Lexer lx(src); auto tokens=lx.scan(); Parser ps(tokens); auto stmts=ps.parse(); vm->ip->current_dir = std::filesystem::path(path).parent_path(); vm->ip->interpret(stmts); return 0; } catch(const RuntimeError& e){ if(error_message) *error_message=adascript_strdup(e.what()); return 3; } catch(const std::exception& e){ if(error_message) *error_message=adascript_strdup(e.what()); return 4; } }

ADASCRIPT_API char* AdaScript_Call(AdaScriptVM* vm, const char* func_name, const char* const* args, int argc, char** error_message){ if(!vm||!func_name){ if(error_message) *error_message=adascript_strdup("invalid vm or func_name"); return nullptr; } try{ Value* vptr = vm->ip->globals->getPtr(func_name); if(!vptr) throw RuntimeError(std::string("Undefined function: ")+func_name); std::vector<Value> av; av.reserve((size_t)argc); for(int i=0;i<argc;i++){ av.emplace_back(std::string(args[i]?args[i]:"")); }
    Value ret;
    if(auto nf = std::get_if<std::shared_ptr<NativeFunction>>(&vptr->data)){
        ret = (*nf)->call(*vm->ip, av);
    } else if(auto uf = std::get_if<std::shared_ptr<Function>>(&vptr->data)){
        ret = (*uf)->call(*vm->ip, av);
    } else if(auto kc = std::get_if<std::shared_ptr<Class>>(&vptr->data)){
        ret = (*kc)->call(*vm->ip, av);
    } else {
        throw RuntimeError("Target is not callable");
    }
    std::string out = value_to_string(ret); return adascript_strdup(out);
  } catch(const RuntimeError& e){ if(error_message) *error_message=adascript_strdup(e.what()); return nullptr; } catch(const std::exception& e){ if(error_message) *error_message=adascript_strdup(e.what()); return nullptr; } }

struct _NativeThunk { AdaScript_NativeStringFn fn; void* user; };

ADASCRIPT_API int AdaScript_RegisterNativeStringFn(AdaScriptVM* vm, const char* name, int arity, AdaScript_NativeStringFn fn, void* user_data){ if(!vm||!name||!fn) return 1; try{ auto thunk = std::make_shared<_NativeThunk>(); thunk->fn = fn; thunk->user = user_data; auto wrapper = std::make_shared<NativeFunction>(std::string(name), arity, [thunk](Interpreter&, const std::vector<Value>& args)->Value{
      std::vector<std::string> sargs; sargs.reserve(args.size()); for(const auto& v: args){ sargs.push_back(value_to_string(v)); }
      std::vector<const char*> cargs; cargs.reserve(sargs.size()); for(const auto& s: sargs){ cargs.push_back(s.c_str()); }
      char* res = thunk->fn(thunk->user, cargs.data(), (int)cargs.size());
      std::string out = res? std::string(res) : std::string("");
      if(res) std::free(res);
      return Value(out);
    });
    vm->ip->globals->define(name, Value(wrapper));
    return 0;
  } catch(...){ return 2; } }

ADASCRIPT_API void AdaScript_FreeString(char* s){ if(s) std::free(s); }
} // extern "C"

// Main
#ifndef ADASCRIPT_NO_MAIN
int main(int argc, char** argv){ std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
    if(argc<2){ std::cerr<<"Usage: adascript [--built-ins-location <dir>] <file.ad>\n"; return 1; }
    // Parse options
    int argi = 1; std::string script;
    std::string builtinsLoc;
    while(argi < argc){ std::string a = argv[argi]; if(a == "--built-ins-location"){ if(argi+1>=argc){ std::cerr<<"Missing value for --built-ins-location\n"; return 1; } builtinsLoc = argv[++argi]; argi++; continue; } else { script = a; argi++; break; } }
    if(script.empty()){ std::cerr<<"Missing script file\n"; return 1; }
    std::ifstream in(script, std::ios::binary); if(!in){ std::cerr<<"Failed to open: "<<script<<"\n"; return 1; }
    std::ostringstream ss; ss<<in.rdbuf(); std::string src = ss.str();
    try{
        Lexer lx(src); auto tokens = lx.scan(); Parser ps(tokens); auto stmts = ps.parse(); std::filesystem::path entry = std::filesystem::path(script).parent_path(); Interpreter ip(entry);
        if(!builtinsLoc.empty()) ip.builtins_dir = builtinsLoc;
        ip.interpret(stmts);
    } catch(const RuntimeError& e){ std::cerr<<"Error: "<<e.what()<<"\n"; return 1; }
    return 0; }
#endif

