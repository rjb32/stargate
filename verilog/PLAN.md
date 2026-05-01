# Verilog Parser — Plan

A flex/bison-based Verilog parser in C++20, integrated into stargate.
This document is the design plan; no AST is implemented in v1 — every
bison action body is empty.

## 1. Scope

**Target language:** Verilog-2001 / IEEE 1364-2005, plus a pragmatic
SystemVerilog (IEEE 1800) subset.

**Concrete coverage benchmark.** The lowRISC/ibex regress
(`regress/designs/ibex/`) parses the synthesizable rtl/ tree end-to-
end — 30 of 30 `.sv` files. That is *not* the full Ibex repository;
across the whole pinned commit (644 SV/SVH files total) coverage is
roughly: rtl/ 30/30, vendor/ 184/452, dv/ 9/135, formal/ 1/20,
shared/ 5/6. The verification (`dv/`) and formal (`formal/`)
directories are mostly out of reach because they rely on SV class /
constraint / randomization / assertion features that this parser
does not accept (see "Out of scope" below). The 30/30 in `rtl/`
also depends on three OpenTitan headers being stubbed empty so
their assertion macros expand to nothing — the macro *bodies* would
not parse.

**SystemVerilog subset accepted** (enable per file by extension —
`.sv`/`.svh` turn on SV keywords, `.v`/`.vh` keep Verilog-2001):
- `package ... endpackage`, `import pkg::*;`
- `typedef` of struct/union/enum/vector
- Data types: `logic`, `bit`, `byte`, `int`, `longint`, `shortint`,
  `string`, `void`, with packed dimensions and `signed`/`unsigned`
- ANSI port lists with typed direction, including scoped types (`pkg::T`)
- Typed parameters (incl. `parameter type T = ...`)
- `always_ff`, `always_comb`, `always_latch`
- `unique`/`unique0`/`priority` on `case` and `if`
- Scope resolution `pkg::IDENT` in expressions and types
- Unsized literals (`'0`, `'1`, `'x`, `'z`), assignment patterns
  `'{...}`, casts `<type>'(expr)` including `void'(expr)` as a
  statement
- `for (int/genvar i = 0; ...; i++)`, `do`/`while`, post/pre
  increment, compound assignments
- `inside` operator, `bus[i].field` member access on selects
- Inline `module foo import pkg::*; #(...)` headers
- DPI export `export "DPI-C" function name;`

