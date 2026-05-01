// Verilog-2001 (IEEE 1364-2005) parser, transcribed from Annex A.
// Recognizer-only: every semantic action body is empty. An AST will
// land in a follow-up.
//
// SystemVerilog (IEEE 1800) — pragmatic subset.
// The grammar accepts a useful slice of SV beyond plain Verilog-2001:
// `package`/`endpackage`, `import`, `typedef` (enum/struct/vector),
// `logic`/`bit`/`int`/... data types, ANSI port lists with typed
// directions, `parameter int unsigned X = ...` and `parameter type T`,
// `always_ff`/`always_comb`/`always_latch`, `unique`/`priority` case,
// scope resolution (`pkg::id`), unsized untyped literals
// (`'0`/`'1`/`'x`/`'z`), assignment patterns (`'{...}`), the cast
// operator `<size_or_type>'(expr)`, post-increment/decrement,
// inline `int` declarations in `for` headers, and unpacked
// dimensions on variable declarations.
// SV constructs deliberately out of scope: classes, interfaces with
// modports, randomization, assertions/properties/sequences,
// constraints, virtual interfaces.

%language "c++"
%skeleton "lalr1.cc"
%require "3.5"

%locations
%define api.namespace { stargate }
%define api.parser.class { Parser }
%define api.value.type variant
%define api.token.constructor
%define parse.error verbose
%define parse.trace

%code requires {
    #include <string>
    namespace stargate { class VerilogDriver; }
}

%param { stargate::VerilogDriver& drv }

%code {
    #include "VerilogDriver.h"

    stargate::Parser::symbol_type yylex(stargate::VerilogDriver& drv);
}

%token YYEOF 0 "end of file"

// ============================================================
// Punctuation
// ============================================================
%token SEMI         ";"
%token COMMA        ","
%token DOT          "."
%token COLON        ":"
%token QUESTION     "?"
%token AT           "@"
%token HASH         "#"
%token ASSIGN       "="
%token LPAREN       "("
%token RPAREN       ")"
%token LBRACK       "["
%token RBRACK       "]"
%token LBRACE       "{"
%token RBRACE       "}"

// ============================================================
// Operators
// ============================================================
%token PLUS         "+"
%token MINUS        "-"
%token STAR         "*"
%token SLASH        "/"
%token PERCENT      "%"
%token POWER        "**"
%token LT           "<"
%token GT           ">"
%token LE           "<="
%token GE           ">="
%token EQEQ         "=="
%token NEQ          "!="
%token CASEEQ       "==="
%token CASENEQ      "!=="
%token LOGAND       "&&"
%token LOGOR        "||"
%token BANG         "!"
%token TILDE        "~"
%token AMP          "&"
%token PIPE         "|"
%token CARET        "^"
%token NAND_OP      "~&"
%token NOR_OP       "~|"
%token XNOR_OP      "~^"
%token LSHIFT       "<<"
%token RSHIFT       ">>"
%token LSHIFTA      "<<<"
%token RSHIFTA      ">>>"
%token ARROW        "->"
%token GENARROW     "=>"
%token POLYARROW    "*>"
%token PLUS_COLON   "+:"
%token MINUS_COLON  "-:"
%token DCOLON       "::"
%token INC          "++"
%token DEC          "--"
%token PLUS_EQ      "+="
%token MINUS_EQ     "-="
%token STAR_EQ      "*="
%token SLASH_EQ     "/="
%token PERCENT_EQ   "%="
%token AMP_EQ       "&="
%token PIPE_EQ      "|="
%token CARET_EQ     "^="
%token LSHIFT_EQ    "<<="
%token RSHIFT_EQ    ">>="
%token LSHIFTA_EQ   "<<<="
%token RSHIFTA_EQ   ">>>="
%token APOST_LBRACE "'{"
%token APOST_LPAREN "'("

// ============================================================
// Keywords (alphabetical)
// ============================================================
%token K_ALWAYS         "always"
%token K_ALWAYS_COMB    "always_comb"
%token K_ALWAYS_FF      "always_ff"
%token K_ALWAYS_LATCH   "always_latch"
%token K_AND            "and"
%token K_ASSIGN         "assign"
%token K_AUTOMATIC      "automatic"
%token K_BEGIN          "begin"
%token K_BIT            "bit"
%token K_BREAK          "break"
%token K_BUF            "buf"
%token K_BUFIF0         "bufif0"
%token K_BUFIF1         "bufif1"
%token K_BYTE           "byte"
%token K_CASE           "case"
%token K_CASEX          "casex"
%token K_CASEZ          "casez"
%token K_CHANDLE        "chandle"
%token K_CMOS           "cmos"
%token K_CONST          "const"
%token K_CONTINUE       "continue"
%token K_DEASSIGN       "deassign"
%token K_DEFAULT        "default"
%token K_DEFPARAM       "defparam"
%token K_DISABLE        "disable"
%token K_DO             "do"
%token K_EDGE           "edge"
%token K_ELSE           "else"
%token K_END            "end"
%token K_ENDCASE        "endcase"
%token K_ENDFUNCTION    "endfunction"
%token K_ENDGENERATE    "endgenerate"
%token K_ENDINTERFACE   "endinterface"
%token K_ENDMODULE      "endmodule"
%token K_ENDPACKAGE     "endpackage"
%token K_ENDPRIMITIVE   "endprimitive"
%token K_ENDSPECIFY     "endspecify"
%token K_ENDTABLE       "endtable"
%token K_ENDTASK        "endtask"
%token K_ENUM           "enum"
%token K_EVENT          "event"
%token K_EXPORT         "export"
%token K_EXTERN         "extern"
%token K_FINAL          "final"
%token K_FOR            "for"
%token K_FORCE          "force"
%token K_FOREVER        "forever"
%token K_FORK           "fork"
%token K_FUNCTION       "function"
%token K_GENERATE       "generate"
%token K_GENVAR         "genvar"
%token K_HIGHZ0         "highz0"
%token K_HIGHZ1         "highz1"
%token K_IF             "if"
%token K_IFF            "iff"
%token K_IFNONE         "ifnone"
%token K_IMPORT         "import"
%token K_INITIAL        "initial"
%token K_INOUT          "inout"
%token K_INPUT          "input"
%token K_INSIDE         "inside"
%token K_INT            "int"
%token K_INTEGER        "integer"
%token K_INTERFACE      "interface"
%token K_JOIN           "join"
%token K_LARGE          "large"
%token K_LOCAL          "local"
%token K_LOCALPARAM     "localparam"
%token K_LOGIC          "logic"
%token K_LONGINT        "longint"
%token K_MACROMODULE    "macromodule"
%token K_MEDIUM         "medium"
%token K_MODPORT        "modport"
%token K_MODULE         "module"
%token K_NAND           "nand"
%token K_NEGEDGE        "negedge"
%token K_NMOS           "nmos"
%token K_NOR            "nor"
%token K_NOT            "not"
%token K_NOTIF0         "notif0"
%token K_NOTIF1         "notif1"
%token K_NULL           "null"
%token K_OR             "or"
%token K_OUTPUT         "output"
%token K_PACKAGE        "package"
%token K_PACKED         "packed"
%token K_PARAMETER      "parameter"
%token K_PMOS           "pmos"
%token K_POSEDGE        "posedge"
%token K_PRIMITIVE      "primitive"
%token K_PRIORITY       "priority"
%token K_PROTECTED      "protected"
%token K_PURE           "pure"
%token K_PULL0          "pull0"
%token K_PULL1          "pull1"
%token K_PULLDOWN       "pulldown"
%token K_PULLUP         "pullup"
%token K_RCMOS          "rcmos"
%token K_REAL           "real"
%token K_REALTIME       "realtime"
%token K_REG            "reg"
%token K_RELEASE        "release"
%token K_REPEAT         "repeat"
%token K_RETURN         "return"
%token K_RNMOS          "rnmos"
%token K_RPMOS          "rpmos"
%token K_RTRAN          "rtran"
%token K_RTRANIF0       "rtranif0"
%token K_RTRANIF1       "rtranif1"
%token K_SCALARED       "scalared"
%token K_SHORTINT       "shortint"
%token K_SIGNED         "signed"
%token K_SMALL          "small"
%token K_SPECIFY        "specify"
%token K_SPECPARAM      "specparam"
%token K_STATIC         "static"
%token K_STRING         "string"
%token K_STRONG0        "strong0"
%token K_STRONG1        "strong1"
%token K_STRUCT         "struct"
%token K_SUPPLY0        "supply0"
%token K_SUPPLY1        "supply1"
%token K_TABLE          "table"
%token K_TASK           "task"
%token K_TIME           "time"
%token K_TRAN           "tran"
%token K_TRANIF0        "tranif0"
%token K_TRANIF1        "tranif1"
%token K_TRI            "tri"
%token K_TRI0           "tri0"
%token K_TRI1           "tri1"
%token K_TRIAND         "triand"
%token K_TRIOR          "trior"
%token K_TRIREG         "trireg"
%token K_TYPE           "type"
%token K_TYPEDEF        "typedef"
%token K_UNION          "union"
%token K_UNIQUE         "unique"
%token K_UNIQUE0        "unique0"
%token K_UNSIGNED       "unsigned"
%token K_UWIRE          "uwire"
%token K_VAR            "var"
%token K_VECTORED       "vectored"
%token K_VIRTUAL        "virtual"
%token K_VOID           "void"
%token K_WAIT           "wait"
%token K_WAND           "wand"
%token K_WEAK0          "weak0"
%token K_WEAK1          "weak1"
%token K_WHILE          "while"
%token K_WIRE           "wire"
%token K_WOR            "wor"
%token K_XNOR           "xnor"
%token K_XOR            "xor"

