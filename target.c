#include <windows.h>

int main() {

    int i = 0;
    while(i < 3) {
    MessageBoxA(
        NULL,
        "Hello",
        "Original",
        MB_OK
    );

    Sleep(3000);
    i+=1;
    }
    


    return 0;
}