#include <winreg.h>


class CRegistry
{
public:
	CRegistry();
	virtual ~CRegistry();

public:
	BOOL IsValid(LPCTSTR lpKey);
	BOOL Delete(LPCTSTR lpKey);
	BOOL CreateKey(LPCTSTR lpSubKey, HKEY hKey = HKEY_CURRENT_USER);
	BOOL Open(LPCTSTR lpSubKey, HKEY hKey = HKEY_CURRENT_USER, BOOL bCreate = TRUE);
	void Close();
	BYTE ReadByte(LPCTSTR lpKey);
	BOOL Write(LPCTSTR lpKey, BYTE BData);
	int ReadInt(LPCTSTR lpKey);
	BOOL Write(LPCTSTR lpKey, int nData);
	DWORD ReadDWORD(LPCTSTR lpKey);
	BOOL Write(LPCTSTR lpKey, DWORD nData);
	CString ReadStr(LPCTSTR lpKey);
	BOOL Write(LPCTSTR lpKey, LPCTSTR lpData);

private:
	HKEY 	m_hKey;
};
