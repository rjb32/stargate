# Journal

## 28/11/2025

Infra lab to build FPGAs.
Run on-demand infrastructure.

Build: a tool can either run locally or on a machine
Machine flow: specify a specific machine already existing
Cloud flow: provision a machine from the cloud

## 27/11/2025

Hybrid netlist representation:
* Immutable contiguous representation
* With modifications on top
* Compaction if necessary

What about parallel editing:
* We could admit parallel editing in parallel non-conflicting hierarchies
* Is that really interesting?

It's possible in a design-based modular flow by merging DBs later or designs later.

Multiple writers:
* Create scoped writers: check if no conflicts with active writers
* On commit: check if no conflicts
* Use snapshots and rollbacks

--> Can use CRDT netlists where each writer submits a list of operations

Inspired by FuseSoc
Build system description:
* Project file in yaml
* Filesets made of files
* Targets
** Each target can inherit from a parent target
** Each target has a fileset or a fileset_append
** Each target has a default tool name
** Additional parameters can be passed to tools
