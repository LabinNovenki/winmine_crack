#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <ImageHlp.h>
#define SIDEOFMINE 16
#define EASY 1
#define MIDDLE 2
#define HARD 3
using namespace std;
BOOL CALLBACK EnumWindowsProc(HWND hWnd,LPARAM lParam);
BOOL CALLBACK EnumChildWindowsProc(HWND hWnd,LPARAM lParam);
DWORD GetPidByName(LPCWSTR p_name);
void display_mines(unsigned char * mines, int size);
bool isMouseInWindowClient(HWND wh);
POINT GetCursorPosInArray(HWND wh);
int LoadMineImage(HWND wh);
int SendToQQ();
DWORD err;
int height, length;
const POINT mine_client_point = {12, 55};

int main() {
    DWORD pid = 0;
    HANDLE hProcess = nullptr;

    auto * mines = (unsigned char*)malloc(height * length * sizeof(char));
    memset(mines, 0, height * length); //储存地雷数组
    auto pAddress = (LPVOID)(0x01005340); //地雷数组在内存中的起始地址

    POINT cur_pos;
    HWND wProcess;

    pid = GetPidByName(L"winmine.exe");
    if (pid <= 0) {
        err = GetLastError();
        cout << "Cannot Open winmine.exe. " << err << endl;
        exit(1);
    }


    hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    if (hProcess == nullptr) {
        err = GetLastError();
        cout << "Cannot Open winmine.exe." << err << endl;
        exit(1);
    }

    int hardness = EASY;
    cout << "type in the hardness:(1 represents easy, 2 for middle and 3 for hard.)" << endl;
    scanf_s("%d", &hardness);

    if (hardness == EASY) {
        length = 9;
        height = 9;
    }
    else if (hardness == MIDDLE) {
        length = 16;
        height = 16;
    }
    else if (hardness == HARD) {
        length = 30;
        height = 16;
    }
    else {
        cout << "Wrong input." << endl;
        return 1;
    }

    DWORD dwOldProtect = 0;
    if (VirtualProtectEx(hProcess, pAddress, height * length * 3, PAGE_READWRITE, &dwOldProtect) == FALSE)
    {
        err = GetLastError();
        cout << "Fail to disable memory protection." << err << endl;
        exit(1);
    }

    //读取雷区
    unsigned char * lp = (unsigned char*)pAddress + 1;
    cout << "start read memory at 0x" << (void*)lp << endl;
    int Ret;
    for (int i = 0; i < height; i ++) {
        lp += 0x20; // 略过边框
        Ret = ReadProcessMemory(hProcess,lp,mines + i * length, length, nullptr);
        if(!Ret)
        {
            err = GetLastError();
            cout << "Fail to read memory." << err << endl;
            exit(1);
        }
    }

    cout << "read memory successfully.\n" << endl;

    display_mines(mines, length * height);

//    lp = (unsigned char*)pAddress + 1;
//    for (int i = 0; i < height; i ++) {
//        lp += 0x20;
//        if (!WriteProcessMemory(hProcess, lp, mines + i * length, length, nullptr)) {
//            err = GetLastError();
//            printf("Fail to write memory. %ld\n ", err);
//            exit(1);
//        }
//    }

    wProcess = FindWindow(nullptr, L"扫雷");
    if(!wProcess) {
        err = GetLastError();
        cout << "Fail to open window's handle." << err << endl;
        exit(1);
    }

    LoadMineImage(wProcess);

    POINT cur_mine = {0, 0};
    while(IsWindow(wProcess)) {
        if (isMouseInWindowClient(wProcess)) {
            cur_mine = GetCursorPosInArray(wProcess);
            if (cur_mine.x >= 0 && cur_mine.x < length && cur_mine.y >=0 && cur_mine.y < height) {
                if (mines[cur_mine.y * length + cur_mine.x] == 0x8F) {
                    cout << "care!";
//                    system("python C:/Users/ASUS/PycharmProjects/pythonProject7/main.py");
                }
            }
        }
        Sleep(400);
    }

    return 0;

}

void display_mines(unsigned char * mines, int size) {
    int i;
    for(i = 0; i < size; i ++) {
        cout <<"0x" << hex << (short)mines[i] << " ";
        if ((i + 1) % length == 0) {
            cout << endl;
        }
    }
}

DWORD GetPidByName(LPCWSTR p_name) {
    PROCESSENTRY32 p32 = {0};
    HANDLE hSnap = nullptr;
    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0); //用于获取进程列表的快照
    DWORD pid = -1;

    if (hSnap == INVALID_HANDLE_VALUE) {
        cout << "ERROR:%d" << errno;
        exit(1);
    }
    p32.dwSize = sizeof(PROCESSENTRY32);

    bool bMore;
    bMore = Process32First(hSnap, &p32);
    while (bMore) {
        if (!lstrcmp(p32.szExeFile, p_name)) {
            pid = p32.th32ProcessID;
        }
        bMore = Process32Next(hSnap, &p32);
    }

    CloseHandle(hSnap);
    return pid;
}

