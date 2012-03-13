// TTSepos.h : main header file for the TTSepos application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

/////////////////////////////////////////////////////////////////////////////
// CTTSeposApp:
//

class CTTSeposApp : public CWinApp
{
public:

	HANDLE	m_threadHandle;	
	
	CTTSeposApp();
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTTSeposApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

protected:
	static DWORD WINAPI ThreadGo(LPVOID pData);

	//{{AFX_MSG(CTTSeposApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