// ============================================================
// Literals & identifiers
// ============================================================
%token <std::string> NUMBER
%token <std::string> REAL_NUMBER
%token <std::string> STRING_LITERAL
%token <std::string> IDENTIFIER
%token <std::string> SYSTEM_ID
%token <std::string> MACRO_REF
%token <std::string> SCOPED_NAME_HEAD    "<ident>::"

// ============================================================
// Precedence (lowest to highest)
// ============================================================
%nonassoc IF_NO_ELSE
%nonassoc K_ELSE

%right QUESTION COLON
%left LOGOR
%left LOGAND
%left PIPE NOR_OP
%left CARET XNOR_OP
%left AMP NAND_OP
%left EQEQ NEQ CASEEQ CASENEQ
%left LT LE GT GE
%left LSHIFT RSHIFT LSHIFTA RSHIFTA
%left PLUS MINUS
%left STAR SLASH PERCENT
%right POWER
%right UNARY_PREC

%%

// ============================================================
// Top level
// ============================================================
source_text
    : %empty                              { }
    | source_text description             { }
    ;

description
    : module_declaration                  { }
    | package_declaration                 { }
    | interface_declaration               { }
    | sv_typedef_declaration              { }
    | sv_package_import_declaration       { }
    ;

// ============================================================
// SystemVerilog package
// ============================================================
package_declaration
    : K_PACKAGE opt_lifetime IDENTIFIER SEMI
        package_item_list_opt K_ENDPACKAGE opt_endlabel             { }
    ;

opt_lifetime
    : %empty                                                        { }
    | K_STATIC                                                      { }
    | K_AUTOMATIC                                                   { }
    ;

opt_endlabel
    : %empty                                                        { }
    | COLON IDENTIFIER                                              { }
    ;

package_item_list_opt
    : %empty                                                        { }
    | package_item_list                                             { }
    ;

package_item_list
    : package_item                                                  { }
    | package_item_list package_item                                { }
    ;

package_item
    : parameter_declaration                                         { }
    | localparam_declaration                                        { }
    | sv_typedef_declaration                                        { }
    | function_declaration                                          { }
    | task_declaration                                              { }
    | net_declaration                                               { }
    | reg_declaration                                               { }
    | sv_data_declaration                                           { }
    | sv_package_import_declaration                                 { }
    ;

// ============================================================
// SystemVerilog interface (accepted as a module-shaped container)
// ============================================================
interface_declaration
    : K_INTERFACE IDENTIFIER opt_module_parameter_port_list
        opt_list_of_ports SEMI module_item_list_opt
        K_ENDINTERFACE opt_endlabel                                 { }
    ;

// ============================================================
// SV package import / export
// ============================================================
sv_package_import_declaration
    : K_IMPORT package_import_item_list SEMI                        { }
    | K_EXPORT package_import_item_list SEMI                        { }
    ;

package_import_item_list
    : package_import_item                                           { }
    | package_import_item_list COMMA package_import_item            { }
    ;

package_import_item
    : SCOPED_NAME_HEAD STAR                                         { }
    | SCOPED_NAME_HEAD IDENTIFIER                                   { }
    ;

// ============================================================
// SV typedef and data types
// ============================================================
sv_typedef_declaration
    : K_TYPEDEF data_type_or_user IDENTIFIER opt_unpacked_dim_list SEMI { }
    | K_TYPEDEF IDENTIFIER SEMI                                     { }
    ;

