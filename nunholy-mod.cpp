﻿#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

#include <locale>
#include <codecvt>
#include<iomanip>
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

// "UnityPlayer.dll"+01CB26A8
DWORD_PTR ResolveBloodStoneAddress(HANDLE hProcess, DWORD_PTR unityBase) {
    DWORD_PTR addr = unityBase + 0x1CB26A8;
    DWORD_PTR offsets[] = { 0xF08, 0xCB0, 0x30, 0x18, 0x28, 0xB8, 0x270 };

    for (int i = 0; i < 7; i++) {
        if (!ReadProcessMemory(hProcess, (LPCVOID)addr, &addr, sizeof(addr), NULL)) {
            std::cout << "[ERROR] 혈석 주소 읽기 실패 at level " << i << std::endl;
            return 0;
        }
        addr += offsets[i];
    }
    return addr;
}

void CurrencyMenu(HANDLE hProcess, DWORD_PTR monoBase) {
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
        int currentSilver = 0, currentRuby = 0;
        ReadProcessMemory(hProcess, (LPCVOID)silverAddr, &currentSilver, sizeof(currentSilver), NULL);
        ReadProcessMemory(hProcess, (LPCVOID)rubyAddr, &currentRuby, sizeof(currentRuby), NULL);

        std::cout << "\n=== 재화 메뉴 ===\n";
        std::cout << "[현재 실링: " << std::dec << currentSilver << "] [현재 루비: " << std::dec << currentRuby << "]\n";
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
void DungeonMenu(HANDLE hProcess, DWORD_PTR monoBase, DWORD_PTR unityBase) {
    // 포인터 체인 정보 (던전용)
    DWORD_PTR basePointer = 0x01D29E48;
    DWORD_PTR pointerOffsets[] = { 0x58, 0x88, 0x8, 0x18, 0x10, 0x28, 0x90 };

    DWORD_PTR baseAddress, maxHealthAddress;
    RefreshBaseAddress(hProcess, unityBase, basePointer, pointerOffsets, 7, baseAddress, maxHealthAddress);

    DWORD_PTR healthAddress = baseAddress + 0x94;
    DWORD_PTR shieldAddress = baseAddress + 0x98;
    DWORD_PTR speedAddress = baseAddress + 0x9C;

    int choice;
    DWORD newValue;
    float newSpeed;


    DWORD_PTR bloodStoneAddr = ResolveBloodStoneAddress(hProcess, unityBase);

        while (true) {
            int currentBloodStone = 0;
            int currentMaxHealth = 0;
            int currentHealth = 0;
            float currentSpeed = 0.0f;
            ReadProcessMemory(hProcess, (LPCVOID)bloodStoneAddr, &currentBloodStone, sizeof(currentBloodStone), NULL);
            ReadProcessMemory(hProcess, (LPCVOID)maxHealthAddress, &currentMaxHealth, sizeof(currentMaxHealth), NULL);
            ReadProcessMemory(hProcess, (LPCVOID)healthAddress, &currentHealth, sizeof(currentHealth), NULL);
            ReadProcessMemory(hProcess, (LPCVOID)speedAddress, &currentSpeed, sizeof(currentSpeed), NULL);
            std::cout << "\n=== 던전 메뉴 ===\n";
           // std::cout << "[현재 BloodStone: " << std::dec << currentBloodStone << "] [현재 체력: " << std::dec << currentHealth << "/" << std::dec << currentMaxHealth << "] [현재 이동속도: " << std::dec << currentSpeed << "]\n";
            std::cout << std::fixed << std::setprecision(2); // 소수점 둘째 자리까지 고정
            std::cout << "[현재 BloodStone: " << std::dec << currentBloodStone
                << "] [현재 체력: " << currentHealth << "/" << currentMaxHealth
                << "] [현재 이동속도: " << currentSpeed << "]\n";
            std::cout << "※ Shield 인스턴스는 아직 연구 중 입니다.\n";
            std::cout << "1. Max Health 변경\n";
            std::cout << "2. Health 변경\n";
            std::cout << "3. Shield 변경\n";
            std::cout << "4. Speed 변경\n";
            std::cout << "5. BloodStone 변경\n";
            std::cout << "6. 새로고침\n";
            std::cout << "7. 돌아가기\n";
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
                std::cout << "변경할 BloodStone 값을 입력하세요: ";
                std::cin >> newValue;
                WriteProcessMemory(hProcess, (LPVOID)(bloodStoneAddr), &newValue, sizeof(newValue), NULL);
                std::cout << "혈석이 " << std::dec << newValue << "으로 변경되었습니다!\n";
                if (DEBUG_MODE) std::cout << "[DEBUG] BloodStone 변경 완료: " << newValue << std::endl;
                break;
            case 6:
                std::cout << "새로고침 중...\n";
                RefreshBaseAddress(hProcess, unityBase, basePointer, pointerOffsets, 7, baseAddress, maxHealthAddress);
                std::cout << "새로고침 완료!\n";
                break;
            case 7:
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

    DWORD_PTR unityBase = GetModuleBaseAddress(GetProcessId(hProcess), L"UnityPlayer.dll");
    if (!unityBase) {
        std::cout << "UnityPlayer.dll의 베이스 주소를 찾을 수 없습니다." << std::endl;
        return -1;
    }

    DWORD_PTR monoBase = GetModuleBaseAddress(GetProcessId(hProcess), L"mono-2.0-bdwgc.dll");
    if (!monoBase) {
        std::cout << "mono-2.0-bdwgc.dll 찾을 수 없습니다.\n" << std::endl;
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
            DungeonMenu(hProcess, monoBase, unityBase);
            break;
        case 2:
            CurrencyMenu(hProcess, monoBase);
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
