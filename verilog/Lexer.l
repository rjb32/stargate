/* Verilog-2001 lexer (flex). Pairs with Parser.y. The custom YY_DECL
 * makes flex's yylex return bison's variant symbol_type directly.
 *
 * The lexer expects already-preprocessed input: `define / `include /
 * `ifdef etc. are handled upstream by Preprocessor. Anything still
 * spelled with a leading backtick at this layer is an undefined macro
 * reference; it is surfaced as a MACRO_REF token so the parser can
 * keep going (the grammar accepts MACRO_REF in a few empty/identifier
 * positions).
 *
 * Not reentrant: parsing two sources concurrently is out of scope. */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "VerilogDriver.h"
#include "Parser.h"

#define YY_DECL stargate::Parser::symbol_type \
    yylex(stargate::VerilogDriver& drv)

#define YY_USER_ACTION drv.location().columns(yyleng);

// SV keyword helper: emits the SV token when SV mode is on, or an
// IDENTIFIER carrying the same text when off. Used in the rules
// section for every keyword from IEEE 1800 that didn't already
// exist in IEEE 1364-2005 (logic, int, bit, always_ff, ...).
#define SV_KW(tok)                                                   \
    do {                                                             \
        if (drv.svKeywords())                                        \
            return stargate::Parser::make_##tok(drv.location());     \
        return stargate::Parser::make_IDENTIFIER(yytext,             \
            drv.location());                                         \
    } while (0)
%}

%option noyywrap nounput noinput batch

%x ATTR
%x BLOCK_COMMENT

DECDIGIT    [0-9]
SIZE        [1-9][0-9_]*
ID          [a-zA-Z_][a-zA-Z0-9_$]*

%%

%{
    drv.location().step();
%}

  /* --------- whitespace & line tracking --------- */

[ \t\r]+        { drv.location().step(); }
\n+             { drv.location().lines(yyleng); drv.location().step(); }

  /* --------- line comments --------- */

"//"[^\n]*      { drv.location().step(); }

  /* --------- block comments --------- */

"/*"            { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>{
    "*/"        { BEGIN(INITIAL); drv.location().step(); }
    \n          { drv.location().lines(1); drv.location().step(); }
    .           { /* swallow */ }
    <<EOF>>     {
        drv.addError(drv.location(), "unterminated block comment");
        BEGIN(INITIAL);
        return stargate::Parser::make_YYEOF(drv.location());
    }
}

  /* --------- attribute instances (* ... *) skipped --------- */
  /* Trailing context excludes ")" so "@(*)" is not mistaken for an
   * attribute opener — attributes require at least one attr_spec, so
   * "(*)" is never a valid attribute. */

"(*"/[^)]       { BEGIN(ATTR); }
<ATTR>{
    "*)"        { BEGIN(INITIAL); drv.location().step(); }
    \n          { drv.location().lines(1); drv.location().step(); }
    .           { /* swallow */ }
    <<EOF>>     {
        drv.addError(drv.location(), "unterminated attribute instance");
        BEGIN(INITIAL);
        return stargate::Parser::make_YYEOF(drv.location());
    }
}

  /* --------- string literals --------- */

\"([^\"\\\n]|\\.)*\"  {
    std::string s(yytext + 1, yyleng - 2);
    return stargate::Parser::make_STRING_LITERAL(s, drv.location());
}

\"([^\"\\\n]|\\.)*\n  {
    drv.addError(drv.location(),
        "unterminated string literal");
    drv.location().lines(1);
}

  /* --------- numbers --------- */
  /* Real with fraction (with or without exponent) */
{DECDIGIT}({DECDIGIT}|_)*"."{DECDIGIT}({DECDIGIT}|_)*([eE][+-]?{DECDIGIT}({DECDIGIT}|_)*)? {
    return stargate::Parser::make_REAL_NUMBER(yytext, drv.location());
}
  /* Real with exponent only (no fraction) */
{DECDIGIT}({DECDIGIT}|_)*[eE][+-]?{DECDIGIT}({DECDIGIT}|_)* {
    return stargate::Parser::make_REAL_NUMBER(yytext, drv.location());
}
  /* Sized literals. The LRM allows whitespace between size, ', base
   * char, and digits — e.g. "32'h 0000_0000". */
