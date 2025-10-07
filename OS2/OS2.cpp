#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

void ErrorExit(const char* msg) {
    std::cerr << msg << std::endl;
    ExitProcess(1);
}

void ChildMode() {
    int size;
    if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), &size, sizeof(size), NULL, NULL)) {
        ErrorExit("Child: Failed to read array size");
    }

    std::vector<int> arr(size);
    DWORD bytesRead;
    if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), arr.data(), size * sizeof(int), &bytesRead, NULL)) {
        ErrorExit("Child: Failed to read array elements");
    }
    int minVal = 2147483647;
    for (int num : arr) {
        if (num < minVal) minVal = num;
    }
    DWORD bytesWritten;
    if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &minVal, sizeof(minVal), &bytesWritten, NULL)) {
        ErrorExit("Child: Failed to write result");
    }
}

void ParentMode() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    int size;
    std::cout << "Enter array size: ";
    std::cin >> size;

    std::vector<int> arr(size);
    std::cout << "Generate randomly? (y/n): ";
    char choice;
    std::cin >> choice;

    if (choice == 'y' || choice == 'Y') {
        for (int i = 0; i < size; i++) {
            arr[i] = std::rand() % 100;
        }
    }
    else {
        std::cout << "Enter " << size << " elements: ";
        for (int i = 0; i < size; i++) {
            std::cin >> arr[i];
        }
    }

    std::cout << "Array: ";
    for (int num : arr) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    HANDLE hPipe1Read, hPipe1Write;
    HANDLE hPipe2Read, hPipe2Write;

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hPipe1Read, &hPipe1Write, &sa, 0)) {
        ErrorExit("CreatePipe failed for Pipe1");
    }
    if (!CreatePipe(&hPipe2Read, &hPipe2Write, &sa, 0)) {
        ErrorExit("CreatePipe failed for Pipe2");
    }

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hPipe1Read;
    si.hStdOutput = hPipe2Write;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi;
    char cmdLine[] = "OS2.exe child";

    if (!CreateProcessA(
        NULL,
        cmdLine,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        ErrorExit("CreateProcess failed");
    }
    CloseHandle(hPipe1Read);
    CloseHandle(hPipe2Write);

    DWORD bytesWritten;
    if (!WriteFile(hPipe1Write, &size, sizeof(size), &bytesWritten, NULL)) {
        ErrorExit("Parent: Failed to write size");
    }
    if (!WriteFile(hPipe1Write, arr.data(), size * sizeof(int), &bytesWritten, NULL)) {
        ErrorExit("Parent: Failed to write array");
    }
    CloseHandle(hPipe1Write);
    int result;
    DWORD bytesRead;
    if (!ReadFile(hPipe2Read, &result, sizeof(result), &bytesRead, NULL)) {
        ErrorExit("Parent: Failed to read result");
    }
    CloseHandle(hPipe2Read);

    std::cout << "Min element: " << result << std::endl;

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "child") {
        ChildMode();
    }
    else {
        ParentMode();
    }
    return 0;
}