// Keyword-prefixed data types. Anything starting with an IDENTIFIER
// (whether `pkg::T` or unscoped `mytype`) is intentionally NOT here:
// at module-item / block-item level an IDENTIFIER also begins a
// statement, a module instantiation, or a procedural assignment, so
// allowing it as `data_type` produces LALR(1) shift-reduce conflicts
// the parser resolves by waiting for tokens that never come (e.g.
// shifting `IDENTIFIER` expecting `::` and then failing on `<=`).
// User-defined types are accepted only via `data_type_or_user` in
// contexts that are syntactically locked (typedef body, struct or
// union member) where the surrounding rule disambiguates.
data_type
    : integer_atom_type opt_signedness                              { }
    | integer_vector_type opt_signedness opt_packed_dim_list        { }
    | non_integer_type                                              { }
    | K_STRUCT opt_packed opt_signedness LBRACE struct_member_list RBRACE
        opt_packed_dim_list                                         { }
    | K_UNION opt_packed opt_signedness LBRACE struct_member_list RBRACE
        opt_packed_dim_list                                         { }
    | K_ENUM opt_enum_base LBRACE enum_member_list RBRACE
        opt_packed_dim_list                                         { }
    | K_STRING                                                      { }
    | K_CHANDLE                                                     { }
    | K_EVENT                                                       { }
    | K_VIRTUAL K_INTERFACE IDENTIFIER                              { }
    ;

data_type_or_user
    : data_type                                                     { }
    | IDENTIFIER opt_packed_dim_list                                { }
    | SCOPED_NAME_HEAD IDENTIFIER opt_packed_dim_list               { }
    ;

integer_atom_type
    : K_BYTE                                                        { }
    | K_SHORTINT                                                    { }
    | K_INT                                                         { }
    | K_LONGINT                                                     { }
    | K_INTEGER                                                     { }
    | K_TIME                                                        { }
    ;

integer_vector_type
    : K_BIT                                                         { }
    | K_LOGIC                                                       { }
    | K_REG                                                         { }
    ;

non_integer_type
    : K_REAL                                                        { }
    | K_REALTIME                                                    { }
    | K_SHORTINT                                                    { }
    ;

opt_packed
    : %empty                                                        { }
    | K_PACKED                                                      { }
    ;

opt_signedness
    : %empty                                                        { }
    | K_SIGNED                                                      { }
    | K_UNSIGNED                                                    { }
    ;

opt_packed_dim_list
    : %empty                                                        { }
    | packed_dim_list                                               { }
    ;

packed_dim_list
    : packed_dim                                                    { }
    | packed_dim_list packed_dim                                    { }
    ;

packed_dim
    : range                                                         { }
    ;

opt_unpacked_dim_list
    : %empty                                                        { }
    | unpacked_dim_list                                             { }
    ;

unpacked_dim_list
    : unpacked_dim                                                  { }
    | unpacked_dim_list unpacked_dim                                { }
    ;

unpacked_dim
    : LBRACK expression RBRACK                                      { }
    | LBRACK expression COLON expression RBRACK                     { }
    ;


opt_enum_base
    : %empty                                                        { }
    | integer_atom_type opt_signedness                              { }
    | integer_vector_type opt_signedness opt_packed_dim_list        { }
    ;

enum_member_list
    : enum_member                                                   { }
    | enum_member_list COMMA enum_member                            { }
    ;

enum_member
    : IDENTIFIER                                                    { }
    | IDENTIFIER ASSIGN expression                                  { }
    | IDENTIFIER LBRACK expression RBRACK                           { }
    | IDENTIFIER LBRACK expression COLON expression RBRACK          { }
    | IDENTIFIER LBRACK expression RBRACK ASSIGN expression         { }
    ;

struct_member_list
    : struct_member                                                 { }
    | struct_member_list struct_member                              { }
    ;

struct_member
    : data_type_or_user identifier_list SEMI                        { }
    ;

// ============================================================
// SV data declaration: `logic [W-1:0] x;`, `int x = 0;`, etc.
// ============================================================
sv_data_declaration
    : opt_const opt_var_lifetime data_type list_of_var_decl_assignments SEMI  { }
    | K_VAR opt_var_lifetime data_type_or_implicit
        list_of_var_decl_assignments SEMI                           { }
    ;

opt_const
    : %empty                                                        { }
    | K_CONST                                                       { }
    ;

opt_var_lifetime
    : %empty                                                        { }
    | K_STATIC                                                      { }
    | K_AUTOMATIC                                                   { }
    ;

data_type_or_implicit
    : data_type                                                     { }
    | opt_signedness opt_packed_dim_list                            { }
    ;

// ============================================================
// Module declaration
// ============================================================
module_declaration
    : module_keyword IDENTIFIER opt_module_imports
        opt_module_parameter_port_list
        opt_list_of_ports SEMI module_item_list_opt
        K_ENDMODULE opt_endlabel                                { }
    ;

opt_module_imports
    : %empty                                                    { }
    | module_imports                                            { }
    ;

module_imports
    : sv_package_import_declaration                             { }
    | module_imports sv_package_import_declaration              { }
    ;

module_keyword
    : K_MODULE                            { }
    | K_MACROMODULE                       { }
    ;

opt_module_parameter_port_list
    : %empty                                                      { }
    | HASH LPAREN module_parameter_port_decls RPAREN              { }
    ;

module_parameter_port_decls
    : module_parameter_port_decl                                  { }
    | module_parameter_port_decls COMMA module_parameter_port_decl { }
    ;

// Module-port parameter declaration. The IDENTIFIER alternative
// disambiguates an unscoped user-type prefix (`parameter foo_t X =
// ...`) from a bare parameter (`parameter X = ...`) by deferring the
// decision to `mp_after_param_ident`, which inspects the token after
// the first IDENTIFIER: ASSIGN means bare, another IDENTIFIER means
// type-then-name.
module_parameter_port_decl
    : K_PARAMETER opt_param_type_or_range param_assignment        { }
    | K_PARAMETER sv_param_data_type param_assignment             { }
    | K_PARAMETER K_TYPE param_type_assignment                    { }
    | K_PARAMETER IDENTIFIER mp_after_param_ident                 { }
    | sv_param_data_type param_assignment                         { }
    | param_assignment                                            { }
    ;

mp_after_param_ident
    : opt_unpacked_dim_list ASSIGN expression                     { }
    | opt_packed_dim_list IDENTIFIER opt_unpacked_dim_list
        ASSIGN expression                                         { }
    ;

sv_param_data_type
    : K_LOGIC opt_signedness opt_packed_dim_list                  { }
    | K_BIT opt_signedness opt_packed_dim_list                    { }
    | K_BYTE opt_signedness                                       { }
    | K_INT opt_signedness                                        { }
    | K_LONGINT opt_signedness                                    { }
    | K_SHORTINT opt_signedness                                   { }
    | K_INTEGER opt_signedness                                    { }
    | K_TIME                                                      { }
    | K_REAL                                                      { }
    | K_REALTIME                                                  { }
    | K_STRING                                                    { }
    | SCOPED_NAME_HEAD IDENTIFIER opt_packed_dim_list             { }
    ;

