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
/*
void UpdateUI(HANDLE hProcess) {
    DWORD_PTR updateUIFunction = 0x23905bbac70;

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)updateUIFunction, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
        if (DEBUG_MODE) {
            std::cout << "[DEBUG] UI 업데이트 함수 (Awake) 실행 완료!\n";
        }
    }
    else {
        std::cout << "[ERROR] UI 업데이트 함수 실행 실패!\n";
    }
}
*/

void DungeonMenu(HANDLE hProcess) {
    DWORD_PTR unityPlayerBase = GetModuleBaseAddress(GetProcessId(hProcess), L"UnityPlayer.dll");
    if (!unityPlayerBase) {
        std::cout << "UnityPlayer.dll의 베이스 주소를 찾을 수 없습니다." << std::endl;
        return;
    }

    // 포인터 체인 정보 (던전용)
    DWORD_PTR basePointer = 0x01D29E48;
    DWORD_PTR pointerOffsets[] = { 0x58, 0x88, 0x8, 0x18, 0x10, 0x28, 0x90 };

    DWORD_PTR baseAddress, maxHealthAddress;
    RefreshBaseAddress(hProcess, unityPlayerBase, basePointer, pointerOffsets, 7, baseAddress, maxHealthAddress);

    DWORD_PTR healthAddress = baseAddress + 0x94;
    DWORD_PTR shieldAddress = baseAddress + 0x98;
    DWORD_PTR speedAddress = baseAddress + 0x9C;

    int choice;
    DWORD newValue;
    float newSpeed;


    while (true) {
        std::cout << "\n=== 던전 메뉴 ===\n";
        std::cout << "1. Max Health 변경\n";
        std::cout << "2. Health 변경\n";
        std::cout << "3. Shield 변경\n";
        std::cout << "4. Speed 변경\n";
        std::cout << "5. 새로고침\n";
        std::cout << "6. 돌아가기\n";
        std::cout << "선택: ";
        std::cin >> choice;

        switch (choice) {
        case 1:
            std::cout << "변경할 Max Health 값을 입력하세요: ";
            std::cin >> newValue;
            WriteProcessMemory(hProcess, (LPVOID)(maxHealthAddress), &newValue, sizeof(newValue), NULL);
            std::cout << "Max Health가 " << std::dec << newValue << "으로 변경되었습니다!\n";
            // UpdateUI(hProcess);
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
            // CloseHandle(hProcess);
            return;
        default:
            std::cout << "잘못된 선택입니다. 다시 입력하세요.\n";
            break;
        }
    }
}

DWORD_PTR ResolveSilverAddress(HANDLE hProcess, DWORD_PTR monoBase) {
    DWORD_PTR addr = monoBase + 0x774518; // mono-2.0-bdwgc.dll + 0x774518
    DWORD_PTR offsets[] = { 0x400, 0x640, 0x640, 0x648, 0x18 };
    for (int i = 0; i < 5; i++) {
        ReadProcessMemory(hProcess, (LPCVOID)addr, &addr, sizeof(addr), NULL);
        addr += offsets[i];
    }
    return addr;
}

DWORD_PTR ResolveRubyAddress(DWORD_PTR silverAddr) {
    DWORD_PTR addr = silverAddr - 0x20;
    return addr;
}

void CurrencyMenu(HANDLE hProcess) {
    DWORD_PTR monoBase = GetModuleBaseAddress(GetProcessId(hProcess), L"mono-2.0-bdwgc.dll");
    if (!monoBase) {
        std::cout << "mono-2.0-bdwgc.dll 찾을 수 없음\n";
        return;
    }

    DWORD_PTR silverAddr = ResolveSilverAddress(hProcess, monoBase);
    if (!silverAddr) {
        std::cout << "'은화'값의 포인터 체인을 찾을 수 없습니다.\n";
        return;
    }

    DWORD_PTR rubyAddr = ResolveRubyAddress(silverAddr);
    if (!rubyAddr) {
        std::cout << "'루비'값의 포인터 체인을 찾을 수 없습니다.\n";
        return;
    }

    int choice;
    int value;
    
    while (true) {
        std::cout << "\n=== 재화 메뉴 ===\n";
        std::cout << "1. 실링 값 변경\n";
        std::cout << "2. 루비 값 변경\n";
        std::cout << "3. 돌아가기\n";
        std::cout << "선택: ";
        std::cin >> choice;

        switch (choice) {
        case 1:
            std::cout << "변경할 실링 값을 입력하세요.\n";
            std::cin >> value;
            WriteProcessMemory(hProcess, (LPVOID)silverAddr, &value, sizeof(value), NULL);
            std::cout << "실링 값이 " << std::dec << value << " 으로 변경되었습니다!\n";
            break;
        case 2:
            std::cout << "변경할 루비 값을 입력하세요.\n";
            std::cin >> value;
            WriteProcessMemory(hProcess, (LPVOID)rubyAddr, &value, sizeof(value), NULL);
            std::cout << "루비 값이 " << std::dec << value << " 으로 변경되었습니다!\n";
            break;
        case 3:
            // CloseHandle(hProcess);
            return;
        default:
            std::cout << "잘못된 선택입니다. 다시 입력하세요.\n";
            break;
        }
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
    int choice;

    while (true) {
        std::cout << "\n===== 메뉴 =====\n";
        std::cout << "1. 던전 기능 (체력 등)\n";
        std::cout << "2. 재화 기능 (실링 등)\n";
        std::cout << "3. 종료\n";
        std::cout << "선택: ";
        std::cin >> choice;
        switch (choice) {
        case 1:
            DungeonMenu(hProcess);
            break;
        case 2:
            CurrencyMenu(hProcess);
            break;
        case 3:
            CloseHandle(hProcess);
            return 0;
        default:
            std::cout << "잘못된 선택입니다.\n";
            break;
        }
        /*
        switch (choice) {
        case 1:
            std::cout << "변경할 Max Health 값을 입력하세요: ";
            std::cin >> newValue;
            WriteProcessMemory(hProcess, (LPVOID)(maxHealthAddress), &newValue, sizeof(newValue), NULL);
            std::cout << "Max Health가 " << std::dec << newValue << "으로 변경되었습니다!\n";
           // UpdateUI(hProcess);
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
        */
    }

    CloseHandle(hProcess);
    return 0;
}