({SIZE})?[ \t]*'[sS]?[hH][ \t]*[0-9a-fA-FxXzZ?_]+ {
    return stargate::Parser::make_NUMBER(yytext, drv.location());
}
({SIZE})?[ \t]*'[sS]?[bB][ \t]*[01xXzZ?_]+ {
    return stargate::Parser::make_NUMBER(yytext, drv.location());
}
({SIZE})?[ \t]*'[sS]?[oO][ \t]*[0-7xXzZ?_]+ {
    return stargate::Parser::make_NUMBER(yytext, drv.location());
}
({SIZE})?[ \t]*'[sS]?[dD][ \t]*[0-9xXzZ?_]+ {
    return stargate::Parser::make_NUMBER(yytext, drv.location());
}
  /* SystemVerilog unsized untyped literals: '0, '1, 'x, 'z. */
'[01xXzZ]       {
    return stargate::Parser::make_NUMBER(yytext, drv.location());
}
{DECDIGIT}({DECDIGIT}|_)* {
    return stargate::Parser::make_NUMBER(yytext, drv.location());
}

  /* --------- multi-char operators (longest first) --------- */

"<<<"           { return stargate::Parser::make_LSHIFTA(drv.location()); }
">>>"           { return stargate::Parser::make_RSHIFTA(drv.location()); }
"==="           { return stargate::Parser::make_CASEEQ(drv.location()); }
"!=="           { return stargate::Parser::make_CASENEQ(drv.location()); }
"=="            { return stargate::Parser::make_EQEQ(drv.location()); }
"!="            { return stargate::Parser::make_NEQ(drv.location()); }
"<="            { return stargate::Parser::make_LE(drv.location()); }
">="            { return stargate::Parser::make_GE(drv.location()); }
"<<"            { return stargate::Parser::make_LSHIFT(drv.location()); }
">>"            { return stargate::Parser::make_RSHIFT(drv.location()); }
"&&"            { return stargate::Parser::make_LOGAND(drv.location()); }
"||"            { return stargate::Parser::make_LOGOR(drv.location()); }
"**"            { return stargate::Parser::make_POWER(drv.location()); }
"~&"            { return stargate::Parser::make_NAND_OP(drv.location()); }
"~|"            { return stargate::Parser::make_NOR_OP(drv.location()); }
"~^"            { return stargate::Parser::make_XNOR_OP(drv.location()); }
"^~"            { return stargate::Parser::make_XNOR_OP(drv.location()); }
"->"            { return stargate::Parser::make_ARROW(drv.location()); }
"=>"            { return stargate::Parser::make_GENARROW(drv.location()); }
"*>"            { return stargate::Parser::make_POLYARROW(drv.location()); }
"+:"            { return stargate::Parser::make_PLUS_COLON(drv.location()); }
"-:"            { return stargate::Parser::make_MINUS_COLON(drv.location()); }
"::"            { return stargate::Parser::make_DCOLON(drv.location()); }
"++"            { return stargate::Parser::make_INC(drv.location()); }
"--"            { return stargate::Parser::make_DEC(drv.location()); }
"+="            { return stargate::Parser::make_PLUS_EQ(drv.location()); }
"-="            { return stargate::Parser::make_MINUS_EQ(drv.location()); }
"*="            { return stargate::Parser::make_STAR_EQ(drv.location()); }
"/="            { return stargate::Parser::make_SLASH_EQ(drv.location()); }
"%="            { return stargate::Parser::make_PERCENT_EQ(drv.location()); }
"&="            { return stargate::Parser::make_AMP_EQ(drv.location()); }
"|="            { return stargate::Parser::make_PIPE_EQ(drv.location()); }
"^="            { return stargate::Parser::make_CARET_EQ(drv.location()); }
"<<="           { return stargate::Parser::make_LSHIFT_EQ(drv.location()); }
">>="           { return stargate::Parser::make_RSHIFT_EQ(drv.location()); }
"<<<="          { return stargate::Parser::make_LSHIFTA_EQ(drv.location()); }
">>>="          { return stargate::Parser::make_RSHIFTA_EQ(drv.location()); }
"'{"            { return stargate::Parser::make_APOST_LBRACE(drv.location()); }
"'("            { return stargate::Parser::make_APOST_LPAREN(drv.location()); }

  /* --------- keywords (alphabetical) --------- */
  /* SV-only keywords use the SV_KW() helper (defined in the C
   * preamble at the top of this file). It falls back to IDENTIFIER
   * when SV mode is off. The driver enables SV mode for `.sv` /
   * `.svh` paths and disables it for `.v` / `.vh`, so legacy
   * Verilog-2001 sources can keep using names like `int` or `logic`
   * as identifiers. */