param_type_assignment
    : IDENTIFIER ASSIGN data_type                                 { }
    | IDENTIFIER                                                  { }
    ;

opt_list_of_ports
    : %empty                              { }
    | LPAREN RPAREN                       { }
    | LPAREN port_list RPAREN             { }
    ;

port_list
    : port_item                           { }
    | port_list COMMA port_item           { }
    ;

// `port_item` accepts the four major shapes:
//   1. port_reference_or_named (no direction): `a`, `a[3:0]`, `.x(y)`
//   2. port_direction + classic Verilog: `input wire [n:0] x`,
//      `output reg signed x`, etc.
//   3. port_direction + SV keyword type: `input logic [n:0] x`.
//   4. port_direction + IDENTIFIER prefix: `input x` (port name only)
//      or `output crash_dump_t crash_dump_o` (unscoped user type
//      then name). Disambiguated via `port_after_dir_ident_tail`.
port_item
    : port_reference_or_named             { }
    | port_direction port_typed_classic_form                        { }
    | port_direction sv_port_data_type IDENTIFIER opt_unpacked_dim_list { }
    | port_direction sv_port_data_type IDENTIFIER opt_unpacked_dim_list
        ASSIGN expression                                            { }
    | port_direction IDENTIFIER port_after_dir_ident_tail           { }
    ;

// Classic Verilog-2001 typed forms — start with a net_type, K_REG,
// K_SIGNED, or a `range`. Empty form is intentionally NOT here; it
// is handled via the IDENTIFIER alternative of `port_item`.
port_typed_classic_form
    : net_type opt_signed opt_range IDENTIFIER opt_port_init        { }
    | K_REG opt_signed opt_range IDENTIFIER opt_port_init           { }
    | K_SIGNED opt_range IDENTIFIER opt_port_init                   { }
    | range IDENTIFIER opt_port_init                                { }
    ;

opt_port_init
    : %empty                                                        { }
    | ASSIGN expression                                             { }
    ;

// What follows `port_direction IDENTIFIER`. If the next token is
// `,` `)` `=` or `[`-then-port-end, the IDENTIFIER was the port
// name. If the next token is another IDENTIFIER (or `[` followed by
// dims and then an IDENTIFIER), the first IDENTIFIER was an
// unscoped user type and the next is the port name.
port_after_dir_ident_tail
    : opt_unpacked_dim_list opt_port_init                           { }
    | opt_packed_dim_list IDENTIFIER opt_unpacked_dim_list
        opt_port_init                                               { }
    ;

// SV port data types — keyword-prefixed only. The classic
// `port_direction opt_net_or_reg opt_signed opt_range IDENTIFIER`
// alternative above already covers `input wire/reg ...` and
// `input <ident>` (port name only). Adding an `IDENTIFIER DCOLON
// IDENTIFIER` alternative here would create a reduce-reduce conflict
// against the old rule (parser cannot tell whether the trailing
// IDENTIFIER is the port name or the start of a scoped type) and
// regresses Verilog-2001 inputs.
sv_port_data_type
    : K_LOGIC opt_signedness opt_packed_dim_list                    { }
    | K_BIT opt_signedness opt_packed_dim_list                      { }
    | K_BYTE opt_signedness                                         { }
    | K_INT opt_signedness                                          { }
    | K_LONGINT opt_signedness                                      { }
    | K_SHORTINT opt_signedness                                     { }
    | K_REAL                                                        { }
    | K_REALTIME                                                    { }
    | K_STRING                                                      { }
    | SCOPED_NAME_HEAD IDENTIFIER opt_packed_dim_list               { }
    ;

port_reference_or_named
    : IDENTIFIER                                                  { }
    | IDENTIFIER LBRACK expression RBRACK                         { }
    | IDENTIFIER LBRACK expression COLON expression RBRACK        { }
    | DOT IDENTIFIER LPAREN RPAREN                                { }
    | DOT IDENTIFIER LPAREN expression RPAREN                     { }
    ;

port_direction
    : K_INPUT                             { }
    | K_OUTPUT                            { }
    | K_INOUT                             { }
    ;

opt_net_or_reg
    : %empty                              { }
    | net_type                            { }
    | K_REG                               { }
    ;

net_type
    : K_WIRE                              { }
    | K_TRI                               { }
    | K_TRI0                              { }
    | K_TRI1                              { }
    | K_TRIAND                            { }
    | K_TRIOR                             { }
    | K_TRIREG                            { }
    | K_WAND                              { }
    | K_WOR                               { }
    | K_UWIRE                             { }
    | K_SUPPLY0                           { }
    | K_SUPPLY1                           { }
    ;

opt_signed
    : %empty                              { }
    | K_SIGNED                            { }
    ;

opt_range
    : %empty                              { }
    | range                               { }
    ;

range
    : LBRACK expression COLON expression RBRACK   { }
    ;

// ============================================================
// Module items
// ============================================================
module_item_list_opt
    : %empty                              { }
    | module_item_list                    { }
    ;

module_item_list
    : module_item                         { }
    | module_item_list module_item        { }
    ;

// `module_item` factors out two MACRO_REF positions that would
// otherwise collide:
//   1. Decoration: `FOO reg X;  — macro stands in for an attribute.
//   2. Instance type: `BAR inst (...);  — macro stands for the
//      module type name.
// Both start with MACRO_REF. The `actual_module_item` alternatives
// all begin with a keyword (input, reg, wire, always, ...), while
// instantiation continues with IDENTIFIER or `#`. That means the
// LALR(1) lookahead is enough to disambiguate cleanly:
//   MACRO_REF KEYWORD ...   → macro_decorations actual_module_item
//   MACRO_REF IDENTIFIER... → gate_or_module_instantiation
//   MACRO_REF MACRO_REF ... → another decoration in the chain
module_item
    : actual_module_item                              { }
    | macro_decorations actual_module_item            { }
    | gate_or_module_instantiation                    { }
    | conditional_generate_construct                  { }
    | loop_generate_construct                         { }
    | case_generate_construct                         { }
    ;

actual_module_item
    : port_decl_stmt                      { }
    | net_declaration                     { }
    | reg_declaration                     { }
    | integer_declaration                 { }
    | real_declaration                    { }
    | time_declaration                    { }
    | event_declaration                   { }
    | genvar_declaration                  { }
    | parameter_declaration               { }
    | localparam_declaration              { }
    | continuous_assign                   { }
    | always_construct                    { }
    | initial_construct                   { }
    | final_construct                     { }
    | function_declaration                { }
    | task_declaration                    { }
    | generate_construct                  { }
    | defparam_statement                  { }
    | sv_data_declaration                 { }
    | sv_typedef_declaration              { }
    | sv_package_import_declaration       { }
    | system_task_enable                  { }
    | dpi_export_declaration              { }
    ;

