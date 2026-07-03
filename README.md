# Inline Hook with Trampoline (C + Inline x64 Assembly)

A Windows DLL that hooks `user32.dll!MessageBoxA` via inline hooking: patching the target function's prologue to redirect execution to a custom handler, then jumping back to the original code through a trampoline.

## How it works

### 1. Patching the target function

On `DLL_PROCESS_ATTACH`, the DLL resolves `MessageBoxA`'s address via `GetProcAddress`, then overwrites its first bytes with a relative jump (`0xE9` + a 4-byte relative offset) to the `Hook()` function, followed by two `0x90` (`NOP`) padding bytes to fill out the patch region.

### 2. Preserving the original bytes: the trampoline

Before patching, the original first bytes of `MessageBoxA` are copied into a separately allocated executable buffer (`VirtualAlloc` with `PAGE_EXECUTE_READWRITE`), the **trampoline**. After the copied bytes, an absolute jump (`0x48 0xB8` = `mov rax, imm64`, then `0xFF 0xE0` = `jmp rax`) is appended, pointing back to the address right after the patched region in the original function. This lets execution resume the original function's logic after the hook runs, without needing the original bytes still in place.

### 3. The hook itself (`Hook`, naked function + inline ASM)

`Hook()` is declared `__attribute__((naked))`, meaning the compiler generates no prologue/epilogue, the function's entire body is hand-written Assembly, giving full control over the stack and registers.

Key details handled manually:
- **Windows x64 calling convention**: the first four integer/pointer arguments are passed in `RCX`, `RDX`, `R8`, `R9`. Since the hook needs to call `printf` (which will clobber these registers), the original arguments are saved to the stack before the call and restored right after.
- **Shadow space**: the Windows x64 ABI reserves 32 bytes of "shadow space" on the stack for any call, even if the callee doesn't use all four register-passed arguments. `sub rsp, 0x40` reserves 32 bytes of shadow space *plus* 32 bytes to save the four original argument registers before calling `printf`.
- **Stack alignment**: the ABI requires `RSP` to be 16-byte aligned at the point of a `call` instruction, but `call` itself pushes 8 bytes (the return address), so at function entry `RSP % 16 == 8`. The `0x40`-byte adjustment keeps this alignment correct after entering `Hook`.
- **RIP-relative addressing**: static variables (`trampoline`, `i`, `msg`) are accessed via `[label][rip]`, letting the assembler compute the correct offset from the current instruction pointer rather than hardcoding an absolute address (important since the code may be relocated).
- **`.intel_syntax noprefix` / `.att_syntax prefix`**: GCC's inline ASM defaults to AT&T syntax; the `.intel_syntax noprefix` directive switches to Intel syntax for readability, then switches back at the end of the block so the rest of the file (if any) isn't affected.

## Build

```bash
gcc -shared -o hook.dll main.c
```

(Load the resulting DLL into a target process, e.g. via a DLL injector, to trigger the hook.)

## What I learned

- Windows x64 calling convention: register usage (`RCX`/`RDX`/`R8`/`R9`) and why the shadow space exists.
- Why the `0x28`/`0x40`-style stack adjustments in hand-written prologues aren't arbitrary, they come directly from the 16-byte alignment requirement combined with the 8 bytes pushed by `call`.
- Writing inline Assembly with GCC (`__asm__ __volatile__`), including the Intel vs AT&T syntax difference.
- What `__attribute__((naked))` actually removes (compiler-generated prologue/epilogue) and why that matters when you need exact control over the stack layout.
- Why the hook has to save and restore the original function's arguments: without it, using those registers for the `printf` call (e.g. loading `msg` into `RCX`) overwrites the arguments the original function needs, breaking it right after the jump back.
- RIP-relative addressing: computing the offset from the current instruction address to reach a static variable, rather than using an absolute address.

## Known issues

- In `DllMain`, the error check after `LoadLibraryA("user32.dll")` tests `!hMod` instead of `!u32`, likely a copy-paste typo, since `hMod` is the DLL's own module handle, not the result of `LoadLibraryA`.
- `patchSize` is set to `8`, but the actual patch written is 7 bytes (a 5-byte relative `jmp` + 2 `NOP` bytes). Worth double-checking that this doesn't accidentally read/copy one byte too many into the trampoline.

## Next steps

- Fix the two issues above.
- Generalize the hook to work on arbitrary target functions rather than being hardcoded to `MessageBoxA`.
- Compare against the [C++ version of this project](https://github.com/AledMikuinIt/hook-inline-trampoline-cpp), same technique, different implementation approach.
