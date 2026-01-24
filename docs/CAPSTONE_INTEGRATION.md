# Capstone Disassembler Integration

## Overview

ChakraSilicon integrates the Capstone disassembly engine to provide real-time JIT assembly tracing and analysis. Capstone is a lightweight, multi-platform, multi-architecture disassembly framework that supports both x86-64 and ARM64 architectures.

## Purpose

The Capstone integration enables:

1. **JIT Code Analysis** - Disassemble and analyze generated machine code in real-time
2. **Performance Debugging** - Understand JIT compiler output and optimization patterns
3. **Architecture Support** - Unified disassembly interface for both x86-64 and ARM64
4. **Code Verification** - Validate generated assembly on different platforms

## Architecture Support

### Supported Architectures

| Architecture | Capstone Mode | ChakraCore Target | Status |
|--------------|---------------|-------------------|---------|
| x86-64       | CS_ARCH_X86 + CS_MODE_64 | CC_TARGETS_AMD64 | ✅ Fully Supported |
| ARM64        | CS_ARCH_ARM64 + CS_MODE_ARM | CC_TARGETS_ARM64 | ✅ Fully Supported |
| x86 32-bit   | CS_ARCH_X86 + CS_MODE_32 | CC_TARGETS_X86 | ⚠️ Limited Support |
| ARM 32-bit   | CS_ARCH_ARM + CS_MODE_ARM | CC_TARGETS_ARM | ⚠️ Limited Support |

### Architecture Detection

The integration automatically detects the target architecture at compile time:

```cpp
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
    // x86-64: Intel/AMD 64-bit
    arch = CS_ARCH_X86;
    mode = CS_MODE_64;
    
#elif defined(_M_ARM64) || defined(__aarch64__) || defined(__arm64__)
    // ARM64: Apple Silicon, ARM64 servers
    arch = CS_ARCH_ARM64;
    mode = CS_MODE_ARM;
    
#elif defined(_M_IX86) || defined(__i386__)
    // x86 32-bit (legacy)
    arch = CS_ARCH_X86;
    mode = CS_MODE_32;
    
#elif defined(_M_ARM) || defined(__arm__)
    // ARM 32-bit (legacy)
    arch = CS_ARCH_ARM;
    mode = CS_MODE_ARM;
#endif
```

## Build Configuration

### CMake Integration

Capstone is configured via `ChakraCore/deps/capstone_integration.cmake`:

```cmake
# Architecture-specific enablement
if(CC_TARGETS_AMD64)
    set(CAPSTONE_X86_SUPPORT ON)
    set(CAPSTONE_ARM64_SUPPORT OFF)
    
elseif(CC_TARGETS_ARM64)
    set(CAPSTONE_X86_SUPPORT OFF)
    set(CAPSTONE_ARM64_SUPPORT ON)
    
else()
    # Universal build: enable both
    set(CAPSTONE_X86_SUPPORT ON)
    set(CAPSTONE_ARM64_SUPPORT ON)
endif()
```

### Build Targets and Capstone

| Build Target | Architecture | Capstone Mode | Enabled Features |
|--------------|--------------|---------------|------------------|
| chintx64     | x86_64       | X86_64        | Intel syntax, x64 registers |
| chjitx64     | x86_64       | X86_64        | Intel syntax, x64 registers |
| chinta64     | arm64        | ARM64         | ARM syntax, ARM64 registers |
| chjita64     | arm64        | ARM64         | ARM syntax, ARM64 registers |

### Compiler Definitions

The CMake configuration sets preprocessor definitions:

- `ENABLE_CAPSTONE_DISASM=1` - Capstone is available
- `CAPSTONE_HAS_X86=1` - x86-64 support enabled
- `CAPSTONE_HAS_ARM64=1` - ARM64 support enabled

## Runtime Usage

### Initialization

```cpp
JitAsmTracer tracer;
// Capstone initialized automatically based on architecture
```

### Disassembling Code

```cpp
void JitAsmTracer::TraceFunction(Func* func, void* codeAddress, size_t codeSize)
{
    // Automatically uses correct architecture
    DisassembleCode(codeAddress, codeSize, functionName);
}
```

### Example Output