dpi_export_declaration
    : K_EXPORT STRING_LITERAL K_FUNCTION IDENTIFIER SEMI            { }
    | K_EXPORT STRING_LITERAL K_TASK IDENTIFIER SEMI                { }
    ;

final_construct
    : K_FINAL statement                                             { }
    ;

macro_decorations
    : MACRO_REF                           { }
    | macro_decorations MACRO_REF         { }
    ;

// ============================================================
// Generate constructs (also valid as plain module items per LRM)
// ============================================================
conditional_generate_construct
    : K_IF LPAREN expression RPAREN generate_block_or_null
        %prec IF_NO_ELSE                                            { }
    | K_IF LPAREN expression RPAREN generate_block_or_null
        K_ELSE generate_block_or_null                               { }
    ;

loop_generate_construct
    : K_FOR LPAREN for_init SEMI expression SEMI
            for_step RPAREN generate_block                          { }
    ;

case_generate_construct
    : K_CASE LPAREN expression RPAREN generate_case_items
        K_ENDCASE                                                   { }
    ;

generate_case_items
    : generate_case_item                                            { }
    | generate_case_items generate_case_item                        { }
    ;

generate_case_item
    : expression_list COLON generate_block_or_null                  { }
    | K_DEFAULT COLON generate_block_or_null                        { }
    | K_DEFAULT generate_block_or_null                              { }
    ;

generate_block_or_null
    : SEMI                                                          { }
    | generate_block                                                { }
    ;

generate_block
    : K_BEGIN generate_block_items K_END opt_endlabel               { }
    | K_BEGIN COLON IDENTIFIER generate_block_items K_END opt_endlabel { }
    | module_item                                                   { }
    ;

generate_block_items
    : %empty                                                        { }
    | generate_block_items module_item                              { }
    ;

// ============================================================
// Declarations
// ============================================================
port_decl_stmt
    : port_direction opt_net_or_reg opt_signed opt_range
        identifier_list SEMI                                      { }
    ;

net_declaration
    : net_type opt_signed opt_range list_of_net_decl_assignments
        SEMI                                                      { }
    ;

list_of_net_decl_assignments
    : net_decl_assignment_or_id                                   { }
    | list_of_net_decl_assignments COMMA net_decl_assignment_or_id  { }
    ;

net_decl_assignment_or_id
    : IDENTIFIER opt_range_or_dimensions                          { }
    | IDENTIFIER ASSIGN expression                                { }
    ;

opt_range_or_dimensions
    : %empty                                                      { }
    | range                                                       { }
    ;

reg_declaration
    : K_REG opt_signed opt_range list_of_var_decl_assignments
        SEMI                                                      { }
    ;

list_of_var_decl_assignments
    : var_decl_assignment                                         { }
    | list_of_var_decl_assignments COMMA var_decl_assignment      { }
    ;

var_decl_assignment
    : IDENTIFIER opt_unpacked_dim_list                            { }
    | IDENTIFIER opt_unpacked_dim_list ASSIGN expression          { }
    ;

integer_declaration
    : K_INTEGER list_of_var_decl_assignments SEMI                 { }
    ;

real_declaration
    : K_REAL list_of_var_decl_assignments SEMI                    { }
    | K_REALTIME list_of_var_decl_assignments SEMI                { }
    ;

time_declaration
    : K_TIME list_of_var_decl_assignments SEMI                    { }
    ;

event_declaration
    : K_EVENT identifier_list SEMI                                { }
    ;

genvar_declaration
    : K_GENVAR identifier_list SEMI                               { }
    ;

parameter_declaration
    : K_PARAMETER opt_param_type_or_range param_assignments SEMI  { }
    | K_PARAMETER sv_param_data_type param_assignments SEMI       { }
    | K_PARAMETER K_TYPE param_type_assignments SEMI              { }
    | K_PARAMETER IDENTIFIER pdecl_after_param_ident SEMI         { }
    ;

localparam_declaration
    : K_LOCALPARAM opt_param_type_or_range param_assignments SEMI { }
    | K_LOCALPARAM sv_param_data_type param_assignments SEMI      { }
    | K_LOCALPARAM K_TYPE param_type_assignments SEMI             { }
    | K_LOCALPARAM IDENTIFIER pdecl_after_param_ident SEMI        { }
    ;

// Same disambiguation pattern as `mp_after_param_ident` but for
// statement-level parameter/localparam declarations, which take a
// comma-separated list of assignments rather than just one.
pdecl_after_param_ident
    : opt_unpacked_dim_list ASSIGN expression
        pdecl_more_param_assignments                              { }
    | opt_packed_dim_list IDENTIFIER opt_unpacked_dim_list
        ASSIGN expression pdecl_more_param_assignments            { }
    ;

pdecl_more_param_assignments
    : %empty                                                      { }
    | pdecl_more_param_assignments COMMA param_assignment         { }
    ;

param_type_assignments
    : param_type_assignment                                       { }
    | param_type_assignments COMMA param_type_assignment          { }
    ;

opt_param_type_or_range
    : %empty                              { }
    | K_SIGNED                            { }
    | K_INTEGER                           { }
    | K_REAL                              { }
    | K_REALTIME                          { }
    | K_TIME                              { }
    | range                               { }
    | K_SIGNED range                      { }
    ;

param_assignments
    : param_assignment                                  { }
    | param_assignments COMMA param_assignment          { }
    ;

param_assignment
    : IDENTIFIER opt_unpacked_dim_list ASSIGN expression { }
    ;

identifier_list
    : IDENTIFIER                                        { }
    | identifier_list COMMA IDENTIFIER                  { }
    ;

defparam_statement
    : K_DEFPARAM defparam_assignments SEMI              { }
    ;

defparam_assignments
    : defparam_assign                                   { }
    | defparam_assignments COMMA defparam_assign        { }
    ;

defparam_assign
    : hierarchical_identifier ASSIGN expression         { }
    ;

// ============================================================
// Continuous assignments
// ============================================================
continuous_assign
    : K_ASSIGN list_of_net_assignments SEMI             { }
    ;

list_of_net_assignments
    : net_assignment                                    { }
    | list_of_net_assignments COMMA net_assignment      { }
    ;

net_assignment
    : variable_lvalue ASSIGN expression                 { }
    ;

// ============================================================
// always / initial / generate
// ============================================================
always_construct
    : K_ALWAYS statement                                { }
    | K_ALWAYS_FF statement                             { }
    | K_ALWAYS_COMB statement                           { }
    | K_ALWAYS_LATCH statement                          { }
    ;

initial_construct
    : K_INITIAL statement                               { }
    ;

generate_construct
    : K_GENERATE module_item_list_opt K_ENDGENERATE     { }
    ;