"always"        { return stargate::Parser::make_K_ALWAYS(drv.location()); }
"always_comb"   { SV_KW(K_ALWAYS_COMB); }
"always_ff"     { SV_KW(K_ALWAYS_FF); }
"always_latch"  { SV_KW(K_ALWAYS_LATCH); }
"and"           { return stargate::Parser::make_K_AND(drv.location()); }
"assign"        { return stargate::Parser::make_K_ASSIGN(drv.location()); }
"automatic"     { return stargate::Parser::make_K_AUTOMATIC(drv.location()); }
"begin"         { return stargate::Parser::make_K_BEGIN(drv.location()); }
"bit"           { SV_KW(K_BIT); }
"break"         { SV_KW(K_BREAK); }
"buf"           { return stargate::Parser::make_K_BUF(drv.location()); }
"bufif0"        { return stargate::Parser::make_K_BUFIF0(drv.location()); }
"bufif1"        { return stargate::Parser::make_K_BUFIF1(drv.location()); }
"byte"          { SV_KW(K_BYTE); }
"case"          { return stargate::Parser::make_K_CASE(drv.location()); }
"casex"         { return stargate::Parser::make_K_CASEX(drv.location()); }
"casez"         { return stargate::Parser::make_K_CASEZ(drv.location()); }
"chandle"       { SV_KW(K_CHANDLE); }
"cmos"          { return stargate::Parser::make_K_CMOS(drv.location()); }
"const"         { SV_KW(K_CONST); }
"continue"      { SV_KW(K_CONTINUE); }
"deassign"      { return stargate::Parser::make_K_DEASSIGN(drv.location()); }
"default"       { return stargate::Parser::make_K_DEFAULT(drv.location()); }
"defparam"      { return stargate::Parser::make_K_DEFPARAM(drv.location()); }
"disable"       { return stargate::Parser::make_K_DISABLE(drv.location()); }
"do"            { SV_KW(K_DO); }
"edge"          { return stargate::Parser::make_K_EDGE(drv.location()); }
"else"          { return stargate::Parser::make_K_ELSE(drv.location()); }
"end"           { return stargate::Parser::make_K_END(drv.location()); }
"endcase"       { return stargate::Parser::make_K_ENDCASE(drv.location()); }
"endfunction"   { return stargate::Parser::make_K_ENDFUNCTION(drv.location()); }
"endgenerate"   { return stargate::Parser::make_K_ENDGENERATE(drv.location()); }
"endinterface"  { SV_KW(K_ENDINTERFACE); }
"endmodule"     { return stargate::Parser::make_K_ENDMODULE(drv.location()); }
"endpackage"    { SV_KW(K_ENDPACKAGE); }
"endprimitive"  { return stargate::Parser::make_K_ENDPRIMITIVE(drv.location()); }
"endspecify"    { return stargate::Parser::make_K_ENDSPECIFY(drv.location()); }
"endtable"      { return stargate::Parser::make_K_ENDTABLE(drv.location()); }
"endtask"       { return stargate::Parser::make_K_ENDTASK(drv.location()); }
"enum"          { SV_KW(K_ENUM); }
"event"         { return stargate::Parser::make_K_EVENT(drv.location()); }
"export"        { SV_KW(K_EXPORT); }
"extern"        { SV_KW(K_EXTERN); }
"final"         { SV_KW(K_FINAL); }
"for"           { return stargate::Parser::make_K_FOR(drv.location()); }
"force"         { return stargate::Parser::make_K_FORCE(drv.location()); }
"forever"       { return stargate::Parser::make_K_FOREVER(drv.location()); }
"fork"          { return stargate::Parser::make_K_FORK(drv.location()); }
"function"      { return stargate::Parser::make_K_FUNCTION(drv.location()); }
"generate"      { return stargate::Parser::make_K_GENERATE(drv.location()); }
"genvar"        { return stargate::Parser::make_K_GENVAR(drv.location()); }
"highz0"        { return stargate::Parser::make_K_HIGHZ0(drv.location()); }
"highz1"        { return stargate::Parser::make_K_HIGHZ1(drv.location()); }
"if"            { return stargate::Parser::make_K_IF(drv.location()); }
"iff"           { SV_KW(K_IFF); }
"ifnone"        { return stargate::Parser::make_K_IFNONE(drv.location()); }
"import"        { SV_KW(K_IMPORT); }
"initial"       { return stargate::Parser::make_K_INITIAL(drv.location()); }
"inout"         { return stargate::Parser::make_K_INOUT(drv.location()); }
"input"         { return stargate::Parser::make_K_INPUT(drv.location()); }
"inside"        { SV_KW(K_INSIDE); }
"int"           { SV_KW(K_INT); }
"integer"       { return stargate::Parser::make_K_INTEGER(drv.location()); }
"interface"     { SV_KW(K_INTERFACE); }
"join"          { return stargate::Parser::make_K_JOIN(drv.location()); }
"large"         { return stargate::Parser::make_K_LARGE(drv.location()); }
"local"         { SV_KW(K_LOCAL); }
"localparam"    { return stargate::Parser::make_K_LOCALPARAM(drv.location()); }
"logic"         { SV_KW(K_LOGIC); }
"longint"       { SV_KW(K_LONGINT); }
"macromodule"   { return stargate::Parser::make_K_MACROMODULE(drv.location()); }
"medium"        { return stargate::Parser::make_K_MEDIUM(drv.location()); }
"modport"       { SV_KW(K_MODPORT); }
"module"        { return stargate::Parser::make_K_MODULE(drv.location()); }
"nand"          { return stargate::Parser::make_K_NAND(drv.location()); }
"negedge"       { return stargate::Parser::make_K_NEGEDGE(drv.location()); }
"nmos"          { return stargate::Parser::make_K_NMOS(drv.location()); }
"nor"           { return stargate::Parser::make_K_NOR(drv.location()); }
"not"           { return stargate::Parser::make_K_NOT(drv.location()); }
"notif0"        { return stargate::Parser::make_K_NOTIF0(drv.location()); }
"notif1"        { return stargate::Parser::make_K_NOTIF1(drv.location()); }
"null"          { SV_KW(K_NULL); }
"or"            { return stargate::Parser::make_K_OR(drv.location()); }
"output"        { return stargate::Parser::make_K_OUTPUT(drv.location()); }
"package"       { SV_KW(K_PACKAGE); }
"packed"        { SV_KW(K_PACKED); }
"parameter"     { return stargate::Parser::make_K_PARAMETER(drv.location()); }
"pmos"          { return stargate::Parser::make_K_PMOS(drv.location()); }
"posedge"       { return stargate::Parser::make_K_POSEDGE(drv.location()); }
"primitive"     { return stargate::Parser::make_K_PRIMITIVE(drv.location()); }
"priority"      { SV_KW(K_PRIORITY); }
"protected"     { SV_KW(K_PROTECTED); }
"pure"          { SV_KW(K_PURE); }
"pull0"         { return stargate::Parser::make_K_PULL0(drv.location()); }
"pull1"         { return stargate::Parser::make_K_PULL1(drv.location()); }
"pulldown"      { return stargate::Parser::make_K_PULLDOWN(drv.location()); }
"pullup"        { return stargate::Parser::make_K_PULLUP(drv.location()); }
"rcmos"         { return stargate::Parser::make_K_RCMOS(drv.location()); }
"real"          { return stargate::Parser::make_K_REAL(drv.location()); }
"realtime"      { return stargate::Parser::make_K_REALTIME(drv.location()); }
"reg"           { return stargate::Parser::make_K_REG(drv.location()); }
"release"       { return stargate::Parser::make_K_RELEASE(drv.location()); }
"repeat"        { return stargate::Parser::make_K_REPEAT(drv.location()); }
"return"        { SV_KW(K_RETURN); }
"rnmos"         { return stargate::Parser::make_K_RNMOS(drv.location()); }
"rpmos"         { return stargate::Parser::make_K_RPMOS(drv.location()); }
"rtran"         { return stargate::Parser::make_K_RTRAN(drv.location()); }
"rtranif0"      { return stargate::Parser::make_K_RTRANIF0(drv.location()); }
"rtranif1"      { return stargate::Parser::make_K_RTRANIF1(drv.location()); }
"scalared"      { return stargate::Parser::make_K_SCALARED(drv.location()); }
"shortint"      { SV_KW(K_SHORTINT); }
"signed"        { return stargate::Parser::make_K_SIGNED(drv.location()); }
"small"         { return stargate::Parser::make_K_SMALL(drv.location()); }
"specify"       { return stargate::Parser::make_K_SPECIFY(drv.location()); }
"specparam"     { return stargate::Parser::make_K_SPECPARAM(drv.location()); }
"static"        { SV_KW(K_STATIC); }
"string"        { SV_KW(K_STRING); }
"strong0"       { return stargate::Parser::make_K_STRONG0(drv.location()); }
"strong1"       { return stargate::Parser::make_K_STRONG1(drv.location()); }
"struct"        { SV_KW(K_STRUCT); }
"supply0"       { return stargate::Parser::make_K_SUPPLY0(drv.location()); }
"supply1"       { return stargate::Parser::make_K_SUPPLY1(drv.location()); }
"table"         { return stargate::Parser::make_K_TABLE(drv.location()); }
"task"          { return stargate::Parser::make_K_TASK(drv.location()); }
"time"          { return stargate::Parser::make_K_TIME(drv.location()); }
"tran"          { return stargate::Parser::make_K_TRAN(drv.location()); }
"tranif0"       { return stargate::Parser::make_K_TRANIF0(drv.location()); }
"tranif1"       { return stargate::Parser::make_K_TRANIF1(drv.location()); }
"tri"           { return stargate::Parser::make_K_TRI(drv.location()); }
"tri0"          { return stargate::Parser::make_K_TRI0(drv.location()); }
"tri1"          { return stargate::Parser::make_K_TRI1(drv.location()); }
"triand"        { return stargate::Parser::make_K_TRIAND(drv.location()); }
"trior"         { return stargate::Parser::make_K_TRIOR(drv.location()); }
"trireg"        { return stargate::Parser::make_K_TRIREG(drv.location()); }
"type"          { SV_KW(K_TYPE); }
"typedef"       { SV_KW(K_TYPEDEF); }
"union"         { SV_KW(K_UNION); }
"unique"        { SV_KW(K_UNIQUE); }
"unique0"       { SV_KW(K_UNIQUE0); }
"unsigned"      { return stargate::Parser::make_K_UNSIGNED(drv.location()); }
"uwire"         { return stargate::Parser::make_K_UWIRE(drv.location()); }
"var"           { SV_KW(K_VAR); }
"vectored"      { return stargate::Parser::make_K_VECTORED(drv.location()); }
"virtual"       { SV_KW(K_VIRTUAL); }
"void"          { SV_KW(K_VOID); }
"wait"          { return stargate::Parser::make_K_WAIT(drv.location()); }
"wand"          { return stargate::Parser::make_K_WAND(drv.location()); }
"weak0"         { return stargate::Parser::make_K_WEAK0(drv.location()); }
"weak1"         { return stargate::Parser::make_K_WEAK1(drv.location()); }
"while"         { return stargate::Parser::make_K_WHILE(drv.location()); }
"wire"          { return stargate::Parser::make_K_WIRE(drv.location()); }
"wor"           { return stargate::Parser::make_K_WOR(drv.location()); }
"xnor"          { return stargate::Parser::make_K_XNOR(drv.location()); }
"xor"           { return stargate::Parser::make_K_XOR(drv.location()); }

  /* --------- system task / function identifiers --------- */