#### x86-64 (Intel Syntax)
```
Function: MyFunction
Address: 0x7f8a2c001000, Size: 256 bytes

0x7f8a2c001000:  push    rbp
0x7f8a2c001001:  mov     rbp, rsp
0x7f8a2c001004:  sub     rsp, 0x20
0x7f8a2c001008:  mov     rax, qword ptr [rcx]
0x7f8a2c00100b:  call    0x7f8a2c002000
```

#### ARM64 (ARM Syntax)
```
Function: MyFunction
Address: 0x104a00000, Size: 256 bytes

0x104a00000:  stp     x29, x30, [sp, #-0x10]!
0x104a00004:  mov     x29, sp
0x104a00008:  sub     sp, sp, #0x20
0x104a0000c:  ldr     x0, [x0]
0x104a00010:  bl      0x104b00000
```

## Implementation Details

### File Structure

```
ChakraCore/
├── deps/
│   ├── capstone/                    # Capstone source code
│   │   ├── include/capstone/        # Public headers
│   │   ├── arch/
│   │   │   ├── X86/                 # x86-64 disassembler
│   │   │   └── AArch64/             # ARM64 disassembler
│   │   └── CMakeLists.txt
│   └── capstone_integration.cmake   # ChakraCore integration
└── lib/Backend/
    ├── JitAsmTrace.h                # Tracer interface
    └── JitAsmTrace.cpp              # Implementation with Capstone
```

### Key Components

#### 1. Architecture Detection (`JitAsmTrace.cpp`)

```cpp
bool JitAsmTracer::InitializeCapstone()
{
    cs_arch arch;
    cs_mode mode;
    
    // Compile-time architecture detection
    #if defined(__x86_64__)
        arch = CS_ARCH_X86;
        mode = CS_MODE_64;
    #elif defined(__aarch64__)
        arch = CS_ARCH_ARM64;
        mode = CS_MODE_ARM;
    #endif
    
    cs_open(arch, mode, &m_capstoneHandle);
}
```

#### 2. Disassembly Engine

```cpp
bool JitAsmTracer::DisassembleCode(void* codeAddress, size_t codeSize)
{
    cs_insn* instructions;
    size_t count = cs_disasm(m_capstoneHandle,
                              static_cast<const uint8_t*>(codeAddress),
                              codeSize,
                              reinterpret_cast<uint64_t>(codeAddress),
                              0, &instructions);
    
    // Process instructions...
    cs_free(instructions, count);
}
```

#### 3. Instruction Analysis

```cpp
void JitAsmTracer::AnalyzeInstruction(const cs_insn* insn, InstructionInfo& info)
{
    // Extract mnemonic and operands
    strcpy(info.mnemonic, insn->mnemonic);
    strcpy(info.operands, insn->op_str);
    
    // Analyze instruction type (architecture-agnostic)
    info.isBranch = /* detect branches */;
    info.isCall = /* detect calls */;
    info.isRet = /* detect returns */;
}
```

## Platform-Specific Considerations

### x86-64 (Intel/AMD)

**Syntax Options:**
- Intel syntax (default): `mov rax, rbx`
- AT&T syntax (optional): `movq %rbx, %rax`

**Register Set:**
- General purpose: RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8-R15
- XMM registers: XMM0-XMM15
- Flags: RFLAGS

**Instruction Sets:**
- SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2
- AVX, AVX2, AVX-512 (if supported)
- BMI, BMI2, FMA

### ARM64 (Apple Silicon, ARM Servers)

**Syntax:**
- ARM syntax (standard): `ldr x0, [x1]`

**Register Set:**
- General purpose: X0-X30 (64-bit), W0-W30 (32-bit)
- Special registers: SP (stack pointer), LR (link register), XZR/WZR (zero)
- Vector registers: V0-V31, D0-D31, S0-S31

**Instruction Sets:**
- Base ARM64
- NEON SIMD
- Cryptographic extensions (if available)
- Apple-specific extensions (on M1/M2/M3)

### Apple Silicon Specifics

**Platform Detection:**
```cpp
#if defined(__APPLE__) && defined(__aarch64__)
    // Apple Silicon detected
    #define APPLE_SILICON 1
#endif
```

**Special Considerations:**
- JIT requires special entitlements on macOS
- Some instruction patterns prohibited (STP/LDP with certain offsets)
- W^X memory protection

## Configuration Options

### Enable/Disable Capstone

```cmake
# In CMakeLists.txt or via command line
option(ENABLE_JIT_ASM_TRACE "Enable JIT assembly tracing" ON)
```

### CMake Build Examples