// ============================================================
// Function / task
// ============================================================
function_declaration
    : K_FUNCTION opt_automatic opt_function_range_or_type
        IDENTIFIER SEMI function_item_decls statement_list_opt
        K_ENDFUNCTION opt_endlabel                      { }
    | K_FUNCTION opt_automatic opt_function_range_or_type
        IDENTIFIER LPAREN tf_port_list RPAREN SEMI
        function_item_decls_opt statement_list_opt
        K_ENDFUNCTION opt_endlabel                      { }
    | K_FUNCTION opt_automatic opt_function_range_or_type
        IDENTIFIER LPAREN RPAREN SEMI
        function_item_decls_opt statement_list_opt
        K_ENDFUNCTION opt_endlabel                      { }
    ;

opt_automatic
    : %empty                                            { }
    | K_AUTOMATIC                                       { }
    ;

opt_function_range_or_type
    : %empty                                            { }
    | K_INTEGER                                         { }
    | K_REAL                                            { }
    | K_REALTIME                                        { }
    | K_TIME                                            { }
    | opt_signed range                                  { }
    | K_SIGNED                                          { }
    | sv_param_data_type                                { }
    | K_VOID                                            { }
    ;

function_item_decls_opt
    : %empty                                            { }
    | function_item_decls                               { }
    ;

function_item_decls
    : function_item_decl                                { }
    | function_item_decls function_item_decl            { }
    ;

function_item_decl
    : port_decl_stmt                                    { }
    | reg_declaration                                   { }
    | integer_declaration                               { }
    | real_declaration                                  { }
    | time_declaration                                  { }
    | event_declaration                                 { }
    | parameter_declaration                             { }
    | localparam_declaration                            { }
    | sv_data_declaration                               { }
    | sv_typedef_declaration                            { }
    ;

task_declaration
    : K_TASK opt_automatic IDENTIFIER SEMI
        task_item_decls_opt statement_list_opt K_ENDTASK opt_endlabel { }
    | K_TASK opt_automatic IDENTIFIER LPAREN tf_port_list RPAREN
        SEMI task_item_decls_opt statement_list_opt K_ENDTASK opt_endlabel { }
    | K_TASK opt_automatic IDENTIFIER LPAREN RPAREN
        SEMI task_item_decls_opt statement_list_opt K_ENDTASK opt_endlabel { }
    ;

task_item_decls_opt
    : %empty                                            { }
    | task_item_decls                                   { }
    ;

task_item_decls
    : task_item_decl                                    { }
    | task_item_decls task_item_decl                    { }
    ;

task_item_decl
    : port_decl_stmt                                    { }
    | reg_declaration                                   { }
    | integer_declaration                               { }
    | real_declaration                                  { }
    | time_declaration                                  { }
    | event_declaration                                 { }
    | parameter_declaration                             { }
    | localparam_declaration                            { }
    | sv_data_declaration                               { }
    | sv_typedef_declaration                            { }
    ;

tf_port_list
    : tf_port_decl                                      { }
    | tf_port_list COMMA tf_port_decl                   { }
    ;

tf_port_decl
    : port_direction opt_net_or_reg opt_signed opt_range IDENTIFIER  { }
    | port_direction opt_net_or_reg opt_signed opt_range IDENTIFIER
        ASSIGN expression                                            { }
    | port_direction sv_port_data_type IDENTIFIER opt_unpacked_dim_list  { }
    | port_direction sv_port_data_type IDENTIFIER opt_unpacked_dim_list
        ASSIGN expression                                            { }
    | sv_port_data_type IDENTIFIER opt_unpacked_dim_list             { }
    | sv_port_data_type IDENTIFIER opt_unpacked_dim_list ASSIGN expression { }
    ;

// ============================================================
// Module / gate instantiation
// ============================================================
// `IDENT inst (...);`, `IDENT #(...) inst (...);`, and the SV
// user-type variable declaration `IDENT var;` / `IDENT var = init;`
// share the same `IDENT IDENT ...` prefix. They cannot be split into
// separate rules without LALR(1) reduce-reduce conflicts (the
// classic Verilog-vs-SV ambiguity); we fold them into one
// `inst_or_var_list` rule and let the parser choose per-element
// based on the next token: `(` → instance, `,`/`;`/`=`/`[` → var.
gate_or_module_instantiation
    : module_or_udp_instantiation                       { }
    | gate_instantiation                                { }
    ;

module_or_udp_instantiation
    : IDENTIFIER opt_param_value_assignment opt_packed_dim_list
        inst_or_var_list SEMI                                      { }
    | SCOPED_NAME_HEAD IDENTIFIER opt_param_value_assignment
        opt_packed_dim_list inst_or_var_list SEMI                  { }
    | MACRO_REF  opt_param_value_assignment inst_or_var_list SEMI  { }
    ;

inst_or_var_list
    : inst_or_var                                       { }
    | inst_or_var_list COMMA inst_or_var                { }
    ;

inst_or_var
    : IDENTIFIER LPAREN port_connections_opt RPAREN     { }
    | IDENTIFIER range LPAREN port_connections_opt RPAREN { }
    | IDENTIFIER opt_unpacked_dim_list                  { }
    | IDENTIFIER opt_unpacked_dim_list ASSIGN expression { }
    ;

opt_param_value_assignment
    : %empty                                            { }
    | HASH LPAREN port_connections RPAREN               { }
    | HASH NUMBER                                       { }
    | HASH IDENTIFIER                                   { }
    ;

instance_list
    : instance                                          { }
    | instance_list COMMA instance                      { }
    ;

instance
    : IDENTIFIER opt_range LPAREN port_connections_opt RPAREN  { }
    ;

port_connections_opt
    : %empty                                            { }
    | port_connections                                  { }
    ;

port_connections
    : port_conn                                         { }
    | port_connections COMMA port_conn                  { }
    ;

port_conn
    : expression                                        { }
    | DOT IDENTIFIER LPAREN RPAREN                      { }
    | DOT IDENTIFIER LPAREN expression RPAREN           { }
    | DOT IDENTIFIER                                    { }
    ;

gate_instantiation
    : gate_type gate_instance_list SEMI                 { }
    ;

gate_type
    : K_AND                                             { }
    | K_OR                                              { }
    | K_NAND                                            { }
    | K_NOR                                             { }
    | K_XOR                                             { }
    | K_XNOR                                            { }
    | K_BUF                                             { }
    | K_NOT                                             { }
    | K_BUFIF0                                          { }
    | K_BUFIF1                                          { }
    | K_NOTIF0                                          { }
    | K_NOTIF1                                          { }
    | K_NMOS                                            { }
    | K_PMOS                                            { }
    | K_RNMOS                                           { }
    | K_RPMOS                                           { }
    | K_CMOS                                            { }
    | K_RCMOS                                           { }
    | K_TRAN                                            { }
    | K_TRANIF0                                         { }
    | K_TRANIF1                                         { }
    | K_RTRAN                                           { }
    | K_RTRANIF0                                        { }
    | K_RTRANIF1                                        { }
    | K_PULLUP                                          { }
    | K_PULLDOWN                                        { }
    ;