"$"{ID}         {
    return stargate::Parser::make_SYSTEM_ID(yytext, drv.location());
}

  /* --------- identifiers (escaped form first) --------- */

\\[^ \t\r\n]+   {
    return stargate::Parser::make_IDENTIFIER(
        std::string(yytext + 1, yyleng - 1), drv.location());
}

  /* SystemVerilog scope-resolution prefix: an identifier immediately
   * followed by `::`. We lex it as a single token so the grammar can
   * distinguish `pkg::T` from a bare identifier without needing
   * lookahead past the trailing `::`. yytext includes the trailing
   * `::`; we strip it so the value carries just the package name.
   * Longest-match in flex makes this rule fire instead of the plain
   * `{ID}` rule whenever `::` follows. */
{ID}"::"        {
    std::string name(yytext, yyleng - 2);
    return stargate::Parser::make_SCOPED_NAME_HEAD(name, drv.location());
}

{ID}            {
    return stargate::Parser::make_IDENTIFIER(yytext, drv.location());
}

  /* --------- macro reference that survived the preprocessor --------- */
  /* The preprocessor handles `define / `include / `ifdef / etc. and
   * expands defined macros. Anything spelled `<id> reaching the lexer
   * is therefore an undefined macro reference; pass it through as a
   * MACRO_REF token so the grammar can keep parsing. */

