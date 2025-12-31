# Hook w/ Trampoline & Inline ASM for args/registers & jmp rax

# What Ive learned :

- need to care abt shadow space (32 bytes) of Windows convention
- ABI alignement w/ shadowspace, so its 32 + 8 (0x28 in hex) bcs ABI is aligned on 16 but need 8 bcs call push on 8 bytes so RSP % 16 = 8
- How to write inline asm (in fucking intel_syntax idk why there is so much diff syntaxes but w.e)
- What NAKED is abt = just say to the compiler (divine intellect, RIP terry davis) that i will write all the prologue/epilogue
- volatile stuff to not optimise
- Did again a hook w/ trampoline but in C and calling the trampoline (original function) via ASM
- Save the args, so like bcs lea rcx msg is for printf, that means that it overwrites the stack so crash on original func after
- How to deal with RIP, its just calculate the offset from a the static variable address

I'm a moron it's so w.e ~