gate_instance_list
    : gate_instance                                     { }
    | gate_instance_list COMMA gate_instance            { }
    ;

gate_instance
    : LPAREN port_connections RPAREN                    { }
    | IDENTIFIER opt_range LPAREN port_connections RPAREN { }
    ;

// ============================================================
// Statements
// ============================================================
statement_or_null
    : statement                                         { }
    | SEMI                                              { }
    ;

statement_list
    : statement                                         { }
    | statement_list statement                          { }
    ;

statement_list_opt
    : %empty                                            { }
    | statement_list                                    { }
    ;

statement
    : blocking_assignment SEMI                          { }
    | nonblocking_assignment SEMI                       { }
    | conditional_statement                             { }
    | case_statement                                    { }
    | loop_statement                                    { }
    | sequential_block                                  { }
    | system_task_enable                                { }
    | task_enable                                       { }
    | event_trigger                                     { }
    | wait_statement                                    { }
    | procedural_continuous_assignment                  { }
    | disable_statement                                 { }
    | event_control statement_or_null                   { }
    | delay_control statement_or_null                   { }
    | jump_statement                                    { }
    | inc_or_dec_expression SEMI                        { }
    | void_call_statement                               { }
    ;

// `void'(tf_call)` used as a statement to discard a function's
// return value (Ibex's tracer does this with `$value$plusargs`).
void_call_statement
    : K_VOID APOST_LPAREN expression RPAREN SEMI                        { }
    ;

jump_statement
    : K_RETURN SEMI                                     { }
    | K_RETURN expression SEMI                          { }
    | K_BREAK SEMI                                      { }
    | K_CONTINUE SEMI                                   { }
    ;

blocking_assignment
    : variable_lvalue ASSIGN expression                                 { }
    | variable_lvalue ASSIGN delay_or_event_control expression          { }
    | variable_lvalue compound_assign_op expression                     { }
    ;

compound_assign_op
    : PLUS_EQ                                                           { }
    | MINUS_EQ                                                          { }
    | STAR_EQ                                                           { }
    | SLASH_EQ                                                          { }
    | PERCENT_EQ                                                        { }
    | AMP_EQ                                                            { }
    | PIPE_EQ                                                           { }
    | CARET_EQ                                                          { }
    | LSHIFT_EQ                                                         { }
    | RSHIFT_EQ                                                         { }
    | LSHIFTA_EQ                                                        { }
    | RSHIFTA_EQ                                                        { }
    ;

nonblocking_assignment
    : variable_lvalue LE expression                                     { }
    | variable_lvalue LE delay_or_event_control expression              { }
    ;

variable_lvalue
    : hierarchical_identifier_with_select                               { }
    | LBRACE variable_lvalue_list RBRACE                                { }
    ;

variable_lvalue_list
    : variable_lvalue                                                   { }
    | variable_lvalue_list COMMA variable_lvalue                        { }
    ;

hierarchical_identifier
    : IDENTIFIER                                                        { }
    | hierarchical_identifier DOT IDENTIFIER                            { }
    | SCOPED_NAME_HEAD IDENTIFIER                                       { }
    ;

hierarchical_identifier_with_select
    : hierarchical_identifier                                           { }
    | hierarchical_identifier_with_select LBRACK expression RBRACK      { }
    | hierarchical_identifier_with_select LBRACK expression COLON expression RBRACK         { }
    | hierarchical_identifier_with_select LBRACK expression PLUS_COLON expression RBRACK    { }
    | hierarchical_identifier_with_select LBRACK expression MINUS_COLON expression RBRACK   { }
    | hierarchical_identifier_with_select DOT IDENTIFIER                { }
    ;

conditional_statement
    : K_IF LPAREN expression RPAREN statement_or_null %prec IF_NO_ELSE        { }
    | K_IF LPAREN expression RPAREN statement_or_null K_ELSE statement_or_null { }
    | case_priority K_IF LPAREN expression RPAREN statement_or_null %prec IF_NO_ELSE { }
    | case_priority K_IF LPAREN expression RPAREN statement_or_null K_ELSE statement_or_null { }
    ;

case_statement
    : case_keyword LPAREN expression RPAREN case_items K_ENDCASE        { }
    | case_priority case_keyword LPAREN expression RPAREN case_items K_ENDCASE { }
    ;

case_priority
    : K_UNIQUE                                                          { }
    | K_UNIQUE0                                                         { }
    | K_PRIORITY                                                        { }
    ;

case_keyword
    : K_CASE                                                            { }
    | K_CASEX                                                           { }
    | K_CASEZ                                                           { }
    ;

case_items
    : case_item                                                         { }
    | case_items case_item                                              { }
    ;

case_item
    : expression_list COLON statement_or_null                           { }
    | K_DEFAULT COLON statement_or_null                                 { }
    | K_DEFAULT statement_or_null                                       { }
    ;

loop_statement
    : K_FOREVER statement                                               { }
    | K_REPEAT LPAREN expression RPAREN statement                       { }
    | K_WHILE LPAREN expression RPAREN statement                        { }
    | K_FOR LPAREN for_init SEMI expression SEMI
            for_step RPAREN statement                                   { }
    | K_DO statement K_WHILE LPAREN expression RPAREN SEMI              { }
    ;

for_init
    : blocking_assignment                                               { }
    | data_type IDENTIFIER ASSIGN expression                            { }
    | K_GENVAR IDENTIFIER ASSIGN expression                             { }
    ;

for_step
    : blocking_assignment                                               { }
    | inc_or_dec_expression                                             { }
    ;

inc_or_dec_expression
    : variable_lvalue INC                                               { }
    | variable_lvalue DEC                                               { }
    | INC variable_lvalue                                               { }
    | DEC variable_lvalue                                               { }
    ;

sequential_block
    : K_BEGIN block_item_decls_opt statement_list_opt K_END opt_endlabel { }
    | K_BEGIN COLON IDENTIFIER block_item_decls_opt
        statement_list_opt K_END opt_endlabel                           { }
    ;

block_item_decls_opt
    : %empty                                                            { }
    | block_item_decls                                                  { }
    ;

block_item_decls
    : block_item_decl                                                   { }
    | block_item_decls block_item_decl                                  { }
    ;

block_item_decl
    : reg_declaration                                                   { }
    | integer_declaration                                               { }
    | real_declaration                                                  { }
    | time_declaration                                                  { }
    | event_declaration                                                 { }
    | parameter_declaration                                             { }
    | localparam_declaration                                            { }
    | sv_data_declaration                                               { }
    | sv_typedef_declaration                                            { }
    ;

