// RearrangeDeskTopIcon.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <vector>
#include <assert.h>

#define UPLIMIT		0.2
#define DOWNLIMT	0.3
int _tmain(int argc, _TCHAR* argv[])
{

	
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	RECT rcRearrange= {0, 0, nScreenWidth, nScreenHeight*0.2};

	HWND hwndSysListView32 = NULL, hwndSHELLDLL_DefView = NULL;
	HWND hWnd = ::FindWindow(_T("Progman"), _T("Program Manager"));
	if (hWnd)
	{
		hwndSHELLDLL_DefView = ::FindWindowEx(hWnd, NULL, _T("SHELLDLL_DefView"), NULL);
		if (hwndSHELLDLL_DefView)
		{
			hwndSysListView32 = ::FindWindowEx(hwndSHELLDLL_DefView, NULL, _T("SysListView32"), NULL);
		}
	}
	
	if (!hwndSysListView32 || !hwndSHELLDLL_DefView)
	{
		assert(false);
		return 0;
	}	


	//移除掉自动排序、开启网格对齐
	DWORD dwStyle = GetWindowLong(hwndSysListView32, GWL_STYLE);
	if ((dwStyle & LVS_AUTOARRANGE) == LVS_AUTOARRANGE)
	{
		::SendMessage(hwndSHELLDLL_DefView, WM_COMMAND, 28785 ,0);
	}

	DWORD dwExStyle = (DWORD)ListView_GetExtendedListViewStyle(hwndSysListView32, GWL_EXSTYLE);	
	if ((dwExStyle & LVS_EX_SNAPTOGRID) != LVS_EX_SNAPTOGRID)
	{
		::SendMessage(hwndSHELLDLL_DefView, WM_COMMAND, 28788 ,0);
	}

	int nShortCutCount = ListView_GetItemCount(hwndSysListView32);

	//因为不能直接跨进程发消息去取坐标
	DWORD dwpId;
	GetWindowThreadProcessId(hWnd, &dwpId);
	HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION|PROCESS_VM_READ|
		PROCESS_VM_WRITE|PROCESS_QUERY_INFORMATION, FALSE, dwpId); 
	int nError = GetLastError();
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		assert(false);
		return 0;
	}

	PVOID ppt=(POINT*)VirtualAllocEx(hProcess, NULL, sizeof(POINT),MEM_COMMIT, PAGE_READWRITE);//获取坐标用
	PVOID pRect = (RECT*)VirtualAllocEx(hProcess, NULL, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE);
	std::vector<int> vecRearrange;
	int nIconWidth = 0, nIconHeight = 0;
	bool bUpdate = true;
	for (DWORD i = 0; i < nShortCutCount; i++)
	{
		POINT pt = {0};
		SendMessage(hwndSysListView32, LVM_GETITEMPOSITION, i, (LPARAM)ppt); 
		ReadProcessMemory(hProcess,ppt,&pt,sizeof(POINT),NULL); 

		RECT rc = {0};
		SendMessage(hwndSysListView32, LVM_GETITEMRECT, i, (LPARAM)pRect);
		ReadProcessMemory(hProcess, pRect, &rc, sizeof(RECT), NULL);
		nIconWidth = nIconWidth == 0 ? rc.right - rc.left : nIconWidth;
		nIconHeight = nIconHeight == 0 ? rc.bottom - rc.top : nIconHeight;

		printf("图标id=%d	坐标%d、%d	区域%d、%d、%d、%d", i, pt.x, pt.y, rc.left, rc.top, rc.right, rc.bottom);
		vecRearrange.push_back(i);
	}
	VirtualFreeEx(hProcess, ppt, 0, MEM_RELEASE);
	VirtualFreeEx(hProcess, pRect, 0, MEM_RELEASE);


	if (bUpdate)
	{
		assert(nIconHeight != 0 && nIconWidth != 0);
		int nMaxColumn = 1000/nIconWidth;
		int nMaxRow = nScreenHeight*0.5/nIconHeight;
		assert(nMaxRow != 0 && nMaxColumn != 0);
		for (int i = 0; i < vecRearrange.size(); i++)
		{
			int nCol = i%nMaxColumn;
			int nRow = i/nMaxColumn;
			::SendMessage(hwndSysListView32, LVM_SETITEMPOSITION, vecRearrange[i], MAKELPARAM(rcRearrange.right - nCol*nIconWidth, rcRearrange.top + nRow*nIconHeight));
			printf("移动图标%d	x=%d	y=%d\n", vecRearrange[i], rcRearrange.right - nCol*nIconWidth, rcRearrange.top + nRow*nIconHeight);
		}
		
		ListView_RedrawItems(hwndSysListView32, 0, ListView_GetItemCount(hwndSysListView32) - 1);
		::UpdateWindow(hwndSysListView32);
	}
	
	//防止F5刷新恢复之前的图标，手动设置一下工作区，第一个参数必须>=2。
	PVOID pRcWorkAreas = (RECT*)VirtualAllocEx(hProcess, NULL, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE);
	RECT rcTmp = {0, 0, nScreenWidth, nScreenHeight};
	WriteProcessMemory(hProcess, pRcWorkAreas, &rcTmp, sizeof(RECT), NULL); 
	::SendMessage(hwndSysListView32, LVM_SETWORKAREAS, 4, (LPARAM)pRcWorkAreas);
	VirtualFreeEx(hProcess, pRcWorkAreas, 0, MEM_RELEASE);
	
	return 0;
}

