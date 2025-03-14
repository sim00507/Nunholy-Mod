#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

#include <locale>
#include <codecvt>

bool DEBUG_MODE = false;

// wchar_t* → std::string 변환 함수
std::string WStringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// UnityPlayer.dll 베이스 주소
DWORD_PTR GetModuleBaseAddress(DWORD pid, const wchar_t* moduleName) {
    DWORD_PTR baseAddress = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 moduleEntry;
        moduleEntry.dwSize = sizeof(moduleEntry);
        if (Module32First(snapshot, &moduleEntry)) {
            do {
                if (!_wcsicmp(moduleEntry.szModule, moduleName)) {
                    baseAddress = (DWORD_PTR)moduleEntry.modBaseAddr;
                    if (DEBUG_MODE) {
                        std::wstring moduleWName = moduleEntry.szModule;
                        std::string moduleNameStr = WStringToString(moduleWName);
                        // std::cout << "[DEBUG] " << moduleName << " 베이스 주소: 0x" << std::hex << baseAddress << std::endl;
                        std::cout << "[DEBUG] " << moduleNameStr << " 베이스 주소: 0x" << std::hex << baseAddress << std::endl;
                    }
                    break;
                }
            } while (Module32Next(snapshot, &moduleEntry));
        }
    }
    CloseHandle(snapshot);
    return baseAddress;
}

// 포인터 체인
DWORD_PTR GetDynamicAddress(HANDLE hProcess, DWORD_PTR baseAddress, DWORD_PTR offsets[], int levels) {
    DWORD_PTR address = baseAddress;
    for (int i = 0; i < levels; i++) {
        ReadProcessMemory(hProcess, (LPCVOID)address, &address, sizeof(address), NULL);
        address += offsets[i];
        if (DEBUG_MODE) {
            std::cout << "[DEBUG] Offset " << i << ": 0x" << std::hex << offsets[i] << " -> 현재 주소: 0x" << address << std::endl;
        }
    }
    return address;
}

// Preiya의 `baseAddress`를 새로 계산하는 함수 (새로고침 기능)
void RefreshBaseAddress(HANDLE hProcess, DWORD_PTR unityPlayerBase, DWORD_PTR basePointer, DWORD_PTR pointerOffsets[], int levels, DWORD_PTR& baseAddress, DWORD_PTR& maxHealthAddress) {
    maxHealthAddress = GetDynamicAddress(hProcess, unityPlayerBase + basePointer, pointerOffsets, levels);

    // baseAddress 계산 (maxHealth - 0x90)
    baseAddress = maxHealthAddress - 0x90;

    if (DEBUG_MODE) {
        std::cout << "[DEBUG] (새로고침) maxHealthAddress: 0x" << std::hex << maxHealthAddress << std::endl;
        std::cout << "[DEBUG] (새로고침) baseAddress (Preiya 인스턴스): 0x" << std::hex << baseAddress << std::endl;
    }
}