system_task_enable
    : SYSTEM_ID SEMI                                                    { }
    | SYSTEM_ID LPAREN expression_list_opt RPAREN SEMI                  { }
    ;

task_enable
    : hierarchical_identifier SEMI                                      { }
    | hierarchical_identifier LPAREN expression_list_opt RPAREN SEMI    { }
    ;

event_trigger
    : ARROW hierarchical_identifier SEMI                                { }
    ;

wait_statement
    : K_WAIT LPAREN expression RPAREN statement_or_null                 { }
    ;

disable_statement
    : K_DISABLE hierarchical_identifier SEMI                            { }
    ;

procedural_continuous_assignment
    : K_ASSIGN variable_lvalue ASSIGN expression SEMI                   { }
    | K_DEASSIGN variable_lvalue SEMI                                   { }
    | K_FORCE variable_lvalue ASSIGN expression SEMI                    { }
    | K_RELEASE variable_lvalue SEMI                                    { }
    ;

event_control
    : AT hierarchical_identifier                                        { }
    | AT LPAREN event_expression RPAREN                                 { }
    | AT STAR                                                           { }
    | AT LPAREN STAR RPAREN                                             { }
    ;

event_expression
    : event_term                                                        { }
    | event_expression K_OR event_term                                  { }
    | event_expression COMMA event_term                                 { }
    ;

event_term
    : expression                                                        { }
    | K_POSEDGE expression                                              { }
    | K_NEGEDGE expression                                              { }
    ;

delay_control
    : HASH NUMBER                                                       { }
    | HASH REAL_NUMBER                                                  { }
    | HASH IDENTIFIER                                                   { }
    | HASH LPAREN expression RPAREN                                     { }
    ;

delay_or_event_control
    : delay_control                                                     { }
    | event_control                                                     { }
    | K_REPEAT LPAREN expression RPAREN event_control                   { }
    ;

// ============================================================
// Expressions
// ============================================================
expression_list
    : expression                                                        { }
    | expression_list COMMA expression                                  { }
    ;

expression_list_opt
    : %empty                                                            { }
    | expression_list                                                   { }
    ;

expression
    : primary                                                           { }
    | unary_op primary  %prec UNARY_PREC                                { }
    | expression PLUS expression                                        { }
    | expression MINUS expression                                       { }
    | expression STAR expression                                        { }
    | expression SLASH expression                                       { }
    | expression PERCENT expression                                     { }
    | expression POWER expression                                       { }
    | expression LT expression                                          { }
    | expression LE expression                                          { }
    | expression GT expression                                          { }
    | expression GE expression                                          { }
    | expression EQEQ expression                                        { }
    | expression NEQ expression                                         { }
    | expression CASEEQ expression                                      { }
    | expression CASENEQ expression                                     { }
    | expression LOGAND expression                                      { }
    | expression LOGOR expression                                       { }
    | expression AMP expression                                         { }
    | expression PIPE expression                                        { }
    | expression CARET expression                                       { }
    | expression NAND_OP expression                                     { }
    | expression NOR_OP expression                                      { }
    | expression XNOR_OP expression                                     { }
    | expression LSHIFT expression                                      { }
    | expression RSHIFT expression                                      { }
    | expression LSHIFTA expression                                     { }
    | expression RSHIFTA expression                                     { }
    | expression QUESTION expression COLON expression                   { }
    | expression K_INSIDE LBRACE inside_value_list RBRACE               { }
    ;

inside_value_list
    : inside_value                                                      { }
    | inside_value_list COMMA inside_value                              { }
    ;

inside_value
    : expression                                                        { }
    | LBRACK expression COLON expression RBRACK                         { }
    ;

unary_op
    : PLUS                                                              { }
    | MINUS                                                             { }
    | BANG                                                              { }
    | TILDE                                                             { }
    | AMP                                                               { }
    | NAND_OP                                                           { }
    | PIPE                                                              { }
    | NOR_OP                                                            { }
    | CARET                                                             { }
    | XNOR_OP                                                           { }
    ;

primary
    : NUMBER                                                            { }
    | REAL_NUMBER                                                       { }
    | STRING_LITERAL                                                    { }
    | hierarchical_identifier_with_select                               { }
    | concatenation                                                     { }
    | multiple_concatenation                                            { }
    | LPAREN expression RPAREN                                          { }
    | function_call                                                     { }
    | system_function_call                                              { }
    | MACRO_REF                                                         { }
    | K_NULL                                                            { }
    | sv_assignment_pattern                                             { }
    | cast_expression                                                   { }
    ;

// SV assignment pattern: '{a, b}, '{key: val, ...}, '{default: val}.
sv_assignment_pattern
    : APOST_LBRACE assignment_pattern_items RBRACE                      { }
    ;

assignment_pattern_items
    : assignment_pattern_item                                           { }
    | assignment_pattern_items COMMA assignment_pattern_item            { }
    ;

assignment_pattern_item
    : expression                                                        { }
    | expression COLON expression                                       { }
    | K_DEFAULT COLON expression                                        { }
    ;

// SV cast: <size_or_type>'(expr). The lexer emits APOST_LPAREN for
// the `'(` so it doesn't get confused with a sized literal. The LHS
// is restricted to a `primary` (number, paren-expr, identifier,
// system call) — that covers Ibex's casts (`Width'(x)`,
// `opcode_e'(x)`, `32'(x)`).
cast_expression
    : primary APOST_LPAREN expression RPAREN                            { }
    | integer_atom_type APOST_LPAREN expression RPAREN                  { }
    | integer_vector_type APOST_LPAREN expression RPAREN                { }
    | K_SIGNED APOST_LPAREN expression RPAREN                           { }
    | K_UNSIGNED APOST_LPAREN expression RPAREN                         { }
    | K_VOID APOST_LPAREN expression RPAREN                             { }
    ;

concatenation
    : LBRACE expression_list RBRACE                                     { }
    ;

multiple_concatenation
    : LBRACE expression LBRACE expression_list RBRACE RBRACE            { }
    ;

function_call
    : hierarchical_identifier LPAREN function_args RPAREN               { }
    | hierarchical_identifier LPAREN RPAREN                             { }
    ;

function_args
    : expression_list                                                   { }
    | named_arg_list                                                    { }
    ;

named_arg_list
    : named_arg                                                         { }
    | named_arg_list COMMA named_arg                                    { }
    ;

named_arg
    : DOT IDENTIFIER LPAREN RPAREN                                      { }
    | DOT IDENTIFIER LPAREN expression RPAREN                           { }
    ;

system_function_call
    : SYSTEM_ID                                                         { }
    | SYSTEM_ID LPAREN expression_list_opt RPAREN                       { }
    ;

%%

void stargate::Parser::error(const location_type& loc,
                             const std::string& msg) {
    drv.addError(loc, msg);
}
