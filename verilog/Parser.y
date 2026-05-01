// Verilog-2001 (IEEE 1364-2005) parser, transcribed from Annex A.
// Recognizer-only: every semantic action body is empty. An AST will
// land in a follow-up.

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

// ============================================================
// Keywords (alphabetical)
// ============================================================
%token K_ALWAYS         "always"
%token K_AND            "and"
%token K_ASSIGN         "assign"
%token K_AUTOMATIC      "automatic"
%token K_BEGIN          "begin"
%token K_BUF            "buf"
%token K_BUFIF0         "bufif0"
%token K_BUFIF1         "bufif1"
%token K_CASE           "case"
%token K_CASEX          "casex"
%token K_CASEZ          "casez"
%token K_CMOS           "cmos"
%token K_DEASSIGN       "deassign"
%token K_DEFAULT        "default"
%token K_DEFPARAM       "defparam"
%token K_DISABLE        "disable"
%token K_EDGE           "edge"
%token K_ELSE           "else"
%token K_END            "end"
%token K_ENDCASE        "endcase"
%token K_ENDFUNCTION    "endfunction"
%token K_ENDGENERATE    "endgenerate"
%token K_ENDMODULE      "endmodule"
%token K_ENDPRIMITIVE   "endprimitive"
%token K_ENDSPECIFY     "endspecify"
%token K_ENDTABLE       "endtable"
%token K_ENDTASK        "endtask"
%token K_EVENT          "event"
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
%token K_IFNONE         "ifnone"
%token K_INITIAL        "initial"
%token K_INOUT          "inout"
%token K_INPUT          "input"
%token K_INTEGER        "integer"
%token K_JOIN           "join"
%token K_LARGE          "large"
%token K_LOCALPARAM     "localparam"
%token K_MACROMODULE    "macromodule"
%token K_MEDIUM         "medium"
%token K_MODULE         "module"
%token K_NAND           "nand"
%token K_NEGEDGE        "negedge"
%token K_NMOS           "nmos"
%token K_NOR            "nor"
%token K_NOT            "not"
%token K_NOTIF0         "notif0"
%token K_NOTIF1         "notif1"
%token K_OR             "or"
%token K_OUTPUT         "output"
%token K_PARAMETER      "parameter"
%token K_PMOS           "pmos"
%token K_POSEDGE        "posedge"
%token K_PRIMITIVE      "primitive"
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
%token K_RNMOS          "rnmos"
%token K_RPMOS          "rpmos"
%token K_RTRAN          "rtran"
%token K_RTRANIF0       "rtranif0"
%token K_RTRANIF1       "rtranif1"
%token K_SCALARED       "scalared"
%token K_SIGNED         "signed"
%token K_SMALL          "small"
%token K_SPECIFY        "specify"
%token K_SPECPARAM      "specparam"
%token K_STRONG0        "strong0"
%token K_STRONG1        "strong1"
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
%token K_UNSIGNED       "unsigned"
%token K_UWIRE          "uwire"
%token K_VECTORED       "vectored"
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
    ;

// ============================================================
// Module declaration
// ============================================================
module_declaration
    : module_keyword IDENTIFIER opt_module_parameter_port_list
        opt_list_of_ports SEMI module_item_list_opt K_ENDMODULE   { }
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

module_parameter_port_decl
    : K_PARAMETER opt_param_type_or_range param_assignment        { }
    | param_assignment                                            { }
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

port_item
    : port_reference_or_named             { }
    | port_direction opt_net_or_reg opt_signed opt_range IDENTIFIER  { }
    | port_direction opt_net_or_reg opt_signed opt_range IDENTIFIER
        ASSIGN expression                                            { }
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
    | function_declaration                { }
    | task_declaration                    { }
    | generate_construct                  { }
    | defparam_statement                  { }
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
    : K_FOR LPAREN blocking_assignment SEMI expression SEMI
            blocking_assignment RPAREN generate_block               { }
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
    : K_BEGIN generate_block_items K_END                            { }
    | K_BEGIN COLON IDENTIFIER generate_block_items K_END           { }
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
    : IDENTIFIER                                                  { }
    | IDENTIFIER range                                            { }
    | IDENTIFIER ASSIGN expression                                { }
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
    ;

localparam_declaration
    : K_LOCALPARAM opt_param_type_or_range param_assignments SEMI { }
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
    : IDENTIFIER ASSIGN expression                      { }
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
        IDENTIFIER SEMI function_item_decls statement_or_null
        K_ENDFUNCTION                                   { }
    | K_FUNCTION opt_automatic opt_function_range_or_type
        IDENTIFIER LPAREN tf_port_list RPAREN SEMI
        function_item_decls_opt statement_or_null
        K_ENDFUNCTION                                   { }
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
    ;

task_declaration
    : K_TASK opt_automatic IDENTIFIER SEMI
        task_item_decls_opt statement_or_null K_ENDTASK { }
    | K_TASK opt_automatic IDENTIFIER LPAREN tf_port_list RPAREN
        SEMI task_item_decls_opt statement_or_null K_ENDTASK  { }
    | K_TASK opt_automatic IDENTIFIER LPAREN RPAREN
        SEMI task_item_decls_opt statement_or_null K_ENDTASK  { }
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
    ;

tf_port_list
    : tf_port_decl                                      { }
    | tf_port_list COMMA tf_port_decl                   { }
    ;

tf_port_decl
    : port_direction opt_net_or_reg opt_signed opt_range IDENTIFIER  { }
    | port_direction opt_net_or_reg opt_signed opt_range IDENTIFIER
        ASSIGN expression                                            { }
    ;

// ============================================================
// Module / gate instantiation
// ============================================================
gate_or_module_instantiation
    : module_or_udp_instantiation                       { }
    | gate_instantiation                                { }
    ;

module_or_udp_instantiation
    : IDENTIFIER opt_param_value_assignment instance_list SEMI  { }
    | MACRO_REF  opt_param_value_assignment instance_list SEMI  { }
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
    ;

blocking_assignment
    : variable_lvalue ASSIGN expression                                 { }
    | variable_lvalue ASSIGN delay_or_event_control expression          { }
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
    ;

hierarchical_identifier_with_select
    : hierarchical_identifier                                           { }
    | hierarchical_identifier_with_select LBRACK expression RBRACK      { }
    | hierarchical_identifier_with_select LBRACK expression COLON expression RBRACK         { }
    | hierarchical_identifier_with_select LBRACK expression PLUS_COLON expression RBRACK    { }
    | hierarchical_identifier_with_select LBRACK expression MINUS_COLON expression RBRACK   { }
    ;

conditional_statement
    : K_IF LPAREN expression RPAREN statement_or_null %prec IF_NO_ELSE        { }
    | K_IF LPAREN expression RPAREN statement_or_null K_ELSE statement_or_null { }
    ;

case_statement
    : case_keyword LPAREN expression RPAREN case_items K_ENDCASE        { }
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
    | K_FOR LPAREN blocking_assignment SEMI expression SEMI
            blocking_assignment RPAREN statement                        { }
    ;

sequential_block
    : K_BEGIN block_item_decls_opt statement_list_opt K_END             { }
    | K_BEGIN COLON IDENTIFIER block_item_decls_opt
        statement_list_opt K_END                                        { }
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
    ;

concatenation
    : LBRACE expression_list RBRACE                                     { }
    ;

multiple_concatenation
    : LBRACE expression LBRACE expression_list RBRACE RBRACE            { }
    ;

function_call
    : hierarchical_identifier LPAREN expression_list RPAREN             { }
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
