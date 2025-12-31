#include <stdio.h>
#include <windows.h>

void *trampoline = NULL;
SIZE_T patchSize = 8; // 5 7 8 bytes works

static const char msg[] = "Hooked %d\n";
static int i = 0;

__attribute__((naked))
void Hook()
{
    __asm__ __volatile__(
        ".intel_syntax noprefix\n"

        // 32 bytes shadow space + 32 pour save nos args/registres
        "sub  rsp, 0x40\n"

        "inc  DWORD PTR i[rip]\n"

        // sauvegarder les arguments de la fonction hookée
        "mov  [rsp+0x20], rcx\n"
        "mov  [rsp+0x28], rdx\n"
        "mov  [rsp+0x30], r8\n"
        "mov  [rsp+0x38], r9\n"

        // printf(\"Hooked\n\")
        "lea  rcx, msg[rip]\n"
        "mov  edx, i[rip]\n"
        "xor  eax, eax\n"
        "call printf\n"

        // restaurer les arguments d'origine
        "mov  rcx, [rsp+0x20]\n"
        "mov  rdx, [rsp+0x28]\n"
        "mov  r8,  [rsp+0x30]\n"
        "mov  r9,  [rsp+0x38]\n"

        "add  rsp, 0x40\n"

        // sauter vers le trampoline
        "mov  rax, trampoline[rip]\n"
        "jmp  rax\n"

        ".att_syntax prefix\n"
    );
}





BOOL APIENTRY DllMain(HMODULE hMod, DWORD ul_reason_for_call, LPVOID lpReserved) {


    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        HMODULE u32 = LoadLibraryA("user32.dll");
        if(!hMod) {
            printf("[ERROR] Load Library, %lu\n", GetLastError());
            return 1;
        }

        BYTE *originalAddr = (BYTE*)GetProcAddress(u32, "MessageBoxA");
        if(!originalAddr) {
            printf("[ERROR] No Original Address, %lu\n", GetLastError());
        }

        trampoline = VirtualAlloc(
            NULL,
            patchSize + 12, // 7 + 12 | 7 bcs 7 bytes for the instruction & 12 for the absolute jmp
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );
        if(!trampoline) {
            printf("[ERROR] Trampoline, %lu\n", GetLastError());
            return 1;
        }
        memcpy(trampoline, originalAddr, patchSize);

        BYTE *continueAddr = (BYTE*)originalAddr + patchSize;   // like its jmp -> hook -> trampo -> contniueAddr so as the patch size is 7
        BYTE *trampolineAddr = (BYTE*)trampoline + patchSize;

        trampolineAddr[0] = 0x48;
        trampolineAddr[1] = 0xB8;
        memcpy(trampolineAddr + 2, &continueAddr, 8); // 8 bytes bcs imm64
        trampolineAddr[10] = 0xFF;
        trampolineAddr[11] = 0xE0; // jmp rax

        DWORD oldProtect;
        VirtualProtect(originalAddr, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect);

        intptr_t offset = (intptr_t)Hook - (intptr_t)(originalAddr+5);

        originalAddr[0] = 0xE9;
        memcpy(originalAddr+1, &offset, 4);

        memset(originalAddr + 5, 0x90, 2);

        VirtualProtect(originalAddr, 7, oldProtect, &oldProtect);


        break;
    
    default:
        break;
    }

    return TRUE;


}