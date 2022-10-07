#include "pch.h"
#include "framework.h"
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <TlHelp32.h>
#include <string>
#include <cassert>
#include <vector>
#include <TCHAR.h>
#include <cstdlib>
#include <sstream>

// Made by https://github.com/realCosmicHub
// Fixed by [Team] IamZiroToxic#8110

DWORD64 Base;
DWORD pid;
HANDLE pHandle;

INT64 readPointer(HANDLE hproc, DWORD64 Address)
{
	INT64 value;
	ReadProcessMemory(hproc, (INT64*)Address, &value, sizeof(value), 0);
	return value;
}

int readInteger(HANDLE hproc, DWORD64 Address)
{
	int value;
	ReadProcessMemory(hproc, (BYTE*)Address, &value, sizeof(value), 0);
	return value;
}

using std::cout;
using std::endl;
using std::string;

struct module
{
	DWORD64 dwBase, dwSize;
};

module TargetModule;
HANDLE TargetProcess;
DWORD64  TargetId;

HANDLE GetProcess(const wchar_t* processName)
{
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);

	do
		if (!_wcsicmp(entry.szExeFile, processName)) {
			TargetId = entry.th32ProcessID;
			CloseHandle(handle);
			TargetProcess = OpenProcess(PROCESS_ALL_ACCESS, false, TargetId);
			return TargetProcess;
		}
	while (Process32Next(handle, &entry));
}

module GetModule(const wchar_t* moduleName) {
	HANDLE hmodule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, TargetId);
	MODULEENTRY32 mEntry;
	mEntry.dwSize = sizeof(mEntry);

	do {
		if (!_wcsicmp(mEntry.szModule, moduleName)) {
			CloseHandle(hmodule);

			TargetModule = { (DWORD64)mEntry.hModule, mEntry.modBaseSize };
			return TargetModule;
		}
	} while (Module32Next(hmodule, &mEntry));

	module mod = { (DWORD64)false, (DWORD64)false };
	return mod;
}

template <typename var>
bool WriteMemory(DWORD64 Address, var Value) {
	return WriteProcessMemory(TargetProcess, (LPVOID)Address, &Value, sizeof(var), 0);
}

template <typename var>
var ReadMemory(DWORD64 Address) {
	var value;
	ReadProcessMemory(TargetProcess, (LPCVOID)Address, &value, sizeof(var), NULL);
	return value;
}

bool MemoryCompare(const BYTE* bData, const BYTE* bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++bData, ++bMask) {
		if (*szMask == 'x' && *bData != *bMask) {
			return false;
		}
	}
	return (*szMask == NULL);
}

DWORD64 FindSignature(DWORD64 start, DWORD64 size, const char* sig, const char* mask)
{
	BYTE* data = new BYTE[size];
	SIZE_T bytesRead;

	ReadProcessMemory(TargetProcess, (LPVOID)start, data, size, &bytesRead);

	for (DWORD64 i = 0; i < size; i++)
	{
		if (MemoryCompare((const BYTE*)(data + i), (const BYTE*)sig, mask)) {
			return start + i;
		}
	}
	delete[] data;
	return NULL;
}

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
	uintptr_t modBaseAddr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				if (!_wcsicmp(modEntry.szModule, modName))
				{
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}

