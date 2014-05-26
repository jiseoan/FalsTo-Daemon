#pragma once


#define _UNLIMITEDLIST_STEP					1024



template <typename T>
class CUnlimitedList
{
public:
	CUnlimitedList()
	{
		m_List = NULL;
		m_nCount = 0;
		m_nLength = 0;
		m_bAutoFree = TRUE;
	}

	~CUnlimitedList()
	{
		if (m_List != NULL)
		{
			if (m_bAutoFree)
			{
				for (int i = 0 ; i < m_nCount ; i++)
				{
					delete m_List[i];
				}
			}
			delete [] m_List;
		}
	}

protected:
	T* m_List;
	int m_nCount;
	int m_nLength;
	BOOL m_bAutoFree;

public:
	void SetAutoFree(BOOL bAutoFree) { m_bAutoFree = bAutoFree; }
	int GetCount() { return m_nCount; }
	T* GetList(int* pCount) { *pCount = m_nCount; return m_List; }
	void Realloc(int nAppend)
	{
		if (m_List == NULL)
		{
			m_nLength = (nAppend > _UNLIMITEDLIST_STEP) ? nAppend : _UNLIMITEDLIST_STEP;
			m_List = new T[m_nLength];
			return;
		}

		if (m_nLength - m_nCount >= nAppend)
		{
			return;
		}

		int n = m_nLength + _UNLIMITEDLIST_STEP;
		T* pT = new T[n];

		memcpy(pT, m_List, sizeof(T) * m_nCount);
		delete [] m_List;

		m_List = pT;
		m_nLength = n;
	}

	int Lookup(T val)
	{
		for (int i = 0 ; i < m_nCount ; i++)
		{
			if (m_List[i] == val)
			{
				return i;
			}
		}
		return -1;
	}

	int Add(T val)
	{
		if (m_nCount == m_nLength)
		{
			Realloc(_UNLIMITEDLIST_STEP);
		}
		m_List[m_nCount++] = val;
		return m_nCount;
	}

	int Remove(T val)
	{
		int i = Lookup(val);
		if (i >= 0)
		{
			memcpy(&m_List[i], &m_List[i + 1], sizeof(T) * (m_nCount-- - i - 1));
		}
		return i;
	}

	T Remove(int nIndex)
	{
		ASSERT(nIndex >= 0 && nIndex < m_nCount);
		T val = m_List[nIndex];
		memcpy(&m_List[nIndex], &m_List[nIndex + 1], sizeof(T) * (m_nCount-- - nIndex - 1));
		return val;
	}

	void RemoveAll()
	{
		if (m_bAutoFree)
		{
			for (int i = 0 ; i < m_nCount ; i++)
			{
				delete m_List[i];
			}
		}
		m_nCount = 0;
	}

	T GetAt(int nIndex)
	{
		ASSERT(nIndex >= 0 && nIndex < m_nCount);
		return m_List[nIndex];
	}
};
