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