bool isMouseInWindowClient(HWND wh) {
    RECT ClientRect;
    POINT cur_pos;
    if (!IsWindow(wh)) {
        exit(1);
    }
    GetCursorPos(&cur_pos);
    ScreenToClient(wh, &cur_pos);
    ClientRect.top = mine_client_point.y;
    ClientRect.left = mine_client_point.x;
    ClientRect.bottom = ClientRect.top + height * SIDEOFMINE;
    ClientRect.right = ClientRect.left + length * SIDEOFMINE;
//    printf("RECT: top:%ld left:%ld bottom:%ld right:%ld\n", ClientRect.top, ClientRect.left, ClientRect.bottom, ClientRect.right);

//    printf("cur_pos:%ld %ld\n", cur_pos.x, cur_pos.y);
    if (PtInRect(&ClientRect,cur_pos)) {
        return true;
    }
    else {
        return false;
    }

}

POINT GetCursorPosInArray(HWND wh) {

    POINT cur_pos;
    int x = 0, y = 0;
    GetCursorPos(&cur_pos);
    if (IsWindow(wh)){
        ScreenToClient(wh, &cur_pos);
    }
    else {
        exit(1);
    }
    x = (cur_pos.x - mine_client_point.x) / SIDEOFMINE;
    y = (cur_pos.y - mine_client_point.y) / SIDEOFMINE;
//    printf("%d %d\n", x, y);
    return {x, y};

}

int LoadMineImage(HWND wh) {
    // 加载位图文件
    HBITMAP hbmMine = nullptr;
    hbmMine = static_cast<HBITMAP>(LoadImageA(nullptr, "mine.bmp", IMAGE_BITMAP, 16, 16, LR_LOADFROMFILE));
    if (!hbmMine) {
        cout << "Fail to Load bmp file." << endl;
        return -1;
    }


    BITMAP bm;
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(wh, &ps);
    if (!hdc) {
        err = GetLastError();
        cout << "Error:BeginPaint()" << err << endl;
    }
    HDC hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem) {
        err = GetLastError();
        cout << "Error:CreateCompatibleDC()" << err << endl;
    }

    auto hbmOld = static_cast<HBITMAP>(SelectObject(hdcMem, hbmMine));
    if (!GetObjectA(hbmMine, sizeof(bm), &bm)) {
        err = GetLastError();
        cout << "Error:GetObject()" << err << endl;
    }

    if (!BitBlt(hdc, 12, 55, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY)) {
        err = GetLastError();
        cout << "Error:BitBlt()" << err << endl;
    }
    if (!SelectObject(hdcMem, hbmOld)) {
        err = GetLastError();
        cout << "Error:SelectObject()" << err << endl;
    }
    DeleteDC(hdcMem);

    EndPaint(wh, &ps);
    UpdateWindow(wh);
    return 0;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam) {
    TCHAR * szTitle = (TCHAR*)malloc(sizeof(TCHAR) * MAX_PATH);
    if (IsWindow(hwnd)) {
        if (!GetWindowText(hwnd, szTitle, MAX_PATH)) {
            err = GetLastError();
            cout << "Error:GetWindowText()" << err << endl;
        }
    }
    wprintf(L"%s\n", szTitle);
    return true;
}

BOOL CALLBACK EnumChildWindowsProc(HWND hwnd,LPARAM lParam) {
    TCHAR * szTitle = (TCHAR*)malloc(sizeof(TCHAR) * MAX_PATH);
    if (IsWindow(hwnd)) {
        if (!GetWindowText(hwnd, szTitle, MAX_PATH)) {
            err = GetLastError();
            cout << "Error:GetWindowText()" << err << endl;
        }
    }
    wprintf(L"%s\n", szTitle);
    return true;
}

int SendToQQ() {
    HWND wQQ;
    wQQ = FindWindow(nullptr, L"我的Android手机");
//    EnumChildWindows(wQQ, EnumChildWindowsProc, 0);
    TCHAR msg[] = L"别点！";
    if (!wQQ) {
        err = GetLastError();
        cout << "Error:FindWindow()" << err << endl;
        return -1;
    }

    if (!OpenClipboard(wQQ)) {
        err = GetLastError();
        cout << "Error:OpenClipboard()" << err << endl;
        return -1;
    }
    if (!EmptyClipboard())
    {
        err = GetLastError();
        cout << "Error:EmptyClipboard()" << err << endl;
        return -1;
    }

    HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(msg));
    if (!hData) {
        err = GetLastError();
        cout << "Error:GlobalAlloc()" << err << endl;
        return -1;
    }
    LPWSTR pData = (LPWSTR)GlobalLock(hData);
    if (!pData) {
        err = GetLastError();
        cout << "Error:GlobalAlloc()" << err << endl;
        return -1;
    }
    CopyMemory(pData, msg, sizeof(msg));

    if (!SetClipboardData(CF_UNICODETEXT, hData))
    {
        err = GetLastError();
        cout << "Error:SetClipboardData()" << err << endl;
        return -1;
    }

    ShowWindow(wQQ, SW_SHOW);

    keybd_event(VK_LBUTTON, 0, 0, 0);
    SendMessage(wQQ, WM_SETTEXT, 0, (LPARAM)pData);
    SendMessage(wQQ, WM_KEYDOWN, VK_RETURN, 0);
    SendMessage(wQQ, WM_KEYUP, VK_RETURN, 0);

    CloseClipboard();
    GlobalUnlock(hData);
    return 0;
}