```bash
# x86-64 with Capstone
cmake -DENABLE_JIT_ASM_TRACE=ON -DCMAKE_SYSTEM_PROCESSOR=x64 ..

# ARM64 with Capstone
cmake -DENABLE_JIT_ASM_TRACE=ON -DCMAKE_SYSTEM_PROCESSOR=arm64 ..

# Disable Capstone
cmake -DENABLE_JIT_ASM_TRACE=OFF ..
```

## Troubleshooting

### Capstone Not Found

**Symptom:**
```
Capstone: Source not found at deps/capstone - JIT assembly tracing disabled
```

**Solution:**
Ensure the capstone submodule/directory exists:
```bash
ls ChakraCore/deps/capstone/
# Should show CMakeLists.txt and source files
```

### Architecture Mismatch

**Symptom:**
Disassembly produces garbage output or crashes.

**Solution:**
Verify that the build architecture matches the runtime architecture:
```bash
# Check compiled architecture
file dist/chjitx64/ch
# Should show: x86_64 or arm64

# Check Capstone configuration
grep "CAPSTONE.*SUPPORT" ChakraCore/deps/capstone_integration.cmake
```

### Missing Instructions on ARM64

**Symptom:**
Some ARM64 instructions are not recognized.

**Solution:**
Ensure Capstone ARM64 support is enabled:
```cmake
set(CAPSTONE_ARM64_SUPPORT ON CACHE BOOL "ARM64 support" FORCE)
```

### Apple Silicon JIT Issues

**Symptom:**
JIT code crashes or fails on Apple Silicon despite correct disassembly.

**Solution:**
Check for prohibited instruction patterns:
- STP/LDP with large offsets may be restricted
- Ensure W^X compliance
- Verify JIT entitlements are set

## Performance Considerations

### Build Size Impact

| Configuration | Additional Size | Build Time Impact |
|---------------|----------------|-------------------|
| x86-64 only   | ~500 KB        | +5-10 seconds     |
| ARM64 only    | ~400 KB        | +5-10 seconds     |
| Both (universal) | ~900 KB     | +10-15 seconds    |

### Runtime Overhead

- **Disabled:** No overhead (preprocessor-disabled)
- **Enabled:** ~2-5% overhead when actively tracing
- **Recommendation:** Enable only for debug builds or when needed

## Testing

### Verify Architecture Support

```cpp
// Test program
#include <capstone/capstone.h>

int main() {
    csh handle;
    
    // Test x86-64
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) == CS_ERR_OK) {
        printf("x86-64 support: OK\n");
        cs_close(&handle);
    }
    
    // Test ARM64
    if (cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &handle) == CS_ERR_OK) {
        printf("ARM64 support: OK\n");
        cs_close(&handle);
    }
    
    return 0;
}
```

### Build Tests

```bash
# Build for x86-64
./scripts/build_target.sh chjitx64

# Build for ARM64
./scripts/build_target.sh chjita64

# Verify Capstone is linked
otool -L dist/chjitx64/libChakraCore.dylib | grep capstone
# or
ldd dist/chjitx64/libChakraCore.so | grep capstone
```

## Future Enhancements

### Planned Features

1. **Cross-Architecture Disassembly**
   - Disassemble ARM64 code on x86-64 host and vice versa
   - Useful for CI/CD and testing

2. **Enhanced Analysis**
   - Performance cost estimation per instruction
   - Memory bandwidth analysis
   - Cache behavior prediction

3. **Interactive Mode**
   - Real-time disassembly during debugging
   - Breakpoint-triggered disassembly
   - Integration with debuggers (lldb, gdb)

4. **Additional Architectures**
   - RISC-V support (if needed)
   - WebAssembly JIT output analysis

## References

- [Capstone Official Documentation](http://www.capstone-engine.org/)
- [Capstone GitHub Repository](https://github.com/aquynh/capstone)
- [ARM64 Architecture Reference](https://developer.arm.com/documentation/)
- [Intel x86-64 Architecture Reference](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [Apple Silicon Development](https://developer.apple.com/documentation/apple-silicon)

## Version History

- **1.0.0** (2025-01-24) - Initial multi-architecture support (x86-64 and ARM64)
  - Automatic architecture detection
  - Unified disassembly interface
  - Platform-specific optimizations

---

**Last Updated:** January 24, 2025  
**Maintainer:** ChakraSilicon Project  
**Status:** Production Ready