![Image](https://github.com/user-attachments/assets/f8bca361-1337-43a0-bc20-2e7e317267f6)

# SymFromModuleOffset

A lightweight Windows command-line utility for resolving symbols and source information directly from **Microsoft Program Database (PDB)** files using the **Debug Interface Access (DIA) SDK**.

The application can:

- Retrieve the **Debug ID** (PDB GUID + Age)
- Resolve one or more **Relative Virtual Addresses (RVAs)** to symbol names
- Display the corresponding **source file** and **line number**
- Accept both hexadecimal and decimal offsets
- Operate without loading the original executable

This makes it useful for crash analysis, symbol servers, debugging tools, profiler development, and offline symbol inspection.

---

## Features

- Resolve symbols directly from a PDB file
- Extract PDB Debug ID (GUID + Age)
- Resolve multiple addresses in a single invocation
- Display:
  - function or symbol name
  - source filename
  - source line number
- Supports both:
  - hexadecimal offsets (`0x12345`)
  - decimal offsets (`74565`)
- Simple command-line interface
- Modern C++ implementation
- No external runtime dependencies beyond Microsoft's DIA SDK

---

# Example

Retrieve the PDB Debug ID:

```text
SymFromModuleOffset.exe mymodule.pdb
```

Example output:

```text
Debug ID:
{A1B2C3D4-E5F6-1122-3344-556677889900}-1
```

Resolve symbols:

```text
SymFromModuleOffset.exe mymodule.pdb 0x4F120 0x50A00
```

Example output:

```text
Symbol:
MyNamespace::Renderer::DrawFrame

Source File:
renderer.cpp

Line:
418
```

---

# Technical Overview

The application is built on top of the **Microsoft DIA (Debug Interface Access)** API.

Typical workflow:

```
PDB file
      │
      ▼
IDiaDataSource
      │
      ▼
loadDataFromPdb()
      │
      ▼
IDiaSession
      │
      ├──────────────► get_globalScope()
      │                     │
      │                     ▼
      │                GUID + Age
      │
      ▼
getSymbolsByAddr()
      │
      ▼
symbolByRVA()
      │
      ▼
IDiaSymbol
      │
      ▼
findLinesByRVA()
      │
      ▼
source file + line number
```

---

# Architecture

The project consists of several small helper components.

## SymbolInfo

Stores the resolved symbol information.

```cpp
struct SymbolInfo
{
    std::string symbolName;
    std::string sourceFile;
    DWORD lineNumber;
};
```

---

## DebugId

Represents the PDB identity.

```cpp
struct DebugId
{
    GUID pdbGuid;
    DWORD pdbAge;
};
```

This information uniquely identifies a PDB and is compatible with symbol server indexing.

---

## GetDebugId()

Loads a PDB and retrieves:

- GUID
- Age

Internally it:

1. Creates a DIA data source.
2. Loads the PDB.
3. Opens a DIA session.
4. Obtains the global scope.
5. Reads the GUID.
6. Reads the Age.

---

## GetSymbolFromOffset()

Resolves an RVA to source information.

Steps:

1. Load PDB.
2. Open DIA session.
3. Obtain `IDiaEnumSymbolsByAddr`.
4. Locate the symbol using `symbolByRVA()`.
5. Retrieve:
   - symbol name
   - source file
   - line number

---

## GuidToString()

Converts a Windows `GUID` into the standard textual representation using:

- `StringFromGUID2`
- `WideCharToMultiByte`

The result is suitable for logging and symbol server identifiers.

---

# Command Line

## Print Debug ID

```text
SymFromModuleOffset.exe file.pdb
```

---

## Resolve addresses

```text
SymFromModuleOffset.exe file.pdb 0x12345
```

or

```text
SymFromModuleOffset.exe file.pdb 74565
```

Multiple addresses are supported:

```text
SymFromModuleOffset.exe file.pdb 0x1000 0x2000 0x3000
```

---

# Technologies

- C++17
- Windows API
- COM
- ATL Smart Pointers (`CComPtr`)
- Microsoft DIA SDK
- OLE Automation (`_bstr_t`)
- Visual Studio

---

# COM Usage

The application uses standard COM initialization:

```cpp
CoInitialize(nullptr);
```

and creates DIA objects via

```cpp
CoCreateInstance()
```

wrapped by ATL's `CComPtr`, providing automatic lifetime management and preventing COM interface leaks.

---

# Error Handling

The application validates every significant DIA operation:

- COM object creation
- PDB loading
- session creation
- symbol enumeration
- source line lookup
- GUID retrieval
- symbol lookup

Errors are reported to the console, and lookup functions return `std::optional` to indicate failure without relying on exceptions.

---

# Modern C++ Features

The implementation makes use of several modern C++ facilities:

- `std::optional`
- RAII
- smart COM pointers (`CComPtr`)
- `std::string`
- `std::ostringstream`
- `std::wstringstream`
- structured helper functions
- automatic resource cleanup

---

# Typical Use Cases

- Offline crash dump analysis
- Symbol server validation
- PDB inspection utilities
- Build verification
- Address-to-symbol conversion
- Debug tooling
- Performance profilers
- Custom stack trace generators
- Reverse engineering support
- Automated symbol lookup scripts

---

# Advantages

- Small and self-contained
- No executable image required
- Works directly with PDB files
- Supports multiple address lookups per execution
- Retrieves precise source-level information
- Uses Microsoft's official debugging interfaces
- Easy to integrate into larger debugging or diagnostics pipelines

---

# Build Requirements

- Windows
- Visual Studio
- Microsoft DIA SDK (`dia2.h`)
- ATL
- COM support
- `comsuppw.lib`

---

# Possible Future Improvements

- Support Virtual Addresses (VA) in addition to RVAs
- Automatic PE image loading and RVA translation
- Demangled symbol formatting options
- Batch processing from text files
- JSON/XML output for tool integration
- Symbol server (`symsrv.dll`) support
- Module base address handling
- Function size and displacement reporting
- Inline function resolution
- Public symbol enumeration
- Source checksum reporting
- Export as a reusable library