void main() {
	Sleep(600);

	HWND hWnd = FindWindowA(("grcWindow"), nullptr);
	GetWindowThreadProcessId(hWnd, &pid);
	pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	Base = GetModuleBaseAddress(pid, L"FiveM_b2372_GTAProcess.exe");
	if (Base) {
		LPCSTR SignatureWorldPTR = "\x48\x8b\x05\x00\x00\x00\x00\x45\x00\x00\x00\x00\x48\x8b\x48\x08\x48\x85\xc9\x74\x07";
		LPCSTR MaskWorldPTR = "xxx????x????xxxxxxxxx";

		LPCSTR SignatureBlipPTR = "\x4c\x8d\x05\x00\x00\x00\x00\x0f\xb7\xc1";
		LPCSTR MaskBlipPTR = "xxx????xxx";

		LPCSTR SignaturePlayerPTR = "\x48\x8b\x0d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x48\x8b\xc8\xe8\x00\x00\x00\x00\x48\x8b\xcf";
		LPCSTR MaskPlayerPTR = "xxx????x????xxxx????xxx";

		LPCSTR SignatureGlobalPTR = "\x4c\x8d\x05\x00\x00\x00\x00\x4d\x8b\x08\x4d\x85\xc9\x74\x11";
		LPCSTR MaskGlobalPTR = "xxx????xxxxxxxx";

		if (GetProcess(L"FiveM_b2372_GTAProcess.exe"))
		{
			module mod = GetModule(L"FiveM_b2372_GTAProcess.exe");
			printf("========== Citizen Scripting ==========\n");
			DWORD64 TempWorldPTR = FindSignature(mod.dwBase, mod.dwSize, SignatureWorldPTR, MaskWorldPTR);
			auto world = (TempWorldPTR)+readInteger(pHandle, TempWorldPTR + 3) + 7;

			printf("world : 0x%I64X\n", world - mod.dwBase);

			DWORD64 TempBlipPTR = FindSignature(mod.dwBase, mod.dwSize, SignatureBlipPTR, MaskBlipPTR);
			auto blipPtr = TempBlipPTR + readInteger(pHandle, TempBlipPTR + 3) + 7;

			printf("blipPtr : 0x%I64X\n", blipPtr - mod.dwBase);

			DWORD64 viewPortTemp = FindSignature(mod.dwBase, mod.dwSize, "\x48\x8B\x15\x00\x00\x00\x00\x48\x8D\x2D\x00\x00\x00\x00\x48\x8B\xCD", "xxx????xxx????xxx");
			auto viewPort = viewPortTemp + readInteger(pHandle, viewPortTemp + 3) + 7;

			printf("viewPort : 0x%I64X\n", viewPort - mod.dwBase);

			DWORD64 replayInterfaceTemp = FindSignature(mod.dwBase, mod.dwSize, "\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\xD7\xE8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x8A\xD8\xE8\x00\x00\x00\x00\x84\xDB\x75\x13\x48\x8D\x0D", "xxx????xxxx????xxx????xxx????xxxxxxx");
			auto replayInterface = replayInterfaceTemp + readInteger(pHandle, replayInterfaceTemp + 3) + 7;

			printf("replayInterface : 0x%I64X\n", replayInterface - mod.dwBase);

			auto globaltestTemp = FindSignature(mod.dwBase, mod.dwSize, "\x4C\x8D\x05\x00\x00\x00\x00\x4D\x8B\x08\x4D\x85\xC9\x74\x11", "xxx????xxxxxxxx");
			auto globaltest = globaltestTemp + readInteger(pHandle, globaltestTemp + 4) + 1;

			printf("citzstop : 0x%I64X\n", globaltest - mod.dwBase);

			auto globaltest1Temp = FindSignature(mod.dwBase, mod.dwSize, "\x4C\x8D\x05\x00\x00\x00\x00\x4D\x8B\x08\x4D\x85\xC9\x74\x11", "xxx????xxxxxxxx");
			auto globaltest1 = globaltest1Temp + readInteger(pHandle, globaltest1Temp + 2) + 1;

			printf("citzstart : 0x%I64X\n", globaltest1 - mod.dwBase);

			auto globaltest2Temp = FindSignature(mod.dwBase, mod.dwSize, "\x4C\x8D\x05\x00\x00\x00\x00\x4D\x8B\x08\x4D\x85\xC9\x74\x11", "xxx????xxxxxxxx");
			auto globaltest2 = globaltest2Temp + readInteger(pHandle, globaltest2Temp + 2) + 1;

			printf("citzmanager : 0x%I64X\n", globaltest2 - mod.dwBase);
		}
	}
	else
	{
		Base = GetModuleBaseAddress(pid, L"gta5.exe");
		LPCSTR SignatureWorldPTR = "\x48\x8b\x05\x00\x00\x00\x00\x45\x00\x00\x00\x00\x48\x8b\x48\x08\x48\x85\xc9\x74\x07";
		LPCSTR MaskWorldPTR = "xxx????x????xxxxxxxxx";

		LPCSTR SignatureBlipPTR = "\x4c\x8d\x05\x00\x00\x00\x00\x0f\xb7\xc1";
		LPCSTR MaskBlipPTR = "xxx????xxx";

		LPCSTR SignaturePlayerPTR = "\x48\x8b\x0d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x48\x8b\xc8\xe8\x00\x00\x00\x00\x48\x8b\xcf";
		LPCSTR MaskPlayerPTR = "xxx????x????xxxx????xxx";

		LPCSTR SignatureGlobalPTR = "\x4c\x8d\x05\x00\x00\x00\x00\x4d\x8b\x08\x4d\x85\xc9\x74\x11";
		LPCSTR MaskGlobalPTR = "xxx????xxxxxxxx";

		if (GetProcess(L"gta5.exe"))
		{
			module mod = GetModule(L"gta5.exe");

			DWORD64 TempWorldPTR = FindSignature(mod.dwBase, mod.dwSize, SignatureWorldPTR, MaskWorldPTR);
			auto world = (TempWorldPTR)+readInteger(pHandle, TempWorldPTR + 3) + 7;
			printf("========== Citizen Scripting ==========\n");
			printf("world : 0x%I64X\n", world - mod.dwBase);

			DWORD64 TempBlipPTR = FindSignature(mod.dwBase, mod.dwSize, SignatureBlipPTR, MaskBlipPTR);
			auto blipPtr = TempBlipPTR + readInteger(pHandle, TempBlipPTR + 3) + 7;

			printf("blipPtr : 0x%I64X\n", blipPtr - mod.dwBase);

			DWORD64 viewPortTemp = FindSignature(mod.dwBase, mod.dwSize, "\x48\x8B\x15\x00\x00\x00\x00\x48\x8D\x2D\x00\x00\x00\x00\x48\x8B\xCD", "xxx????xxx????xxx");
			auto viewPort = viewPortTemp + readInteger(pHandle, viewPortTemp + 3) + 7;

			printf("viewPort : 0x%I64X\n", viewPort - mod.dwBase);

			DWORD64 replayInterfaceTemp = FindSignature(mod.dwBase, mod.dwSize, "\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\xD7\xE8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x8A\xD8\xE8\x00\x00\x00\x00\x84\xDB\x75\x13\x48\x8D\x0D", "xxx????xxxx????xxx????xxx????xxxxxxx");
			auto replayInterface = replayInterfaceTemp + readInteger(pHandle, replayInterfaceTemp + 3) + 7;

			printf("replayInterface : 0x%I64X\n", replayInterface - mod.dwBase);

			auto globaltestTemp = FindSignature(mod.dwBase, mod.dwSize, "\x4C\x8D\x05\x00\x00\x00\x00\x4D\x8B\x08\x4D\x85\xC9\x74\x11", "xxx????xxxxxxxx");
			auto globaltest = globaltestTemp + readInteger(pHandle, globaltestTemp + 4) + 1;

			printf("citzstop : 0x%I64X\n", globaltest - mod.dwBase);

			auto globaltest1Temp = FindSignature(mod.dwBase, mod.dwSize, "\x4C\x8D\x05\x00\x00\x00\x00\x4D\x8B\x08\x4D\x85\xC9\x74\x11", "xxx????xxxxxxxx");
			auto globaltest1 = globaltest1Temp + readInteger(pHandle, globaltest1Temp + 2) + 1;

			printf("citzstart : 0x%I64X\n", globaltest1 - mod.dwBase);

			auto globaltest2Temp = FindSignature(mod.dwBase, mod.dwSize, "\x4C\x8D\x05\x00\x00\x00\x00\x4D\x8B\x08\x4D\x85\xC9\x74\x11", "xxx????xxxxxxxx");
			auto globaltest2 = globaltest2Temp + readInteger(pHandle, globaltest2Temp + 2) + 1;

			printf("citzmanager : 0x%I64X\n", globaltest2 - mod.dwBase);
		}
	}

	while (1) {
		Sleep(1000);
	}
}

// Made by https ://github.com/realCosmicHub
// Fixed by [Team] IamZiroToxic#8110
