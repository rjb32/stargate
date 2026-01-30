# Compilation flows

## Overview

We want to make a flow management system for the Stargate hardware design compiler.


## Flows and tasks

Tasks are the main operations necessary to be done on a hardware design project in Stargate.
Each task is the invocation of a tool implementing the task, which can be a stargate internal tool
or an external tool.

A flow is a sequence of tasks to be done. This is a well-known abstraction for EDA tools and FPGA
prototyping tools.

A flow can have several sections. Each section is a sequence of tasks, such as build and run.

We recognize that typical EDA prototyping flows have a "build" phase which goes usually
from analyzing RTL to synthesis, netlist transformations to make it amenable to FPGAs and finally
invoking proprietary FPGA vendor tools to generate the final FPGA bitstream.:

We can imagine to be able to have flows that have more than just build and run in the future,
but also custom "post run" phases such as to visualize waveforms for example.

We will in stargate various flow classes, inheriting from a common "Flow" ancestor, for the most
common flows in FPGA design. For example, a vivado flow where the build section will be all the
tasks in vivado going from synthesis to bitstream generation, and a run section that will be the
actual programming of the device and run of the design on an FPGA board.


## How flows and tasks are invoked

In Stargate we always compile a target in the project configuration file.
There is always a unique flow specified for each target.

Run a flow section:

stargate build

or 

stargate run

Run a task:

stargate -task synth

Run or re-run a range of tasks:

stargate -start_task synth -end_task timing


## Dependency management

Each flow section is a sequence of tasks. 
A given task can not be started if all the tasks before it have not completed successfully.

How do we know previous tasks status: a status file will be written at the end of each task in 
the task directory.

A given flow section can not be started when the previous sections before it in the same flow
have not completed successfully. For example, we can not execute the run section of a given flow
if the build section has not successfully completed.


## How tasks are defined

A task is defined as invoking tools. 
A tool can be external, such as vivado, or internal to stargate.
An internal stargate tool can be linked as code inside the stargate binary and just be defined
as a child of the Tool class.

## Tools

Tools are standalone classes that are the embodiment and the external interface of an internal
or external tool (such as vivado).

Tool classes reside in the sgc_tool_s static library. There is no tool registry at this point.
Tasks can instantiate directly the tool class that they need.

## Directory structure

Since there is only one flow specified for the current target in the configuration file,
we can create one directory per task under sgc.out in a flat way:

sgc.out
|----> project/
|----> task1_name/
|----> task2_name/

Should we instead have a flow directory?

sgc.out
|---> project/
|---> vivado/
|--------> task1/
|--------> task2/


## Code architecture

Physical architecture:
In a stargate static library stargate_flow_s in a flow/ directory.

The concrete classes that need to be owned by an owning class use the static create pattern:
static create method in the owned class that calls in the implementation a addX(obj) in the parent class
to register itself. That way the parent class can use a std::vector<Obj*> with good old pointers to facilitate
ranges on Obj*.

Classes:
* FlowManager
* Flow
* FlowSection
* FlowTask

Ownership structure:
FlowManager --> Flow --> FlowSection --> FlowTask

FlowManager:
public:
using Flows = std::vector<Flow*>;
using FlowNameMap = std::unordered_map<std::string_view, Flow*>;
const Flows& flows() const;
Flow* getFlow(std::string_view name) const;

private:
Flows _flows;
FlowNameMap _flowNameMap;


Flow: abstract base class
public:
virtual std::string_view getName() const = 0;
FlowSection* getBuildSection() const;
FlowSection* getRunSection() const;

protected:
FlowSection* _build
FlowSection* _runSection

Each concrete Flow class instance registers itself in FlowManager using the static create pattern.

FlowSection: abstract base class
public:
using Tasks = std::vector<Task*>;
using TaskNameMap = std::unordered_map<std::string_view, FlowTask*>;
Flow* getParent() const;
const Tasks& tasks() const;
FlowTask* getTask(std::string_view name) const;

FlowTask: abstract base class
public:
FlowSection* getParent() const;

