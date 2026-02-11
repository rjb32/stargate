# Netlist Data Structure Design Discussion

This document captures the ongoing design discussion for a hierarchical FPGA netlist data structure optimized for cache efficiency and transformation operations.

## 1. Overview and Design Goals

### Primary Goals

1. **Cache-efficient iteration**: Objects stored in contiguous containers (`std::vector`) to maximize cache locality during traversal
2. **Hierarchical multi-instantiation**: Support for DAG hierarchy where the same Design can be instantiated multiple times
3. **Space-based object allocation**: New objects are always allocated in a new space; existing objects in old spaces may only be mutated by the Uniquifier at commit time
4. **Minimal uniquification**: When transforming specific instantiation contexts, only uniquify the minimum necessary hierarchy
5. **Dual representation support**: A hierarchical netlist (source of truth) and a fast flat netlist (constructed on demand for transformations)

### Key Assumptions

- The netlist is read-only during parallel transformations; only the Uniquifier mutates it at commit time
- New objects are always created in a new space; the Uniquifier may also update existing objects in old spaces (e.g., appending ChunkedSpan chunks, updating connectivity)
- Modifications are batched and committed atomically via the Uniquifier
- Level-by-level processing (bottom-up) is the primary parallelism model
- No intra-level visibility of changes is required during parallel transformations
- Compaction can be deferred; dump/reload is acceptable for major cleanup
- Undo/history is not required; old spaces are discarded after commit

---

## 2. Core Design Principles

### Space-Based Mutation Model

