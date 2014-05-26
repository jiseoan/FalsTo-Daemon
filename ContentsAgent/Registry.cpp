#include "stdafx.h"
#include "Registry.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



CRegistry::CRegistry()
{
	m_hKey = NULL;
}

CRegistry::~CRegistry()
{
	Close();
}

BOOL CRegistry::Open(LPCTSTR lpSubKey, HKEY hKey, BOOL bCreate)
{
	return ((RegOpenKeyEx(hKey, lpSubKey, 0L, KEY_ALL_ACCESS, &m_hKey) == ERROR_SUCCESS) ? TRUE : ((bCreate) ? CreateKey(lpSubKey) : FALSE));
}

void CRegistry::Close()
{
	if (m_hKey)
	{
		RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
}

BOOL CRegistry::CreateKey(LPCTSTR lpSubKey, HKEY hKey)
{
	DWORD temp;
	return ((ERROR_SUCCESS == RegCreateKeyEx(hKey, lpSubKey, 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_hKey, &temp)) ? TRUE : FALSE);
}

BOOL CRegistry::Delete(LPCTSTR lpKey)
{
	return ((RegDeleteValue(m_hKey, lpKey) == ERROR_SUCCESS) ? TRUE : FALSE);
}

BOOL CRegistry::IsValid(LPCTSTR lpKey)
{
	DWORD type, size = 200;
	char buf[255];
	return ((RegQueryValueEx(m_hKey, (LPTSTR)lpKey, NULL, &type, (BYTE*)buf, &size) == ERROR_SUCCESS) ? TRUE : FALSE);
}

BYTE CRegistry::ReadByte(LPCTSTR lpKey)
{
	DWORD type, size = sizeof(BYTE);
	BYTE rVal = 0;

	RegQueryValueEx(m_hKey, (LPTSTR)lpKey, NULL, &type, (BYTE*)&rVal, &size);
	return rVal;
}

BOOL CRegistry::Write(LPCTSTR lpKey, BYTE BData)
{
	DWORD dw = (DWORD)BData;
	return ((RegSetValueEx(m_hKey, lpKey, 0L, REG_BINARY, (CONST BYTE*)&dw, sizeof(BYTE)) == ERROR_SUCCESS) ? TRUE : FALSE);
}

int CRegistry::ReadInt(LPCTSTR lpKey)
{
	DWORD type, size = sizeof(REG_DWORD);
	DWORD rVal = 0;

	RegQueryValueEx(m_hKey, (LPTSTR)lpKey, NULL, &type, (BYTE*)&rVal, &size);
	return (int)rVal;
}

BOOL CRegistry::Write(LPCTSTR lpKey, int nData)
{
	DWORD dw = (DWORD)nData;
	return ((RegSetValueEx(m_hKey, lpKey, 0L, REG_DWORD, (CONST BYTE*)&dw, sizeof(REG_DWORD)) == ERROR_SUCCESS) ? TRUE : FALSE);
}

DWORD CRegistry::ReadDWORD(LPCTSTR lpKey)
{
	DWORD type, size = sizeof(REG_DWORD);
	DWORD rVal = 0;

	RegQueryValueEx(m_hKey, (LPTSTR)lpKey, NULL, &type, (BYTE*)&rVal, &size);
	return rVal;
}

BOOL CRegistry::Write(LPCTSTR lpKey, DWORD nData)
{
	return ((RegSetValueEx(m_hKey, lpKey, 0L, REG_DWORD, (CONST BYTE*)&nData, sizeof(REG_DWORD)) == ERROR_SUCCESS) ? TRUE : FALSE);
}

CString CRegistry::ReadStr(LPCTSTR lpKey)
{
	DWORD type, size = 200;
	TCHAR buf[255];
	CString rVal;

	if(RegQueryValueEx(m_hKey, (LPTSTR)lpKey, NULL, &type, (BYTE*)buf, &size) == ERROR_SUCCESS)
		rVal = buf;

	return rVal;
}

BOOL CRegistry::Write(LPCTSTR lpKey, LPCTSTR lpData)
{
	return ((RegSetValueEx(m_hKey, lpKey, 0L, REG_SZ, (CONST BYTE*)lpData, (_tcslen(lpData) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS) ? TRUE : FALSE);
}