int main() {
    // 디버깅 모드
    std::cout << "디버깅 모드를 활성화할까요? (1: 활성화 / 0: 비활성화): ";
    std::cin >> DEBUG_MODE;

    DWORD pid;
    HWND hwnd = FindWindowW(NULL, L"Nunholy"); 
    if (DEBUG_MODE) {
        if (hwnd) {
            std::cout << "[DEBUG] 게임 창 핸들 찾음: 0x" << std::hex << (DWORD_PTR)hwnd << std::endl;
        }
        else {
            std::cout << "[DEBUG] 게임 창을 찾을 수 없음!" << std::endl;
        }
    }

    GetWindowThreadProcessId(hwnd, &pid);
    if (DEBUG_MODE) {
        if (pid) {
            std::cout << "[DEBUG] 프로세스 ID (PID): " << std::dec << pid << std::endl;
        }
        else {
            std::cout << "[DEBUG] 프로세스 ID를 찾을 수 없음!" << std::endl;
        }
    }


    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (DEBUG_MODE) {
        if (hProcess) {
            std::cout << "[DEBUG] 프로세스 핸들 획득: 0x" << std::hex << (DWORD_PTR)hProcess << std::endl;
        }
        else {
            std::cout << "[DEBUG] 프로세스 핸들 획득 실패!" << std::endl;
        }
    }


    if (!hProcess) {
        std::cout << "프로세스를 열 수 없음." << std::endl;
        return -1;
    }

    DWORD_PTR unityPlayerBase = GetModuleBaseAddress(pid, L"UnityPlayer.dll");
    if (!unityPlayerBase) {
        std::cout << "UnityPlayer.dll의 베이스 주소를 찾을 수 없음." << std::endl;
        CloseHandle(hProcess);
        return -1;
    }

    // 포인터 체인 정보
    DWORD_PTR basePointer = 0x01D29E48;  // UnityPlayer.dll 내부 오프셋
    DWORD_PTR pointerOffsets[] = { 0x58, 0x88, 0x8, 0x18, 0x10, 0x28, 0x90 }; // Offset 체인

    // maxHealth 주소
    DWORD_PTR maxHealthAddress = GetDynamicAddress(hProcess, unityPlayerBase + basePointer, pointerOffsets, 7);

    // 찾은 maxHealth로 baseAddress 구하기
    DWORD_PTR baseAddress = maxHealthAddress - 0x90;

    if (DEBUG_MODE) {
        std::cout << "[DEBUG] maxHealthAddress: 0x" << std::hex << maxHealthAddress << std::endl;
        std::cout << "[DEBUG] baseAddress (Preiya 인스턴스): 0x" << std::hex << baseAddress << std::endl;
    }

    // 찾은 오프셋들
    DWORD_PTR healthAddress = baseAddress + 0x94;
    DWORD_PTR shieldAddress = baseAddress + 0x98;
    DWORD_PTR speedAddress = baseAddress + 0x9C;

    int choice;
    DWORD newValue;
    float newSpeed;

    while (true) {
        std::cout << "\n===== 값 변경 메뉴 =====\n";
        std::cout << "1. Max Health 변경\n";
        std::cout << "2. Health 변경\n";
        std::cout << "3. Shield 변경\n";
        std::cout << "4. Speed 변경\n";
        std::cout << "5. 새로고침 (던전 입장 후 다시 찾기)\n";
        std::cout << "6. 종료\n";
        std::cout << "선택: ";
        std::cin >> choice;

        switch (choice) {
        case 1:
            std::cout << "변경할 Max Health 값을 입력하세요: ";
            std::cin >> newValue;
            WriteProcessMemory(hProcess, (LPVOID)(maxHealthAddress), &newValue, sizeof(newValue), NULL);
            std::cout << "Max Health가 " << std::dec << newValue << "으로 변경되었습니다!\n";
            if (DEBUG_MODE) std::cout << "[DEBUG] Max Health 변경 완료: " << newValue << std::endl;
            break;
        case 2:
            std::cout << "변경할 Health 값을 입력하세요: ";
            std::cin >> newValue;
            WriteProcessMemory(hProcess, (LPVOID)(healthAddress), &newValue, sizeof(newValue), NULL);
            std::cout << "Health가 " << std::dec << newValue << "으로 변경되었습니다!\n";
            if (DEBUG_MODE) std::cout << "[DEBUG] Health 변경 완료: " << newValue << std::endl;
            break;
        case 3:
            std::cout << "변경할 Shield 값을 입력하세요: ";
            std::cin >> newValue;
            WriteProcessMemory(hProcess, (LPVOID)(shieldAddress), &newValue, sizeof(newValue), NULL);
            std::cout << "Shield가 " << std::dec << newValue << "으로 변경되었습니다!\n";
            if (DEBUG_MODE) std::cout << "[DEBUG] Shield 변경 완료: " << newValue << std::endl;
            break;
        case 4:
            std::cout << "변경할 Speed 값을 입력하세요 (소수점 가능): ";
            std::cin >> newSpeed;
            WriteProcessMemory(hProcess, (LPVOID)(speedAddress), &newSpeed, sizeof(newSpeed), NULL);
            std::cout << "Speed가 " << std::dec << newSpeed << "으로 변경되었습니다!\n";
            if (DEBUG_MODE) std::cout << "[DEBUG] Speed 변경 완료: " << newSpeed << std::endl;
            break;
        case 5:
            std::cout << "새로고침 중...\n";
            RefreshBaseAddress(hProcess, unityPlayerBase, basePointer, pointerOffsets, 7, baseAddress, maxHealthAddress);
            std::cout << "새로고침 완료!\n";
            break;
        case 6:
            CloseHandle(hProcess);
            return 0;
        default:
            std::cout << "잘못된 선택입니다. 다시 입력하세요.\n";
            break;
        }
    }

    CloseHandle(hProcess);
    return 0;
}