- A `NetlistSpace` contains the `std::vector` containers that own all netlist objects
- **New objects** are always allocated in a **new** space's vectors
- **Existing objects** in old spaces may be **mutated by the Uniquifier at commit time** (e.g., appending a chunk to a Design's `ChunkedSpan`, updating connectivity on an existing net)
- During parallel transformations (between commits), the netlist is effectively **read-only** — workers only collect changes, they do not modify the netlist
- A `Design`'s objects may span multiple spaces via `ChunkedSpan`: the original objects remain in the old space, new objects live in the new space, and the Design's ChunkedSpan is updated to reference both

**Analogy: generational garbage collection.** This model is similar to a generational GC. New objects are allocated in the young generation (new space), old objects remain in older generations (old spaces), and the collector (Uniquifier) updates cross-generation references (ChunkedSpans, model pointers) during a stop-the-world pause (commit barrier). Compaction serves the same role as GC heap compaction — consolidating fragmented spans into contiguous storage. Like generational GC, the key bet is that most objects survive untouched: only a small fraction of Designs are affected by a given transformation.

### ChunkedSpan for Cross-Space Spanning

All netlist objects (nets, instances, terms, etc.) are allocated in the `std::vector` containers within `NetlistSpace`. Objects in a `Design`, `Instance`, or `BusNet` are **not** owned by those structs—they are owned by the space. `ChunkedSpan` is a lightweight view that references contiguous ranges within those space-level vectors.

When a Design is first constructed, all its objects land in a single `NetlistSpace` vector, so each `ChunkedSpan` has one chunk. After a transformation creates a new space and appends new objects (e.g., new nets added to an existing Design), the `ChunkedSpan` gains an additional chunk pointing into the new space's vector. Each chunk is a `std::span` into a vector belonging to a different `NetlistSpace`.

```cpp
template<typename T, size_t MaxChunks = 3>
struct ChunkedSpan {
    std::array<std::span<T>, MaxChunks> chunks;
    uint8_t numChunks = 0;

    void append(std::span<T> newChunk);

    template<typename F>
    void forEach(F&& func) const {
        if (numChunks >= 1) for (auto& e : chunks[0]) func(e);
        if (numChunks >= 2) for (auto& e : chunks[1]) func(e);
        if (numChunks >= 3) for (auto& e : chunks[2]) func(e);
    }

    bool needsCompaction() const { return numChunks == MaxChunks; }
    size_t size() const;

    // Direct access for single-chunk case (used by primitives)
    bool isSingleChunk() const { return numChunks <= 1; }
    T* data() { return chunks[0].data(); }
    const T* data() const { return chunks[0].data(); }
};
```

- Each span references a contiguous range within a `NetlistSpace` vector
- Fixed array of 3 spans maximum (inlined if-statements, no loop overhead)
- Compaction is required if a 4th chunk would be needed
- Provides `ChunkedRange` with iterators for range-based for loops

### Example: Adding a ScalarNet to an Existing Design

Two distinct cases depending on whether the change is design-wide or occurrence-specific:

**Design-wide** (all instances see the change):
1. A new `ScalarNet` is allocated in the **new space**'s `scalarNets` vector
2. At commit, the Uniquifier updates the existing Design's `scalarNets` ChunkedSpan, appending a new chunk that points into the new space's vector
3. All `Instance::model` pointers still reference the same Design — no pointer updates needed
4. Every instance of this Design automatically sees the new net

**Occurrence-specific** (only one instantiation context is affected):
1. A new `ScalarNet` is allocated in the **new space**'s `scalarNets` vector
2. The Uniquifier creates a **uniquified copy** of the Design in the new space, copying the old ChunkedSpans and appending the new chunk
3. Only the specific `Instance::model` pointer along the target path is updated to point to the new Design copy
4. Other instances of the original Design are unaffected

### Mutable Instance-to-Design Reference

- `Instance::model` pointer is mutable (can be swapped during uniquification)
- This avoids cascading uniquification when only internal Design changes occur
- Uniquification propagates upward only when interface/context changes are needed

### Globally Unique IDs

- All object IDs are globally unique within the netlist
- Enables serialization and path representation without name lookups
- Strongly-typed ID wrappers prevent mixing ID types

### Name Interning

- Object names stored in a `NameTable` (string interning)
- Objects store `Name` (4 bytes) instead of `std::string`
- Same string always yields the same `Name`

---

## 3. Entity Descriptions and Relationships

### Hierarchy Model

```
Netlist
└── Design (multiple)
    ├── Nets (ScalarNet, BusNet)
    │   └── BusNetBit (for buses)
    ├── DesignTerms / Ports (ScalarDesignTerm, BusDesignTerm)
    │   └── BusDesignTermBit (for buses)
    └── Instances (of other Designs)
        └── InstTerms (ScalarInstTerm, BusInstTerm)
            └── BusInstTermBit (for buses)
```

### Entity Relationships

| Entity | Contains | References |
|--------|----------|------------|
| Netlist | Libraries, NameTable, AttributeTable, NameIndex, Epochs | Top Design, PrimitiveLibrary |
| Library | Designs (with name index) | - |
| Design | Nets, DesignTerms, Instances | - |
| Instance | InstTerms | parent Design, model Design (mutable) |
| Net (bit-level) | - | parent Design, Connected InstTerms, Connected DesignTerms |
| DesignTerm (bit-level) | - | parent Design, Connected Net |
| InstTerm (bit-level) | - | parent Instance, DesignTerm (by index), Connected Net |
| BusNet | BusNetBits | parent Design |
| BusNetBit | - | parent BusNet |
| BusDesignTerm | BusDesignTermBits | parent Design |
| BusDesignTermBit | - | parent BusDesignTerm |
| BusInstTerm | BusInstTermBits | parent Instance |
| BusInstTermBit | - | parent BusInstTerm |

### Connectivity Model

- Connectivity is **bidirectional** at the bit level:
  - `BitNet` stores lists of connected `BitInstTerm*` and `BitDesignTerm*`
  - `BitInstTerm` and `BitDesignTerm` store their connected `BitNet*`
- A terminal connects to **at most one** net (or nullptr if unconnected)
- A net can connect to **multiple** terminals
- **No direct net-to-net connections**; only net-to-term connections

### InstTerm to DesignTerm Mapping

- Uses **hierarchical indexing** (not flattened bit-level):
  - `ScalarInstTerm` stores `scalarTermIndex` → index into `Design::scalarDesignTerms`
  - `BusInstTermBit` stores `busTermIndex` + `bitIndex` → index into `Design::busDesignTerms`, then bit within
- When Design is uniquified, indices remain valid (term order preserved)
- Only `Instance::model` pointer needs updating, not individual InstTerms

---

## 4. Class and Struct Definitions

### ID Types

```cpp
struct NetID { uint32_t value; };
struct InstanceID { uint32_t value; };
struct DesignTermID { uint32_t value; };
struct InstTermID { uint32_t value; };
struct DesignID { uint32_t value; };
struct LibraryID { uint32_t value; };
struct Name { uint32_t value; };
```

### Direction Enum

```cpp
enum class Direction : uint8_t {
    Input,
    Output,
    InOut,
};
```

### Name Table

```cpp
class NameTable {
public:
    Name getName(std::string_view str);
    std::string_view getString(Name name) const;

private:
    std::unordered_map<std::string, Name> _stringToName;
    std::vector<std::string> _nameToString;
};
```

### Net Classes

```cpp
// Base for bit-level connectable nets
struct BitNet {
    NetID id;
    Name name;
    Design* parent;            // Containing design
    ChunkedSpan<BitInstTerm*> connectedInstTerms;
    ChunkedSpan<BitDesignTerm*> connectedDesignTerms;
};

struct ScalarNet : BitNet {
    // No additional members
};

struct BusNetBit : BitNet {
    BusNet* bus;               // Parent bus (renamed from 'parent' for clarity)
    uint32_t index;
};

struct BusNet {
    NetID id;
    Name name;
    Design* parent;            // Containing design
    int32_t msb;
    int32_t lsb;
    ChunkedSpan<BusNetBit> bits;
};
```

### Design Term Classes (Ports)

```cpp
struct BitDesignTerm {
    DesignTermID id;
    Design* parent;            // Containing design
    Direction direction;
    BitNet* connectedNet;
};

struct ScalarDesignTerm : BitDesignTerm {
    Name name;
};

struct BusDesignTermBit : BitDesignTerm {
    BusDesignTerm* bus;        // Parent bus (renamed from 'parent' for clarity)
    uint32_t index;
};

struct BusDesignTerm {
    DesignTermID id;
    Name name;
    Design* parent;            // Containing design
    Direction direction;
    int32_t msb;
    int32_t lsb;
    ChunkedSpan<BusDesignTermBit> bits;
};
```

### Instance Term Classes

```cpp
// Base for bit-level connectable instance terms
struct BitInstTerm {
    InstTermID id;
    Instance* instance;        // Parent instance
    BitNet* connectedNet;
};

struct ScalarInstTerm : BitInstTerm {
    uint16_t scalarTermIndex;  // Index into design's scalarDesignTerms
};

struct BusInstTermBit : BitInstTerm {
    BusInstTerm* bus;          // Parent bus
    uint16_t busTermIndex;     // Index into design's busDesignTerms
    uint16_t bitIndex;         // Index within that bus
};

struct BusInstTerm {
    InstTermID id;
    Instance* instance;        // Parent instance
    uint16_t busTermIndex;     // Index into design's busDesignTerms
    ChunkedSpan<BusInstTermBit> bits;
};
```

### Instance and Design

```cpp
struct Instance {
    InstanceID id;
    Name name;
    Design* parent;  // Containing design
    Design* model;   // Instantiated design (mutable - can swap on uniquification)
    uint32_t flags = 0;

    ChunkedSpan<ScalarInstTerm> scalarInstTerms;
    ChunkedSpan<BusInstTerm> busInstTerms;

    // --- Flag layout ---
    // Bits 0-15:  Instance flags (reserved for future use)
    // Bits 16-31: PrimitiveKind (0 = not a primitive instance)

    static constexpr uint32_t PrimitiveKindShift = 16;
    static constexpr uint32_t PrimitiveKindMask = 0xFFFF0000;
    static constexpr uint32_t InstanceFlagsMask = 0x0000FFFF;

    PrimitiveKind primitiveKind() const {
        return static_cast<PrimitiveKind>((flags & PrimitiveKindMask) >> PrimitiveKindShift);
    }

    bool isPrimitive() const {
        return primitiveKind() != PrimitiveKind::None;
    }

    void setPrimitiveKind(PrimitiveKind kind) {
        flags = (flags & InstanceFlagsMask) | (uint32_t(kind) << PrimitiveKindShift);
    }

    // --- Fast primitive term access (single-chunk guaranteed for primitives) ---

    ScalarInstTerm* getPrimitiveScalarTerm(uint16_t scalarIndex) {
        return scalarInstTerms.data() + scalarIndex;
    }

    const ScalarInstTerm* getPrimitiveScalarTerm(uint16_t scalarIndex) const {
        return scalarInstTerms.data() + scalarIndex;
    }

    BusInstTerm* getPrimitiveBusTerm(uint16_t busIndex) {
        return busInstTerms.data() + busIndex;
    }

    const BusInstTerm* getPrimitiveBusTerm(uint16_t busIndex) const {
        return busInstTerms.data() + busIndex;
    }

    BusInstTermBit* getPrimitiveBusTermBit(uint16_t busIndex, uint16_t bitIndex) {
        return getPrimitiveBusTerm(busIndex)->bits.data() + bitIndex;
    }

    const BusInstTermBit* getPrimitiveBusTermBit(uint16_t busIndex, uint16_t bitIndex) const {
        return getPrimitiveBusTerm(busIndex)->bits.data() + bitIndex;
    }
};

struct Design {
    DesignID id;
    Name name;
    uint32_t flags = 0;

    // Flag bit definitions
    static constexpr uint32_t FlagPrimitive = 1 << 0;
    // Future flags can use bits 1-31

    bool isPrimitive() const { return flags & FlagPrimitive; }
    void setPrimitive(bool val) {
        if (val) flags |= FlagPrimitive;
        else flags &= ~FlagPrimitive;
    }

    ChunkedSpan<ScalarNet> scalarNets;
    ChunkedSpan<BusNet> busNets;

    ChunkedSpan<ScalarDesignTerm> scalarDesignTerms;
    ChunkedSpan<BusDesignTerm> busDesignTerms;

    ChunkedSpan<Instance> instances;
};
```

### Netlist Space

```cpp
struct NetlistSpace {
    NetlistSpace* parent;  // For structural sharing (may be nullptr)

    // Append-only containers
    std::vector<ScalarNet> scalarNets;
    std::vector<BusNet> busNets;
    std::vector<BusNetBit> busNetBits;

    std::vector<ScalarDesignTerm> scalarDesignTerms;
    std::vector<BusDesignTerm> busDesignTerms;
    std::vector<BusDesignTermBit> busDesignTermBits;

    std::vector<ScalarInstTerm> scalarInstTerms;
    std::vector<BusInstTerm> busInstTerms;
    std::vector<BusInstTermBit> busInstTermBits;

    std::vector<Instance> instances;
    std::vector<Design> designs;
};
```

### Netlist

```cpp
class Netlist {
public:
    Design* getTopDesign() const;

    NameTable* nameTable();
    const NameTable* nameTable() const;

    AttributeTable* attributeTable();
    const AttributeTable* attributeTable() const;

    NameIndex* nameIndex();
    const NameIndex* nameIndex() const;

    // Library access (designs reside in libraries)
    PrimitiveLibrary* primitiveLibrary() const;
    std::span<Library*> libraries() const;

    // Helper: find design by name across all libraries (O(n) in libraries)
    Design* findDesign(Name name) const {
        for (Library* lib : _libraries) {
            if (Design* d = lib->findDesign(name)) {
                return d;
            }
        }
        return nullptr;
    }

private:
    NetlistSpace* _currentSpace = nullptr;
    std::unique_ptr<NameTable> _nameTable;
    std::unique_ptr<AttributeTable> _attributeTable;
    std::unique_ptr<NameIndex> _nameIndex;

    PrimitiveLibrary* _primitiveLibrary = nullptr;
    std::vector<Library*> _libraries;

    // Global ID allocation
    uint32_t _nextNetID = 0;
    uint32_t _nextInstanceID = 0;
    uint32_t _nextDesignTermID = 0;
    uint32_t _nextInstTermID = 0;
    uint32_t _nextDesignID = 0;
    uint32_t _nextLibraryID = 0;

    friend class Uniquifier;
    friend class Compactor;
    friend class NetlistBuilder;
    friend class NetlistLoader;
    friend class NetlistDumper;
};
```

### Attribute Table

```cpp
class AttributeTable {
public:
    void set(ObjectID obj, Name key, AttrValue value);
    AttrValue* get(ObjectID obj, Name key);
    const AttrValue* get(ObjectID obj, Name key) const;
    void remove(ObjectID obj, Name key);

    void forEach(ObjectID obj,
                 std::function<void(Name, const AttrValue&)> callback) const;

private:
    std::unordered_map<ObjectID, std::unordered_map<Name, AttrValue>> _attrs;
};
```

### Name Index

Optional name-based lookup for objects within designs. Built on demand, either explicitly or on first lookup.

```cpp
class NameIndex {
public:
    // Explicit bulk indexing
    void indexDesign(Design* design);
    void indexLibrary(Library* library);

    // Lookups - auto-indexes on first access if not already indexed
    Instance* findInstance(Design* design, Name name);
    ScalarNet* findScalarNet(Design* design, Name name);
    BusNet* findBusNet(Design* design, Name name);
    ScalarDesignTerm* findScalarDesignTerm(Design* design, Name name);
    BusDesignTerm* findBusDesignTerm(Design* design, Name name);

    // Check if indexed
    bool isDesignIndexed(Design* design) const;
    bool isLibraryIndexed(Library* library) const;

    // Invalidate (call after space change)
    void invalidate();
    void invalidateDesign(Design* design);
    void invalidateLibrary(Library* library);

private:
    struct DesignIndex {
        std::unordered_map<Name, Instance*> instances;
        std::unordered_map<Name, ScalarNet*> scalarNets;
        std::unordered_map<Name, BusNet*> busNets;
        std::unordered_map<Name, ScalarDesignTerm*> scalarDesignTerms;
        std::unordered_map<Name, BusDesignTerm*> busDesignTerms;
    };

    std::unordered_map<Design*, DesignIndex> _designIndices;
    std::unordered_set<Library*> _indexedLibraries;
};
```

**Note:** Library already has its own `_designsByName` index (always built). The `NameIndex::indexLibrary()` is for consistency and marks the library as indexed for `isLibraryIndexed()` checks.

---

## 5. Libraries and Primitives

### Library Structure

Designs are organized into libraries. Primitive libraries can only contain primitive designs. Design names must be unique within a library.

```cpp
class Library {
public:
    LibraryID id;
    Name name;
    uint32_t flags = 0;

    static constexpr uint32_t FlagPrimitiveOnly = 1 << 0;

    bool isPrimitiveOnly() const { return flags & FlagPrimitiveOnly; }

    // Add a design to the library (name must be unique)
    void addDesign(Design* design) {
        designs.push_back(design);
        _designsByName[design->name] = design;
    }

    // Lookup by name - O(1)
    Design* findDesign(Name name) const {
        auto it = _designsByName.find(name);
        return (it != _designsByName.end()) ? it->second : nullptr;
    }

    // Iteration
    std::span<Design*> getDesigns() const { return designs; }

protected:
    std::vector<Design*> designs;

private:
    std::unordered_map<Name, Design*> _designsByName;
};
```

### Primitive Designs

Primitives are leaf-level designs with no internal netlist objects (no nets, no instances—only DesignTerms).

**Invariants:**
- Primitive designs have `Design::FlagPrimitive` set
- Primitive designs can only exist in libraries with `Library::FlagPrimitiveOnly`
- Primitive designs have no nets and no instances, only DesignTerms
- Instances of primitives have single-chunk storage for InstTerms (guaranteed O(1) indexed access)

### PrimitiveKind Enum

```cpp
// Stored in upper 16 bits of Instance::flags
// 0 = not a primitive instance
enum class PrimitiveKind : uint16_t {
    None = 0,
    LUT1, LUT2, LUT3, LUT4, LUT5, LUT6,
    DFF, DFFE, DFFR, DFFRE, DFFS, DFFSE,
    BUF, INV,
    IBUF, OBUF, IOBUF,
    CARRY4,
    MUX2, MUX4, MUX8,
    BRAM18, BRAM36,
    DSP48,
    // Extend as needed
    Count,
};
```

### PrimitiveLibrary

`PrimitiveLibrary` inherits from `Library` and provides fast access to primitive designs.

```cpp
class PrimitiveLibrary : public Library {
public:
    // Designs are stored in inherited Library::designs vector
    // Index = PrimitiveKind - 1 (since None = 0 is not stored)

    void initialize(Netlist* netlist);

    // Generic access by kind
    Design* get(PrimitiveKind kind) const {
        if (kind == PrimitiveKind::None) return nullptr;
        uint16_t index = uint16_t(kind) - 1;
        return designs[index];
    }

    // Direct accessors
    Design* getLUT1() const { return get(PrimitiveKind::LUT1); }
    Design* getLUT2() const { return get(PrimitiveKind::LUT2); }
    Design* getLUT3() const { return get(PrimitiveKind::LUT3); }
    Design* getLUT4() const { return get(PrimitiveKind::LUT4); }
    Design* getLUT5() const { return get(PrimitiveKind::LUT5); }
    Design* getLUT6() const { return get(PrimitiveKind::LUT6); }

    Design* getDFF() const { return get(PrimitiveKind::DFF); }
    Design* getDFFE() const { return get(PrimitiveKind::DFFE); }
    Design* getDFFR() const { return get(PrimitiveKind::DFFR); }
    Design* getDFFRE() const { return get(PrimitiveKind::DFFRE); }

    Design* getBUF() const { return get(PrimitiveKind::BUF); }
    Design* getINV() const { return get(PrimitiveKind::INV); }

    Design* getBRAM18() const { return get(PrimitiveKind::BRAM18); }
    Design* getBRAM36() const { return get(PrimitiveKind::BRAM36); }
    Design* getDSP48() const { return get(PrimitiveKind::DSP48); }

    // Category checks using PrimitiveKind
    static bool isLUT(PrimitiveKind kind) {
        return kind >= PrimitiveKind::LUT1 && kind <= PrimitiveKind::LUT6;
    }

    static bool isFlipFlop(PrimitiveKind kind) {
        return kind >= PrimitiveKind::DFF && kind <= PrimitiveKind::DFFSE;
    }

    static bool isBRAM(PrimitiveKind kind) {
        return kind == PrimitiveKind::BRAM18 || kind == PrimitiveKind::BRAM36;
    }
};
```

### Primitive Pin Indices

Compile-time constants define pin positions for fast O(1) access. Scalar and bus pins use separate index spaces.

```cpp
// LUTs - all scalar pins
namespace LUT1Pins {
    constexpr uint16_t I0 = 0;
    constexpr uint16_t O  = 1;
}

namespace LUT4Pins {
    constexpr uint16_t I0 = 0;
    constexpr uint16_t I1 = 1;
    constexpr uint16_t I2 = 2;
    constexpr uint16_t I3 = 3;
    constexpr uint16_t O  = 4;
}

namespace LUT6Pins {
    constexpr uint16_t I0 = 0;
    constexpr uint16_t I1 = 1;
    constexpr uint16_t I2 = 2;
    constexpr uint16_t I3 = 3;
    constexpr uint16_t I4 = 4;
    constexpr uint16_t I5 = 5;
    constexpr uint16_t O  = 6;
}

// Flip-flops - all scalar pins
namespace DFFPins {
    constexpr uint16_t D   = 0;
    constexpr uint16_t CLK = 1;
    constexpr uint16_t Q   = 2;
}

namespace DFFREPins {
    constexpr uint16_t D   = 0;
    constexpr uint16_t CLK = 1;
    constexpr uint16_t CE  = 2;
    constexpr uint16_t R   = 3;
    constexpr uint16_t Q   = 4;
}

// Buffers - all scalar pins
namespace BUFPins {
    constexpr uint16_t I = 0;
    constexpr uint16_t O = 1;
}

namespace INVPins {
    constexpr uint16_t I = 0;
    constexpr uint16_t O = 1;
}

// BRAM - mixed scalar and bus pins
// Scalar pins use getPrimitiveScalarTerm(), bus pins use getPrimitiveBusTerm()
namespace BRAM18Pins {
    // Scalar pins
    constexpr uint16_t CLKA = 0;
    constexpr uint16_t ENA  = 1;
    constexpr uint16_t WEA  = 2;
    constexpr uint16_t CLKB = 3;
    constexpr uint16_t ENB  = 4;
    constexpr uint16_t WEB  = 5;

    // Bus pins (separate index space)
    constexpr uint16_t ADDRA = 0;
    constexpr uint16_t ADDRB = 1;
    constexpr uint16_t DIA   = 2;
    constexpr uint16_t DIB   = 3;
    constexpr uint16_t DOA   = 4;
    constexpr uint16_t DOB   = 5;
}

// DSP - mixed scalar and bus pins
namespace DSP48Pins {
    // Scalar pins
    constexpr uint16_t CLK = 0;
    constexpr uint16_t CE  = 1;
    constexpr uint16_t RST = 2;

    // Bus pins
    constexpr uint16_t A      = 0;
    constexpr uint16_t B      = 1;
    constexpr uint16_t C      = 2;
    constexpr uint16_t P      = 3;
    constexpr uint16_t OPMODE = 4;
}
```

### Fast Primitive Access Examples

```cpp
// Get LUT output
Instance* lutInst = ...;
ScalarInstTerm* outTerm = lutInst->getPrimitiveScalarTerm(LUT4Pins::O);

// Get flip-flop clock
Instance* ffInst = ...;
ScalarInstTerm* clkTerm = ffInst->getPrimitiveScalarTerm(DFFPins::CLK);

// Get BRAM data bus
Instance* bramInst = ...;
BusInstTerm* dataOutA = bramInst->getPrimitiveBusTerm(BRAM18Pins::DOA);

// Get specific bit of BRAM address
BusInstTermBit* addrBit5 = bramInst->getPrimitiveBusTermBit(BRAM18Pins::ADDRA, 5);

// Check primitive kind directly from instance (no pointer comparison needed)
if (PrimitiveLibrary::isLUT(inst->primitiveKind())) {
    // Handle LUT
}

if (inst->primitiveKind() == PrimitiveKind::BRAM18) {
    // Handle BRAM18 specifically
}
```

---

## 6. Occurrence System

### Definition

An **Occurrence** combines a hierarchical path with an object in that context. The path specifies a design in a particular instantiation context, and the object is within that design.

### Path Semantics

- `Path` is a sequence of `InstanceID` values representing the instantiation path
- For path `[top_id, cpu_0_id]`, the context is the design instantiated by `cpu_0`
- The object (e.g., Instance, Net) is **within** the design at the end of the path

Example: For occurrence of instance `alu` at path `top/cpu_0`:
- `path = [top_id, cpu_0_id]`
- `instance = alu` (alu is within cpu_0's design)

### Occurrence Types

```cpp
using Path = std::vector<InstanceID>;

struct InstanceOccurrence {
    Path path;
    Instance* instance;
};

struct NetOccurrence {
    Path path;
    Net* net;  // Could be BitNet* for bit-level
};

struct InstTermOccurrence {
    Path path;
    InstTerm* instTerm;  // Could be BitInstTerm* for bit-level
};

struct DesignTermOccurrence {
    Path path;
    DesignTerm* designTerm;  // Could be BitDesignTerm* for bit-level
};

// Bit-level variants
struct BitNetOccurrence {
    Path path;
    BitNet* bitNet;
};

struct BitInstTermOccurrence {
    Path path;
    BitInstTerm* bitInstTerm;
};

struct BitDesignTermOccurrence {
    Path path;
    BitDesignTerm* bitDesignTerm;
};
```

### Occurrence Validity

- Occurrences may become invalid after uniquification
- Users must be aware that committing changes invalidates previously obtained occurrences
- Re-resolution against the new space is required after commit

---

## 7. Uniquifier and Change Collector

### Design Principles

- The `Uniquifier` class owns the change collection and commit logic
- Users do not manipulate spaces directly; the Uniquifier handles space management
- Changes are recorded (not applied) during collection, then applied atomically on commit
- Validation occurs at commit time, not during change collection

### Uniquification Rules

1. **Internal Design changes** (nets, logic inside): Uniquify only the affected Design
2. **Interface/context changes** (ports, connections): Propagate uniquification upward
3. **Minimal uniquification**: Use LCA (Lowest Common Ancestor) approach
4. If only `Instance::model` pointer changes (no interface changes), no upward propagation

### Pending References

For objects created within a change collection (before commit):

```cpp
struct PendingScalarNetRef { uint32_t pendingId; };
struct PendingBusNetRef { uint32_t pendingId; };
struct PendingInstanceRef { uint32_t pendingId; };
struct PendingScalarDesignTermRef { uint32_t pendingId; };
struct PendingBusDesignTermRef { uint32_t pendingId; };

template<typename Existing, typename Pending>
using MaybeNewRef = std::variant<Existing, Pending>;

using AnyScalarNetRef = MaybeNewRef<ScalarNetRef, PendingScalarNetRef>;
using AnyBusNetRef = MaybeNewRef<BusNetRef, PendingBusNetRef>;
// ... etc
```

**Note on resolution**: After `commit()`, pending references are not automatically resolved to actual pointers. Transformations typically don't need to access created objects after commit. If needed, objects can be looked up by name in the new space.

### Uniquifier Interface

```cpp
class Uniquifier {
public:
    explicit Uniquifier(Netlist* netlist);

    void setPrefix(std::string prefix);  // For auto-naming (default: "uniq_")

    // --- Structural Additions ---

    PendingScalarNetRef addScalarNet(Path context, Name name);
    PendingBusNetRef addBusNet(Path context, Name name,
                                int32_t msb, int32_t lsb);

    PendingInstanceRef addInstance(Path context, Name name,
                                   Design* designToInstantiate);

    PendingScalarDesignTermRef addScalarDesignTerm(Path context,
                                                    Name name, Direction dir);
    PendingBusDesignTermRef addBusDesignTerm(Path context, Name name,
                                              Direction dir, int32_t msb, int32_t lsb);

    // --- Structural Removals ---

    void removeInstance(InstanceOccurrence occ);
    void removeNet(NetOccurrence occ);

    // --- Connectivity (unified for same-level and cross-hierarchy) ---

    void connect(BitNetOccurrence net, BitInstTermOccurrence term);
    void connect(BitNetOccurrence net, BitDesignTermOccurrence term);
    void connect(AnyBitNetRef net, BitInstTermOccurrence term);
    void connect(AnyBitNetRef net, BitDesignTermOccurrence term);

    void disconnect(BitInstTermOccurrence term);
    void disconnect(BitDesignTermOccurrence term);

    // Convenience: connect entire bus
    void connectBus(BusNetOccurrence net, BusInstTermOccurrence term);
    void connectBus(BusNetOccurrence net, BusDesignTermOccurrence term);

    // --- Design-wide Operations (all instances of a design) ---
    // Note: These do NOT trigger uniquification; changes apply to the Design itself

    void connectInDesign(Design* design, BitNetRef net, BitInstTermRef term);
    void connectInDesign(Design* design, BitNetRef net, BitDesignTermRef term);
    void disconnectInDesign(Design* design, BitInstTermRef term);
    void disconnectInDesign(Design* design, BitDesignTermRef term);

    // --- Attributes ---

    void setAttribute(InstanceOccurrence occ, Name key, AttrValue val);
    void setAttribute(NetOccurrence occ, Name key, AttrValue val);
    // ... etc

    // --- Commit ---

    void commit();  // Apply all changes, create new space, discard old
    void clear();   // Discard pending changes without committing

private:
    Netlist* _netlist;
    std::string _prefix = "uniq_";
    uint32_t _counter = 0;

    struct PendingChanges;
    std::unique_ptr<PendingChanges> _pending;
};
```

### Cross-Hierarchy Punch-Through

When connecting objects at different hierarchical levels, the Uniquifier automatically computes and applies the minimal punch-through:

1. Find LCA of the two paths
2. Punch UP from source to LCA (create output ports at each level)
3. Punch DOWN from LCA to target (create input ports at each level)
4. Create intermediate nets at each level
5. Uniquify all touched designs

**Naming convention**: Auto-generated names use `prefix + unique_counter` (e.g., `uniq_42`)

**No reuse**: Existing paths are not reused; new ports/nets are always created for clarity and debugging.

**Direction convention**: Output ports going up, input ports going down.

---

## 8. Bus vs Scalar vs Bit System

### Rationale

To be Verilog-compliant, the netlist supports both scalar (single-bit) and bus (multi-bit) nets and terms.

### Inheritance Hierarchy

```
Net:
  BitNet (base for connectable bit-level)
    ├── ScalarNet
    └── BusNetBit
  BusNet (container for BusNetBits)

DesignTerm:
  BitDesignTerm (base for connectable bit-level)
    ├── ScalarDesignTerm
    └── BusDesignTermBit
  BusDesignTerm (container for BusDesignTermBits)

InstTerm:
  BitInstTerm (base for connectable bit-level)
    ├── ScalarInstTerm
    └── BusInstTermBit
  BusInstTerm (container for BusInstTermBits)
```

### Connectivity

- Only bit-level objects (`BitNet`, `BitDesignTerm`, `BitInstTerm`) participate in connectivity
- Bus-level objects contain their bits and provide bus-wide operations
- Buses store `msb` and `lsb` for range information (e.g., `[7:0]` or `[0:7]`)

### Storage Strategy

Separate containers per type for cache efficiency:

```cpp
struct NetlistSpace {
    std::vector<ScalarNet> scalarNets;
    std::vector<BusNet> busNets;
    std::vector<BusNetBit> busNetBits;
    // ... similarly for terms
};
```

Iteration helpers:
- Type-specific: `design.scalarNets()`, `design.busNets()`
- Combined bit-level: `design.bitNets()` (iterates ScalarNets + all BusNetBits)

---

## 9. Netlist Dump and Load

### Serialization Format

- **Cap'n Proto** is used for netlist serialization
- Schema-based, fast, compact, supports schema evolution
- Full netlist dump (no incremental/delta support initially)
- Option to dump by Design for partial serialization

### Separate Classes

Dump and load functionality is delegated to specialized classes:

```cpp
class NetlistLoader {
public:
    explicit NetlistLoader(Netlist* netlist);
    void load(const std::filesystem::path& file);

    // Or static factory
    static std::unique_ptr<Netlist> load(const std::filesystem::path& file);

private:
    Netlist* _netlist;
};

class NetlistDumper {
public:
    explicit NetlistDumper(const Netlist* netlist);

    void dump(const std::filesystem::path& file) const;
    void dumpDesign(const std::filesystem::path& file, Design* design) const;

private:
    const Netlist* _netlist;
};
```

### NetlistBuilder for Initial Construction

For building the initial netlist (before any transformations):

```cpp
class NetlistBuilder {
public:
    explicit NetlistBuilder(Netlist* netlist);

    // Design creation
    Design* createDesign(Name name);
    void setTopDesign(Design* design);

    // Object creation within a design
    ScalarNet* addScalarNet(Design* design, Name name);
    BusNet* addBusNet(Design* design, Name name, int32_t msb, int32_t lsb);

    Instance* addInstance(Design* design, Name name, Design* instDesign);

    ScalarDesignTerm* addScalarDesignTerm(Design* design, Name name, Direction dir);
    BusDesignTerm* addBusDesignTerm(Design* design, Name name, Direction dir,
                                     int32_t msb, int32_t lsb);

    // Connectivity
    void connect(BitNet* net, BitInstTerm* term);
    void connect(BitNet* net, BitDesignTerm* term);

    // Finalize (creates the initial space)
    void finalize();

private:
    Netlist* _netlist;
};
```

---

## 10. Compaction

### Compactor Class

All compaction logic resides in the `Compactor` class. No compaction helpers exist on `ChunkedSpan` or `Netlist`.

```cpp
class Compactor {
public:
    explicit Compactor(Netlist* netlist);

    void compactFull();                    // Entire netlist
    void compactDesign(Design* design);    // Single design only

    // Query methods
    bool needsCompaction() const;                    // Any design at chunk limit?
    bool needsCompaction(Design* design) const;     // Specific design at limit?

private:
    Netlist* _netlist;
};
```

### Behavior

- Compaction creates a new space with all objects stored contiguously
- Old spaces are discarded after compaction
- All `ChunkedSpan`s are reduced to single spans
- Pointers are updated to point to new locations
- Caller explicitly decides when to compact (no auto-compaction)

---

## 11. Flat Netlist

### Purpose

A flat netlist is constructed on demand for transformations that work on a flattened view (e.g., multi-driver resolution). It is a separate class from `Netlist`.

### Design

```cpp
class FlatNetlist {
public:
    static std::unique_ptr<FlatNetlist> create(const Netlist& netlist);

    struct FlatBitNet {
        uint32_t id;
        Name name;
        BitNetOccurrence origin;  // Provenance to hierarchical
        std::span<FlatBitInstTerm*> connectedTerms;
    };

    struct FlatBitInstTerm {
        uint32_t id;
        BitInstTermOccurrence origin;
        FlatBitNet* connectedNet;
    };

    struct FlatInstance {
        uint32_t id;
        InstanceOccurrence origin;
        std::span<FlatBitInstTerm> instTerms;
    };

    // Contiguous storage
    std::span<FlatBitNet> nets() const;
    std::span<FlatInstance> instances() const;
    std::span<FlatBitInstTerm> instTerms() const;

private:
    std::vector<FlatBitNet> _nets;
    std::vector<FlatInstance> _instances;
    std::vector<FlatBitInstTerm> _instTerms;
};
```

### Characteristics

- Constructed fresh for each transformation (fast construction is feasible)
- **No caching**: fresh construction per transformation is sufficient
- **Pure bit-level**: no bus structure preserved; bus bits appear as individual `FlatBitNet`
- Flat objects carry provenance back to hierarchical objects
- Uses contiguous containers for cache efficiency
- Bidirectional connectivity at flat level

---

## 12. EquipotentialExplorator

### Purpose

The `EquipotentialExplorator` traverses connectivity across hierarchy to find all electrically connected points (the equipotential) starting from a net or terminal. This is essential for:
- Multi-driver detection and resolution
- Timing analysis
- Clock tree analysis
- Connectivity verification

### Exploration Semantics

An equipotential is the set of all electrically connected points across hierarchy. Starting from a net or term, the exploration:
1. Follows connectivity through hierarchical instances (InstTerm → DesignTerm inside → internal net → recurse)
2. Follows connectivity upward through ports (DesignTerm → InstTerm on parent → net at parent level → recurse)
3. Stops at:
   - **Primitive InstTerms**: terminals on leaf instances (no further hierarchy inside)
   - **Boundary DesignTerms**: ports of the bounding design

### Bounding Box

Two modes of operation:
- **Bounded**: A `Design` is specified as a hierarchical bounding box. Exploration stops at its ports.
- **Unbounded**: No bound specified, equivalent to bounding at the top design.

### ExploreDirection Enum

`Direction` enum (Input, Output, InOut) is defined in section 4. For exploration:

```cpp
enum class ExploreDirection {
    Bidirectional,  // All connections (default)
    ToLoads,        // Fanout: follow Output/InOut → Input/InOut
    ToDrivers,      // Fanin: follow Input/InOut → Output/InOut
};
```

### Result Types

```cpp
// Endpoint: primitive InstTerm or boundary DesignTerm
struct EquipotentialEndpoint {
    std::variant<BitInstTermOccurrence, BitDesignTermOccurrence> endpoint;

    bool isPrimitiveInstTerm() const;
    bool isBoundaryDesignTerm() const;

    BitInstTermOccurrence asPrimitiveInstTerm() const;
    BitDesignTermOccurrence asBoundaryDesignTerm() const;
};

// Net on the equipotential (for exploreNets)
struct EquipotentialNet {
    BitNetOccurrence net;
};
```

### Range Types

```cpp
class EquipotentialEndpointRange {
public:
    class Iterator {
    public:
        using value_type = EquipotentialEndpoint;
        using reference = const EquipotentialEndpoint&;

        Iterator& operator++();
        reference operator*() const;
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;
    };

    Iterator begin() const;
    Iterator end() const;

    // Convenience: collect all endpoints into a vector
    std::vector<EquipotentialEndpoint> collect() const;
};

class EquipotentialNetRange {
public:
    class Iterator { /* similar to above */ };

    Iterator begin() const;
    Iterator end() const;

    std::vector<EquipotentialNet> collect() const;
};
```

### EquipotentialExplorator Class

```cpp
class EquipotentialExplorator {
public:
    // Bounded exploration (stops at boundingDesign's ports)
    explicit EquipotentialExplorator(const Netlist* netlist, Design* boundingDesign);

    // Unbounded exploration (bounding at top design)
    explicit EquipotentialExplorator(const Netlist* netlist);

    // --- Explore endpoints (primitives and boundary ports) ---

    EquipotentialEndpointRange explore(
        BitNetOccurrence start,
        ExploreDirection dir = ExploreDirection::Bidirectional) const;

    EquipotentialEndpointRange explore(
        BitInstTermOccurrence start,
        ExploreDirection dir = ExploreDirection::Bidirectional) const;

    EquipotentialEndpointRange explore(
        BitDesignTermOccurrence start,
        ExploreDirection dir = ExploreDirection::Bidirectional) const;

    // --- Explore intermediate nets ---

    EquipotentialNetRange exploreNets(
        BitNetOccurrence start,
        ExploreDirection dir = ExploreDirection::Bidirectional) const;

    EquipotentialNetRange exploreNets(
        BitInstTermOccurrence start,
        ExploreDirection dir = ExploreDirection::Bidirectional) const;

    EquipotentialNetRange exploreNets(
        BitDesignTermOccurrence start,
        ExploreDirection dir = ExploreDirection::Bidirectional) const;

private:
    const Netlist* _netlist;
    Design* _boundingDesign;  // nullptr means top design
};
```

### Exploration Direction Semantics

| ExploreDirection | From Output port | From Input port | From InOut port |
|------------------|------------------|-----------------|-----------------|
| Bidirectional | Follow all | Follow all | Follow all |
| ToLoads | Follow (driving) | Stop (not a driver) | Follow (can drive) |
| ToDrivers | Stop (not a load) | Follow (is a load) | Follow (can be driven) |

### Visited Tracking

- Visited tracking is internal to each `explore()` call
- Reset per exploration; no shared state across calls
- Prevents infinite loops in combinational cycles

### Usage Examples

```cpp
EquipotentialExplorator explorator(netlist, cpuDesign);

BitNetOccurrence clockNet = { path, clkNet };

// Find all endpoints on the clock equipotential
for (const auto& endpoint : explorator.explore(clockNet)) {
    if (endpoint.isPrimitiveInstTerm()) {
        auto termOcc = endpoint.asPrimitiveInstTerm();
        Instance* inst = termOcc.bitInstTerm->instance;  // Back pointer to parent
        if (PrimitiveLibrary::isFlipFlop(inst->primitiveKind())) {
            // Found a flip-flop clock input
        }
    } else {
        auto termOcc = endpoint.asBoundaryDesignTerm();
        // Signal exits the bounding design here
    }
}

// Find all nets on an equipotential (for debugging/analysis)
for (const auto& net : explorator.exploreNets(startNet)) {
    std::cout << "Net: " << netlist->nameTable()->getString(net.net.bitNet->name) << "\n";
}

// Fanout analysis: find all loads driven by this output
for (const auto& endpoint : explorator.explore(driverTerm, ExploreDirection::ToLoads)) {
    // Process load terminals
}
```

---

## 13. Synchronization Model

- Synchronization occurs at a higher level (space barriers), not on individual objects
- No atomics on `Instance::model` pointer
- Level-by-level processing with barriers between levels
- Within a level, parallel work can proceed without locking
- Changes are collected per-worker, then merged at commit

---

## 14. Primary Transformation Use Cases

| Transformation | Objects Touched | Structural Changes |
|---------------|-----------------|-------------------|
| Attribute change | Many | None |
| Multi-driver resolution (MDR) | Few nets | Adds buffers, rewires |
| Clocking (gated/derived clocks) | Variable | Adds logic, rewires |
| Loadless logic removal | Variable | Deletes instances |
| Retiming | Localized | Moves registers, heavy rewiring |

---

## 15. Design Decisions

This section documents decisions made on previously open questions.

### Storage and Indexing

1. **InstTerm indexing with buses**: **Hierarchical indexing**
   - `ScalarInstTerm` stores `scalarTermIndex` (index into `scalarDesignTerms`)
   - `BusInstTermBit` stores `busTermIndex` + `bitIndex` (index into `busDesignTerms`, then bit within)
   - Rationale: Stable indices when adding terms; explicit structure

2. **BusNetBit storage**: **Global container in space**
   - All `BusNetBit` objects stored in `NetlistSpace::busNetBits`
   - `BusNet::bits` is a `ChunkedSpan` into this container
   - Rationale: Consistency with other object types; simpler space management

### Flat Netlist

3. **Bus structure preservation**: **Pure bit-level (no bus structure)**
   - Flat netlist contains only `FlatBitNet`, no `FlatBusNet`
   - Bus bits appear as individual `FlatBitNet` with names like `"bus[3]"`
   - Rationale: Simpler implementation; sufficient for MDR and other transformations

### Attributes

4. **AttributeTable location**: **In Netlist (not in space)**
   - `AttributeTable` is owned by `Netlist`, persists across spaces
   - Uniquifier updates attributes during commit
   - Rationale: Attributes are sparse; no benefit from space-based immutability

### API Details

5. **Design-wide operations (`connectInDesign`)**: **No uniquification**
   - Changes apply to the Design itself, not to specific occurrences
   - All instances of the Design see the change
   - No uniquification triggered because the change is not context-specific
   - Use occurrence-based `connect()` for context-specific changes

6. **Pending reference resolution**: **Caller doesn't need resolution**
   - Transformations typically don't need to access created objects after commit
   - If needed, objects can be looked up by name in the new space
   - No resolution map or post-commit resolve API required

### Serialization

7. **Netlist file format**: **Cap'n Proto**
   - Schema-based serialization with Cap'n Proto
   - Fast, compact, supports schema evolution
   - External dependency acceptable

8. **Incremental save**: **Full netlist only (for now)**
   - Dump full netlist, with option to dump by Design
   - No delta/patch support initially
   - Can be revisited if needed for version control or checkpointing

### Performance

9. **Compaction helpers**: **In Compactor class only**
   - No `compactIfNeeded()` on `ChunkedSpan` or `Netlist`
   - All compaction logic resides in the `Compactor` class
   - Caller explicitly decides when to compact

10. **Flat netlist caching**: **No caching**
    - Flat netlist constructed fresh for each transformation
    - Fast construction is sufficient; no caching needed
    - Simpler design; no cache invalidation concerns

---

## 16. Next Steps

1. Define `AttributeValue` type and its supported value types
2. Design the iteration API for combined bit-level traversal (`bitNets()`, `bitDesignTerms()`, etc.)
3. Consider error handling strategy (exceptions vs error codes per CODING_STYLE.md)
4. Define Cap'n Proto schema for netlist serialization
5. Implement core data structures (Name, NameTable, IDs, basic objects)
6. Implement NetlistSpace and ChunkedSpan (including `data()` for single-chunk access)
7. Implement Library and Design structures
8. Implement Netlist with basic iteration
9. Implement PrimitiveLibrary and primitive pin index namespaces
10. Implement NetlistBuilder for initial construction
11. Implement Uniquifier
12. Implement Compactor
13. Implement FlatNetlist
14. Implement EquipotentialExplorator
15. Implement NetlistLoader and NetlistDumper with Cap'n Proto
