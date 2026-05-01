# Parse Limitations

The complete list of files in the regress design suite that sgcparse cannot
parse, grouped by root cause. Each entry is a path relative to that
regress's `src/` cache after the upstream tarball/repo is fetched.

Categories are split into:

- **sgcparse limitation** — the construct is legal Verilog the parser does
  not yet accept. Fixing sgcparse should remove the entry.
- **upstream issue** — the source itself is malformed or carries a hard-
  coded environmental assumption (e.g. an absolute include path on the
  original author's machine). Fixing sgcparse will not help.

## Summary

| Category | Files | Source |
| --- | --: | --- |
| Continuous-assign with delay (`assign #N a = b`) | 54 | iwls2005 |
| `specify` / `endspecify` timing blocks | 2 | iwls2005 |
| Intra-assignment delay via macro (`<= #` `` ` ``MACRO) | 2 | iwls2005 |
| `wire`/`logic` packed multi-dimensional declarations | 1 | mips_neelkshah |
| `timescale.v` missing from include path | 2 | iwls2005 |
| Trailing comma in module port list | 1 | iwls2005 |
| `always (@event)` (parens around event control) | 6 | mips_neelkshah |
| Hard-coded absolute include path | 2 | iwls2005, ddr2_controller |
| **Total** | **70** | |

The lowRISC/ibex synthesizable rtl/ tree — 30 `.sv` files — parses
end-to-end and is no longer counted here. That is the synthesizable
core, not the full Ibex repository: the rest of the tree (vendored
OpenTitan IP, UVM verification, formal harness) is described in
"Ibex coverage" below.

---

## sgcparse limitations

### Continuous-assign with delay — 54 files

The parser rejects a `#delay` between `assign` and the LHS of a continuous
assignment, e.g. `assign #1 q = sel ? a : b;`. The same form appears
inside `always` blocks as the intra-assignment delay `q <= #1 rhs;` which
the parser accepts when the delay is a literal number. The error is
`syntax error, unexpected #, expecting { or IDENTIFIER`.

iwls2005:

- `faraday/rtl/DSP/hdl/BDMA/m_bdma.v`
- `faraday/rtl/DSP/hdl/BTB/c_btb.v`
- `faraday/rtl/DSP/hdl/DAG/d_adder15.v`
- `faraday/rtl/DSP/hdl/DAG/d_cla15.v`
- `faraday/rtl/DSP/hdl/DAG/d_ILM1reg.v`
- `faraday/rtl/DSP/hdl/DAG/d_ILM2reg.v`
- `faraday/rtl/DSP/hdl/DAG/d_lpn.v`
- `faraday/rtl/DSP/hdl/DAG/d_moduloL1.v`
- `faraday/rtl/DSP/hdl/DAG/d_moduloL2.v`
- `faraday/rtl/DSP/hdl/DEC/c_dec.v`
- `faraday/rtl/DSP/hdl/EU/ALU/ea_core.v`
- `faraday/rtl/DSP/hdl/EU/ALU/ea_reg.v`
- `faraday/rtl/DSP/hdl/EU/CUN/ec_cun.v`
- `faraday/rtl/DSP/hdl/EU/MAC/em_adder.v`
- `faraday/rtl/DSP/hdl/EU/MAC/em_array.v`
- `faraday/rtl/DSP/hdl/EU/MAC/em_core.v`
- `faraday/rtl/DSP/hdl/EU/MAC/em_mvovf.v`
- `faraday/rtl/DSP/hdl/EU/MAC/em_part.v`
- `faraday/rtl/DSP/hdl/EU/MAC/em_psel_part.v`
- `faraday/rtl/DSP/hdl/EU/SHT/es_array.v`
- `faraday/rtl/DSP/hdl/EU/SHT/es_dec.v`
- `faraday/rtl/DSP/hdl/EU/SHT/es_exp.v`
- `faraday/rtl/DSP/hdl/EU/SHT/es_reg.v`
- `faraday/rtl/DSP/hdl/ICE/i_sice.v`
- `faraday/rtl/DSP/hdl/ICE/x_parser.v`
- `faraday/rtl/DSP/hdl/IDMA/m_idma_dummy.v`
- `faraday/rtl/DSP/hdl/IDMA/m_idma.v`
- `faraday/rtl/DSP/hdl/LIB/m_ecm32kx24.v`
- `faraday/rtl/DSP/hdl/LIB/x_lib.v`
- `faraday/rtl/DSP/hdl/MEMC/m_emc.v`
- `faraday/rtl/DSP/hdl/MEMC/m_memc.v`
- `faraday/rtl/DSP/hdl/PSQ/c_pcstk.v`
- `faraday/rtl/DSP/hdl/PSQ/c_psq.v`
- `faraday/rtl/DSP/hdl/SPORT0/s_autoctl.v`
- `faraday/rtl/DSP/hdl/SPORT0/s_compress.v`
- `faraday/rtl/DSP/hdl/SPORT0/s_expand.v`
- `faraday/rtl/DSP/hdl/SPORT0/s_rxctl0.v`
- `faraday/rtl/DSP/hdl/SPORT0/s_scfg0.v`
- `faraday/rtl/DSP/hdl/SPORT0/s_screg0.v`
- `faraday/rtl/DSP/hdl/SPORT0/s_sport0_dummy.v`
- `faraday/rtl/DSP/hdl/SPORT0/s_sport0.v`
- `faraday/rtl/DSP/hdl/SPORT0/s_txctl0.v`
- `faraday/rtl/DSP/hdl/SPORT1/s_rxctl1.v`
- `faraday/rtl/DSP/hdl/SPORT1/s_scfg1.v`
- `faraday/rtl/DSP/hdl/SPORT1/s_screg1.v`
- `faraday/rtl/DSP/hdl/SPORT1/s_sport1.v`
- `faraday/rtl/DSP/hdl/SPORT1/s_txctl1.v`
- `faraday/rtl/DSP/hdl/TOP/t_clkctl.v`
- `faraday/rtl/DSP/hdl/TOP/t_core.v`
- `faraday/rtl/DSP/hdl/TOP/t_rego.v`
- `gaisler/rtl/leon2/eth_top.v`
- `gaisler/rtl/leon2/ethermac.v`
- `opencores/rtl/ethernet/eth_spram_256x32.v`
- `opencores/rtl/vga_lcd/generic_spram.v`

### `specify` / `endspecify` timing blocks — 2 files

The parser does not accept `specify ... endspecify` (timing arcs,
`specparam`, `$width`, etc.). Error: `syntax error, unexpected specify,
expecting endmodule`.

iwls2005:

- `faraday/rtl/DSP/hdl/CODEC/FXADDA162H90A/ADDA162H90A_atop.v`
- `library/GSCLib_3.0.v` *(stub variant `library/GSCLib_3.0_stub.v` parses)*

### Intra-assignment delay via macro — 2 files

`q <= #N rhs;` parses when `N` is a numeric literal but fails when `N`
is a macro reference such as `q <= #` `` ` ``db rhs;`. Error: `syntax error,
unexpected MACRO_REF, expecting ( or NUMBER or REAL_NUMBER or IDENTIFIER`.

iwls2005:

- `faraday/rtl/DSP/hdl/ICE/i_sice_dummy.v`
- `faraday/rtl/DSP/hdl/ICE/x_parsm.v`

### Packed multi-dimensional vector declarations — 1 file

`wire [31:0][31:0] q;` (a packed array of vectors) is rejected. This is
a SystemVerilog construct — Verilog-2001 only allows one packed
dimension on a `wire`/`reg` declaration.

mips_neelkshah:

- `RegFile32.v`

### SystemVerilog subset

sgcparse now accepts a pragmatic SystemVerilog (IEEE 1800) subset on
top of Verilog-2001 — enough to parse the synthesizable lowRISC/ibex
rtl/ tree end-to-end. Specifically supported:

- `package ... endpackage`, `import pkg::*;`, `import pkg::IDENT;`
- `typedef` of struct/union/enum/vector and forward typedef
- Data types: `logic`, `bit`, `byte`, `int`, `longint`, `shortint`,
  `string`, `void`, plus `signed`/`unsigned` and packed dimensions
- ANSI port lists with typed direction (`input logic [n:0] x`,
  `output crash_dump_t y`, `output pkg::T z`)
- Typed parameters: `parameter int unsigned X`, `parameter logic
  [W-1:0] Y`, `parameter mytype Z`, `parameter pkg::T W`,
  `parameter type T = ...`
- `always_ff`, `always_comb`, `always_latch`
- `unique`, `unique0`, `priority` on `case` and `if`
- Scope resolution `pkg::T` and `pkg::CONST` in expressions
- Unsized untyped literals `'0`, `'1`, `'x`, `'z`
- Assignment patterns `'{a, b, c}`, `'{key: val, ...}`, `'{default:
  ...}`
- Casts `<size_or_type>'(expr)` including `void'(expr)` as a statement
- `for (int i = 0; ...; i++)` and `for (genvar i = ...; ...; i++)`
- Inline `int`/etc. declarations inside `for` headers
- Post/pre `++` / `--` and compound assignments (`+=`, `-=`, `|=`, …)
- `inside` operator: `expr inside { v1, [lo:hi], ... }`
- Member access on selects: `bus[i].field`
- Inline package import in module header: `module foo import pkg::*; #(...)`
- DPI export: `export "DPI-C" function name;`
- End-labels on `module`/`package`/`function`/`task`/`begin`
- Unpacked dimensions on parameters and variables (`X[N]`)

Out of scope (will not parse): classes, virtual interfaces, modports
beyond the bare `interface ... endinterface` shell, randomization,
constraints, properties/sequences, assertions (Ibex hides these
behind macros which expand to nothing through the empty
`prim_assert.sv` stub).

SystemVerilog keywords are scoped to file extension. Files ending in
`.sv` / `.svh` get the SV keyword set; `.v` / `.vh` files keep the
Verilog-2001 keyword set so legacy sources that use names like
`int`, `logic`, `bit`, `type`, `var`, `const`, `static`, `local`,
`return`, `final`, `do`, `enum`, `struct`, `union`, `package`,
`packed`, `import`, `export`, `null`, `inside`, `iff`, `unique`,
`priority`, `interface`, `modport`, `virtual`, `pure`, or `void` as
identifiers continue to parse.

### Ibex coverage — what actually parses

The Ibex regress (`regress/designs/ibex/`) parses only the
synthesizable RTL — the 30 `.sv` files directly under `rtl/`. Even
that has caveats, and the rest of the repository is mostly out of
reach. Honest accounting against the pinned Ibex commit (644 total
`.sv` / `.svh` files):

| Subtree    | Files | Pass | Notes |
| ---------- | ----: | ---: | --- |
| `rtl/`     |    30 |   30 | What the regress exercises. Synthesizable core. |
| `vendor/`  |   452 |  184 | OpenTitan `prim_*` IP and RISC-V tooling. ~40% — the rest needs class/property/sequence support. |
| `dv/`      |   135 |    9 | UVM testbench: classes, randomization, constraints, covergroups — almost entirely out of scope. |
| `formal/`  |    20 |    1 | Assertion-heavy. |
| `shared/`  |     6 |    5 | |
| `examples/`|     1 |    — | Not measured. |

The 30/30 in `rtl/` also depends on three OpenTitan headers that the
harness stubs to empty files: `prim_assert.sv`, `dv_fcov_macros.svh`,
`formal_tb_frag.svh`. Their real contents define seven assertion
macros (`` `ASSERT ``, `` `ASSERT_KNOWN ``, `` `ASSERT_INIT ``,
`` `ASSERT_IF ``, `` `ASSERT_KNOWN_IF ``, `` `COVER ``,
`` `ASSUME ``) that expand to SV `property` / `sequence` /
`assert property` constructs — none of which the grammar accepts.
Stubbing the includes makes every macro call site disappear, so the
30 files parse but the full preprocessor + assertion bodies do not.

What's still missing to push beyond `rtl/` (in roughly decreasing
order of how many files each unlocks):
- SV classes (`class`/`endclass`, `extends`, `new`, `this`, `super`)
- Constraints (`constraint`/`solve`/`with`)
- Randomization (`rand`/`randc`/`randomize()`/`pre_randomize`)
- Covergroups (`covergroup`/`coverpoint`/`bins`/`cross`)
- Properties / sequences / `assert property` / clocking blocks
- `interface` bodies with `modport`s used as port types

Lifting any of these would multiply the regress matrix and is
beyond the current SV subset's stated scope (see
`verilog/PLAN.md`).

---

## Upstream issues

These are bugs or environmental assumptions in the source. Other Verilog
simulators reject most of these as well.

### `timescale.v` missing from include path — 2 files

These files do `` `include "timescale.v" `` (unqualified). The
`opencores/rtl/simple_spi/` directory has no `timescale.v` of its own;
neighbouring opencores designs do. The regress harness only adds the
file's own directory to the include path, so resolution fails. Adding
`-I .../opencores/rtl/ethernet` (or any sibling that ships a
`timescale.v`) would make these parse.

iwls2005:

- `opencores/rtl/simple_spi/fifo4.v`
- `opencores/rtl/simple_spi/simple_spi_top.v`

### Trailing comma in module port list — 1 file

Module port list ends with `..., MICM, );` — the trailing comma before
`)` is not legal Verilog. Error: `syntax error, unexpected )`.

iwls2005:

- `faraday/rtl/DSP/hdl/CODEC/FXADDA162H90A/FS90B779.v`

### `always (@event)` — parens around event control — 6 files

Upstream typo: `always (@negedge clk) begin ... end`. The legal form is
`always @(negedge clk) begin ... end`. Affects six pipeline-register
modules in the `mips_neelkshah` design.

mips_neelkshah:

- `DataForwardingUnit.v`
- `EX_MEM.v`
- `HazardDetectionUnit.v`
- `ID_EX.v`
- `IF_ID.v`
- `MEM_WB.v`

### Hard-coded absolute include path — 2 files

The `` `include `` directive is a host-specific absolute path on the
upstream author's filesystem.

iwls2005:

- `faraday/rtl/DSP/hdl/CODEC/FXADDA162H90A/FS90B779_sim.v`
  — `` `include "/home/msd/users/cjm1173/ad/FS90B717_b/src/include/header.v" ``

ddr2_controller:

- `tb/tb.v`
  — `` `include "/auto/home-scf-06/ee577/design_pdk/osu_stdcells/lib/tsmc018/lib/osu018_stdcells.v" ``