**Out of scope (deliberately):**
- AST construction. Every bison action body is `{ }`.
- Compiler directives (`` `define ``, `` `include ``, `` `ifdef ``, ...).
  These are handled by the preprocessor pass; the lexer sees only
  post-preprocessing input.
- SV classes (`class`/`endclass`, `extends`, `new`, `this`, `super`),
  randomization (`rand`/`randc`/`randomize`), constraints
  (`constraint`/`solve`/`with`), covergroups
  (`covergroup`/`coverpoint`/`bins`/`cross`), properties/sequences/
  `assert property`, clocking blocks, full `interface` bodies with
  `modport`s used as port types. Ibex's macro-based `` `ASSERT* ``
  calls vanish through an empty `prim_assert.sv` stub; the real
  expansions are SV property syntax this parser does not accept.

## 2. Grammar sourcing

**Primary source (authoritative):**
- IEEE 1364-2005, **Annex A** (formal BNF) and **Annex B** (keyword
  list). The BNF text describes a public standard and is republished
  freely. This is what gets transcribed into `Parser.y`.

**Cross-references — for tricky lexer corners only, no code copying:**
- **Icarus Verilog** `parse.y` / `lexor.lex` (steveicarus/iverilog,
  GPLv2) — battle-tested edge cases for numbers, strings, attribute
  instances, escaped identifiers.
- **Verilog-Perl** `VParseGrammar.y` (Wilson Snyder, LGPL) — clean
  shift/reduce conflict resolution notes.

These are consulted only to verify our rules. Nothing is pasted —
the file stays clean-room and unencumbered.

## 3. Dependencies

**System tools, not vendored:**
- **bison >= 3.5** — required for the modern C++ skeleton
  (`%skeleton "lalr1.cc"`, variant-based `%define api.value.type
  variant`, `api.token.constructor`).
- **flex >= 2.6**.

Both are mature and ubiquitous. Vendoring them as submodules adds no
value.

**install_build_tools.sh additions:**
- macOS: `brew install bison flex`. Both are keg-only, so their `bin`
  directories must be exported and `BISON_EXECUTABLE` /
  `FLEX_EXECUTABLE` written into `CMAKE_ARGS` via
  `external/dependencies/macos_setenv.sh` (same mechanism already used
  for LLVM). System bison is 2.3 — too old.
- apt: `sudo apt-get install -y bison flex`.

**CMake side:** `find_package(BISON 3.5 REQUIRED)` +
`find_package(FLEX REQUIRED)` + `BISON_TARGET` / `FLEX_TARGET` /
`ADD_FLEX_BISON_DEPENDENCY`.

No new submodule under `external/`.

## 4. Layout

A new top-level library, mirroring `common/`, `project/`, `stargate/`:

```
verilog/
    CMakeLists.txt
    Lexer.l                 # flex source — reentrant
    Parser.y                # bison source — %skeleton "lalr1.cc"
    VerilogParser.h         # thin C++ facade (parseFile / parseString)
    VerilogParser.cpp
    VerilogDriver.h         # holds lexer state + error list
    VerilogDriver.cpp
    ParseException.h        # derives TuringException

tools/
    sgcparse/               # smoke-test CLI
        CMakeLists.txt
        SgcParse.cpp

regress/
    verilog_parse/          # tiny .v files the CLI must accept
```

Top-level `CMakeLists.txt` gets one new `add_subdirectory(verilog)`
between `common` and `project`.

## 5. Lexer (Lexer.l)

**Configuration:**
- Keep flex in C-mode (no `%option c++` — flex's C++ mode is
  ill-maintained).
- `%option reentrant`, `%option bison-bridge bison-locations`,
  `%option noyywrap`.
- Bison's C++ skeleton calls a hand-written
  `yylex(yy::parser::value_type*, yy::location*, VerilogDriver&)` that
  wraps the reentrant flex scanner. This is the modern idiomatic
  combo and avoids `FlexLexer.h`.
- Track `yylloc` for line and column.

**Token classes:**
- Keywords (Annex B, ~120 total): `module`, `endmodule`, `input`,
  `output`, `inout`, `wire`, `reg`, `always`, `assign`, `begin`,
  `end`, `if`, `else`, `case`, `casez`, `casex`, `endcase`, `for`,
  `while`, `repeat`, `forever`, `function`, `endfunction`, `task`,
  `endtask`, `parameter`, `localparam`, `generate`, `endgenerate`,
  `posedge`, `negedge`, `initial`, `default`, `integer`, `real`,
  `time`, `signed`, `unsigned`, `genvar`, `specify`, `endspecify`,
  `primitive`, `endprimitive`, ...
- Identifiers: simple `[a-zA-Z_][a-zA-Z0-9_$]*` and **escaped
  identifiers** `\\[^ \t\n]+` (backslash to whitespace).
- Numbers: sized literals `<size>'<base><digits>` for `'b 'o 'd 'h`
  with `_` separators and `x`/`z` digits, plain integers, real numbers
  (`1.5`, `1e3`, `1.5e-3`).
- Strings: `"..."` with the standard escapes.
- Operators / punctuation: `<=`, `>=`, `==`, `!=`, `===`, `!==`,
  `&&`, `||`, `**`, `<<`, `>>`, `<<<`, `>>>`, `->`, `=>`, `*>`, plus
  singles.
- System tasks/functions: `\$[a-zA-Z_][a-zA-Z0-9_$]*` (`$display`,
  `$finish`, `$time`, ...).
- Attribute instances `(* ... *)` — recognized and skipped in v1
  (returned as a single token the parser ignores), to avoid context
  dependency.
- Comments: `//...\n` and `/* ... */`, skipped.
- Compiler directives: backtick-prefixed line — skipped to end of line
  for now, with a TODO comment.
- Whitespace: skipped.

## 6. Parser (Parser.y)

**Configuration:**
```
%language "c++"
%skeleton "lalr1.cc"
%require "3.5"
%define api.value.type variant
%define api.token.constructor
%define parse.error verbose
%define parse.trace
%locations
%param { VerilogDriver& drv }
```

**Top-level structure (mirroring Annex A):**
- `source_text` -> `{ description }` where `description` is
  `module_declaration | udp_declaration` (UDPs can be a stub for v1).
- `module_declaration` -> header + `{ module_item } endmodule`.
- `module_item` -> port decl, net decl, reg decl, parameter decl,
  `always` / `initial` / `assign`, instance, generate, function/task,
  ...
- `statement` -> blocking/non-blocking assign, `if`, `case`, `for`,
  `while`, `repeat`, `forever`, block (`begin..end`), event control,
  sys-task call, ...
- `expression` -> standard operator precedence: ternary, `||`, `&&`,
  `|`, `^`, `&`, `==/!=`, comparisons, shifts, `+/-`, `*/%`, `**`,
  unary, primary.
- Operator precedence: declared via `%left` / `%right` / `%nonassoc`
  per LRM Table 5-4 — this resolves the bulk of the shift/reduce
  conflicts cleanly.

**Every action body is `{}`.** Tokens carrying useful payloads
(identifiers, numeric literals, strings) are typed with
`%token <std::string>` so the wiring is in place when an AST lands
later — but rules do not construct anything yet.

**Known sticky conflicts** (flag in comments, resolve with
precedence):
- `if`/`else` dangling-else -> `%nonassoc IF_NO_ELSE` < `%nonassoc
  ELSE`.
- Function call vs. array-index in expressions — both are `id ( ... )`
  vs `id [ ... ]` — handled by separate primary alternatives.
- Block name (`begin : name`) ambiguity — single shift/reduce,
  accepted.

Goal: zero conflicts on a clean v1 build.

## 7. C++ wiring

```cpp
namespace stargate {

class VerilogDriver {
public:
    VerilogDriver();
    ~VerilogDriver();

    int parseFile(const std::string& path);     // 0 = OK
    int parseString(const std::string& source);

    const std::vector<std::string>& errors() const { return _errors; }

    void* scanner() { return _scanner; }
    void addError(const std::string& msg);

private:
    void* _scanner {nullptr};        // yyscan_t (opaque)
    std::vector<std::string> _errors;
    std::string _filename;

    void initScanner();
    void destroyScanner();
};

}
```

Plus `ParseException : public TuringException`, used only by the CLI
when exception-on-error behavior is wanted. The driver itself
accumulates errors and returns a code, which is the more useful API.

## 8. CLI (tools/sgcparse)

```
sgcparse file.v [file.v ...]
```
- Parses each file, prints errors to stderr, exits 1 on first
  failure.
- Optional `--trace` to enable bison's debug trace.
- Driven by `regress/verilog_parse` to assert acceptance of a small
  Verilog corpus.

## 9. Build integration

`verilog/CMakeLists.txt`:
```cmake
find_package(BISON 3.5 REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(VerilogParser Parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/Parser.h
    COMPILE_FLAGS "-Wall")

FLEX_TARGET(VerilogLexer Lexer.l
    ${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp)

ADD_FLEX_BISON_DEPENDENCY(VerilogLexer VerilogParser)

add_library(sgc_verilog_s STATIC
    VerilogDriver.cpp
    VerilogParser.cpp
    ${BISON_VerilogParser_OUTPUTS}
    ${FLEX_VerilogLexer_OUTPUTS})

target_include_directories(sgc_verilog_s PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(sgc_verilog_s PUBLIC sgc_common_s)
```

The repo's `-Wall -Wextra -Werror` will fight bison/flex generated
code (unused-function, sign-compare). Suppress warnings on **the
generated translation units only** with
`set_source_files_properties(... COMPILE_FLAGS -w)` rather than
weakening the project-wide flags.

## 10. Regress

Drop a few `.v` files in `regress/verilog_parse/`:
- `empty_module.v`
- `gates.v` (basic primitives + `assign`)
- `flop.v` (`always @(posedge clk)` with `<=`)
- `case_stmt.v`
- `params.v` (`parameter`, `localparam`)
- `function_task.v`

Each must parse to exit 0. A `verilog_parse_fail.v` with a bad token
must exit 1.

## 11. Order of work

1. `install_build_tools.sh` updates (bison/flex on macOS + linux).
2. `verilog/` skeleton: empty driver + minimal `Parser.y` + `Lexer.l`
   parsing just `module foo; endmodule`. Wire CMake. Confirm clean
   build on macOS.
3. Grow the lexer to the full token set, with tests in regress.
4. Grow parser rules (Annex A walk, top-down: `source_text` ->
   `module_item` -> `statement` -> `expression`). Empty actions
   throughout. Resolve precedence; aim for zero conflicts.
5. Run all regress targets.