"`"{ID}         {
    return stargate::Parser::make_MACRO_REF(yytext, drv.location());
}

  /* --------- single-char punctuation / operators --------- */

";"             { return stargate::Parser::make_SEMI(drv.location()); }
","             { return stargate::Parser::make_COMMA(drv.location()); }
"."             { return stargate::Parser::make_DOT(drv.location()); }
":"             { return stargate::Parser::make_COLON(drv.location()); }
"?"             { return stargate::Parser::make_QUESTION(drv.location()); }
"@"             { return stargate::Parser::make_AT(drv.location()); }
"#"             { return stargate::Parser::make_HASH(drv.location()); }
"="             { return stargate::Parser::make_ASSIGN(drv.location()); }
"("             { return stargate::Parser::make_LPAREN(drv.location()); }
")"             { return stargate::Parser::make_RPAREN(drv.location()); }
"["             { return stargate::Parser::make_LBRACK(drv.location()); }
"]"             { return stargate::Parser::make_RBRACK(drv.location()); }
"{"             { return stargate::Parser::make_LBRACE(drv.location()); }
"}"             { return stargate::Parser::make_RBRACE(drv.location()); }
"+"             { return stargate::Parser::make_PLUS(drv.location()); }
"-"             { return stargate::Parser::make_MINUS(drv.location()); }
"*"             { return stargate::Parser::make_STAR(drv.location()); }
"/"             { return stargate::Parser::make_SLASH(drv.location()); }
"%"             { return stargate::Parser::make_PERCENT(drv.location()); }
"<"             { return stargate::Parser::make_LT(drv.location()); }
">"             { return stargate::Parser::make_GT(drv.location()); }
"!"             { return stargate::Parser::make_BANG(drv.location()); }
"~"             { return stargate::Parser::make_TILDE(drv.location()); }
"&"             { return stargate::Parser::make_AMP(drv.location()); }
"|"             { return stargate::Parser::make_PIPE(drv.location()); }
"^"             { return stargate::Parser::make_CARET(drv.location()); }

  /* --------- end of file --------- */

<<EOF>>         { return stargate::Parser::make_YYEOF(drv.location()); }

  /* --------- error catch-all --------- */

.               {
    drv.addError(drv.location(),
        std::string("unexpected character: '") + yytext + "'");
}

%%

namespace stargate {

void scanBeginFile(VerilogDriver& drv, const std::string& path) {
    yyin = fopen(path.c_str(), "r");
    if (!yyin) {
        drv.addError(drv.location(),
            "cannot open file: " + path);
        return;
    }
    YY_BUFFER_STATE buf = yy_create_buffer(yyin, YY_BUF_SIZE);
    yy_switch_to_buffer(buf);
}

void scanBeginString(VerilogDriver&, const std::string& source) {
    yy_scan_bytes(source.c_str(),
        static_cast<int>(source.size()));
}

void scanEnd() {
    if (yyin && yyin != stdin) {
        fclose(yyin);
        yyin = nullptr;
    }
    yylex_destroy();
}

}
