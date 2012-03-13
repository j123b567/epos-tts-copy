#include <afxwin.h>	// MFC core and standard components
#include <afxext.h>	// MFC extensions
#include <afxsock.h>	// MFC socket extensions
#include <afxmt.h>	// MFC multithreading support

#ifdef AFX_CORE1_SEG
#pragma code_seg(AFX_CORE1_SEG)
#endif

#define SERVICE_NAME  "TTSepos"
#define SERVICE_TITLE "text to speech system Epos"
#define SERVICE_COMPANY "ISC"

static CHAR ParamPath[] = "System\\CurrentControlSet\\Services\\" SERVICE_NAME "\\Parameters";

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <winreg.h>
#include <process.h>    

#define CNTSERVICE_DEF

#include "service.h"
#include "winapp.h"
#include "common.h"

BOOL ScanArgv(char *cmd, char *string, int *argc, char ***argv);
static VOID NTServiceControl(DWORD dwCtrlCode);
static VOID NTServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
static VOID NTServiceControlThread(VOID *notUsed);


/////////////////////////////////////////////////////////////////////////////
//	AfxWinMain

int AFXAPI AfxWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	int rtn;
	int argc;
	int i;
	char **argv;
	char * arg;

    theService.hInstance	 = hInstance;
    theService.hPrevInstance = hPrevInstance;
    theService.lpCmdLine	 = lpCmdLine;
    theService.nCmdShow		 = nCmdShow;

	//  Look at command line parameters
    if (ScanArgv("Service", lpCmdLine, &argc, &argv))
    {
        theService.fService = TRUE;

        for(i=1; i<argc; i++)
        {
            arg = argv[i];
            if (*arg == '-')	
            {
				// DEBUG_
                if (stricmp(arg, "-noservice") == 0)
                {
                    theService.fService = FALSE;
                    continue;
                }
				// INSTALL_
                if (stricmp(arg, "-install") == 0)
                {
                    theService.schSCManager =
                        OpenSCManager(NULL,                 // machine(NULL == local)
                                      NULL,                 // database(NULL == default)
                                      SC_MANAGER_ALL_ACCESS // access required
                                     );
                    theService.InstallService(SERVICE_NAME);
                    CloseServiceHandle(theService.schSCManager);

                    exit(0);
                }
				// REMOVE_
                if (stricmp(arg, "-remove") == 0)
                {
                    theService.schSCManager =
                        OpenSCManager(NULL,                 // machine(NULL == local)
                                      NULL,                 // database(NULL == default)
                                      SC_MANAGER_ALL_ACCESS // access required
                                      );
                    theService.RemoveService(SERVICE_NAME);
                    CloseServiceHandle(theService.schSCManager);

                    exit(0);
                }
            }
        }
    }
    else
    {
        theService.fService = FALSE;
    }

	delete [] argv;

//  Create the event object in case we are running under service controller.
//  The control handler function signals this event when it receives the "stop"
//  control code.

    theService.hServDoneEvent = CreateEvent(NULL,    // no security attributes
                                            TRUE,    // manual reset event
                                            FALSE,   // not-signalled
                                            NULL);   // no name

    if (theService.hServDoneEvent ==(HANDLE)NULL)
    {
        MessageBox(NULL, "Failed to create event", "Error", MB_OK);
        rtn = 1;
        goto cleanup;
    }

//  Start the thread that performs the work of the service controller.
    if (theService.fService)
    {
	    HANDLE h;
    
        theService.threadHandle = GetCurrentThread();
// ZS 14.7.1998
        theService.threadID		= GetCurrentThreadId();

        h =(HANDLE)_beginthread(NTServiceControlThread, 0, NULL);

        if (!h)
        {
            MessageBox(NULL, "Failed to create thread", "Error", MB_OK);
            rtn = 2;
            goto cleanup;
        }
        else
        {
            CloseHandle(h);
        }
    }

    rtn = theService.AfxWinMain(theService.hInstance, theService.hPrevInstance,
                                theService.lpCmdLine, theService.nCmdShow);

    SetEvent(theService.hServDoneEvent);

cleanup:

    return rtn;
}
// end of AfxWinMain


/////////////////////////////////////////////////////////////////////////////
//	AfxWinMain

int AFXAPI CNTService::AfxWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPTSTR lpCmdLine, int nCmdShow)
{
    ASSERT(hPrevInstance == NULL);

    int nReturnCode = -1;
    CWinApp* pApp = AfxGetApp();

//	Added By Zdenek Skalnik 1999
	AfxOleSetUserCtrl(FALSE);
    
//	Added By Zdenek Skalnik 1999,	no main window
	pApp->m_pMainWnd = NULL;

//  AFX internal initialization
    if (!AfxWinInit(hInstance, hPrevInstance, lpCmdLine, nCmdShow))
        goto InitFailure;

//  App global initializations(rare)
    ASSERT_VALID(pApp);
    if (!pApp->InitApplication())
        goto InitFailure;
    ASSERT_VALID(pApp);

//  Perform specific initializations
    if (!pApp->InitInstance())
    {
        if (pApp->m_pMainWnd != NULL)
        {
            TRACE0("Warning: Destroying non-NULL m_pMainWnd\n");
            pApp->m_pMainWnd->DestroyWindow();
        }
        nReturnCode = pApp->ExitInstance();
        goto InitFailure;
    }
    ASSERT_VALID(pApp);

    nReturnCode = pApp->Run();
    ASSERT_VALID(pApp);

InitFailure:

#ifdef _DEBUG
//  Check for missing AfxLockTempMap calls
    if (AfxGetModuleThreadState()->m_nTempMapLock != 0)
    {
        TRACE1("Warning: Temp map lock count non-zero(%ld).\n",
        AfxGetModuleThreadState()->m_nTempMapLock);
    }
    AfxLockTempMaps();
    AfxUnlockTempMaps();
#endif

    AfxWinTerm();

    SetEvent(hServDoneEvent);           // Tell the service controller thread

    return nReturnCode;
}
// end of AfxWinMain



/////////////////////////////////////////////////////////////////////////////
//	Constructor / Destructor

CNTService::CNTService()
{
    hServDoneEvent = NULL;
}

CNTService::~CNTService()
{
}
// end of CNTService


/////////////////////////////////////////////////////////////////////////////
//	RegOpenKey

long CNTService::RegOpenKey(HKEY  *hKey)
{
	long err;

    err = ::RegOpenKey(HKEY_LOCAL_MACHINE, ParamPath, hKey);
    return err;
}
// end of RegOpenKey


/////////////////////////////////////////////////////////////////////////////
//	RegCloseKey

long CNTService::RegCloseKey(HKEY  hKey)
{
	long err;

    err = ::RegCloseKey(hKey);

    return err;
}
// end of RegCloseKey


/////////////////////////////////////////////////////////////////////////////
//	StopService

VOID CNTService::StopService(VOID)
{
//  Set a termination event to stop SERVICE MAIN FUNCTION.

    SetEvent(hServDoneEvent);
}
// end of StopService


/////////////////////////////////////////////////////////////////////////////
//	InstallService

VOID CNTService::InstallService(LPCTSTR serviceName)
{
	char    BinaryPathName[256];
	HKEY    hKey;
	LONG    err;
	
	// add startup parameters ZS 17.7.1998
	CString strCommandLine = theService.lpCmdLine;

	strCommandLine.Replace("-install", "");
	strCommandLine.TrimRight();
	strCommandLine.TrimLeft();
	strCommandLine = " " + strCommandLine;	// trailing one space, delimiter

    if (!GetModuleFileName(NULL, BinaryPathName, sizeof BinaryPathName))
    {
		char msg[132];

        sprintf(msg, "Failed to get exe for service %s(%d)\n", SERVICE_NAME,GetLastError());
		MessageBox(NULL, msg, "Error", MB_OK);
        return;
    }

	strcat(BinaryPathName, strCommandLine);

    schService =
        CreateService(schSCManager,               // SCManager database
                      serviceName,                // name of service
                      SERVICE_TITLE,              // name to display
                      SERVICE_ALL_ACCESS,         // desired access
                      SERVICE_WIN32_OWN_PROCESS | // service type
                      SERVICE_INTERACTIVE_PROCESS,// New a window
                      SERVICE_DEMAND_START,       // start type
                      SERVICE_ERROR_NORMAL,       // error control type
                      BinaryPathName,             // service's binary
                      NULL,                       // no load ordering group
                      NULL,                       // no tag identifier
                      NULL,                       // no dependencies
                      NULL,                       // LocalSystem account
                      NULL);                      // no password

    if (schService == NULL)
    {
		char msg[132];

        sprintf(msg, "Failed to create service %s(%d)\n", SERVICE_NAME,
               GetLastError());
		MessageBox(NULL, msg, "Error", MB_OK);
        return;
    }
    else
    {
		char msg[132];

        sprintf(msg, "Created service %s\n", SERVICE_NAME);
		MessageBox(NULL, msg, "Completed", MB_OK);
    }

    CloseServiceHandle(schService);

    if (err = RegCreateKey(HKEY_LOCAL_MACHINE, ParamPath, &hKey))
    {
	    char msg[132];

        sprintf(msg, "Problem creating key %s(%d)\n", ParamPath, GetLastError());
		MessageBox(NULL, msg, "Error", MB_OK);
        goto ParmProb;
    }

    RegCloseKey(hKey);

ParmProb:

    return;
}
// end of InstallService


/////////////////////////////////////////////////////////////////////////////
//	RemoveService

VOID CNTService::RemoveService(LPCTSTR serviceName)
{
	BOOL    ret;

    schService = OpenService(schSCManager, serviceName, SERVICE_ALL_ACCESS);

    if (schService == NULL)
    {
		char msg[132];

        sprintf(msg, "Failed: to open service %s(%d)\n", SERVICE_NAME,GetLastError());
		MessageBox(NULL, msg, "Error", MB_OK);
        return;
    }

    ret = DeleteService(schService);

    if (ret)
	{
		char msg[132];

        sprintf(msg, "Deleted service %s\n", SERVICE_NAME);
		MessageBox(NULL, msg, "Completed", MB_OK);
	}
    else
	{
		char msg[132];

        sprintf(msg, "Failed in attempt to delete service %s(%d)\n",SERVICE_NAME, GetLastError());
		MessageBox(NULL, msg, "Error", MB_OK);
	}
}
// end of RemoveService



/////////////////////////////////////////////////////////////////////////////
//	ReportStatus

BOOL CNTService::ReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwCheckPoint, DWORD dwWaitHint)
{
	BOOL fResult;

//  Disable control requests until the service is started.

    if (dwCurrentState == SERVICE_START_PENDING)
        ssStatus.dwControlsAccepted = 0;
    else
        ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP; /*|
                                      SERVICE_ACCEPT_PAUSE_CONTINUE;*/

// ZS 14.7.1998 --- Disable PAUSE_CONTINUE

//  These SERVICE_STATUS members are set from parameters.
    ssStatus.dwCurrentState = dwCurrentState;
    ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    ssStatus.dwCheckPoint = dwCheckPoint;
    ssStatus.dwWaitHint = dwWaitHint;

//  Report the status of the service to the service control manager.
    fResult = SetServiceStatus(sshStatusHandle,    // service reference handle
                               &ssStatus);

//  If an error occurs, stop the service.

    if (!fResult)
        StopService();

    return fResult;
}
// end of ReportStatus


/////////////////////////////////////////////////////////////////////////////
//	ScanArgv

BOOL ScanArgv(char *cmd, char *string, int *argc, char ***argv)
{
	char **wargv;
	int wargc,  len;
	char *cps, *cpe, quote;

    if (!string)
        return FALSE;

//  First count the number of fields, including the command if applicable

    wargc = 0;
    if (cmd != NULL)
        wargc++;

    for(cps=string; *cps;)
    {
        while((*cps) &&(isspace(*cps))) cps++;

        if (*cps)
        {
            if (*cps == '"' || *cps == '\'')
                quote = *cps++;
            else
                quote = 0;

            if (quote)
                for(cpe=cps;(*cpe) &&(*cpe != quote); )
                {
                    if (*cpe == '\\')
                    {
                        switch(*(cpe+1))
                        {
                            case '\'':
                            case '"':
                                cpe++;
                                break;
                            default:
                                break;
                        }
                    }
                    cpe++;
                }
            else
                for(cpe=cps;(*cpe) &&(!isspace(*cpe)); ) cpe++;

            wargc++;

            cps = cpe;
        }		
    }

//  Get a buffer large enough to hold the argv and the string

    len = strlen(string) + 1;
    if (cmd)
        len += strlen(cmd) + 1;

    len +=(sizeof(char *)) * wargc;

    wargv =(char **) new char[len];
    if (argv == NULL)
        return FALSE;

//  Move the callers string to the buffer

    cps =((char *) wargv) +((sizeof(char *)) * wargc);
    strcpy(cps, string);

//  If the caller provided a command move it in also

    wargc = 0;

    if (cmd != NULL)
    {
        cpe = cps + strlen(cps) + 1;
        strcpy(cpe, cmd);
        
        *(wargv+(wargc++)) = cpe;
    }

//  Parse the string splitting into the component fields

    while(*cps)
    {
        while((*cps) &&(isspace(*cps))) cps++;

        if (*cps)
        {
            if (*cps == '"' || *cps == '\'')
                quote = *cps++;
            else
                quote = 0;

            if (quote)
                for(cpe=cps;(*cpe) &&(*cpe != quote); )   cpe++;
            else
                for(cpe=cps;(*cpe) &&(!isspace(*cpe)); ) cpe++;

            if (*cpe)
                *cpe++ = '\0';

            *(wargv+(wargc++)) = cps;

            cps = cpe;
        }		
    }

//  Now return our values

    *argc = wargc;
    *argv = wargv;

    return TRUE;
}
// end of ScanArgv


/////////////////////////////////////////////////////////////////////////////
//	NTServiceMain

static VOID NTServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	DWORD                   dwWait;

//  Register our service control handler.

    theService.sshStatusHandle =
        RegisterServiceCtrlHandler(TEXT(SERVICE_NAME),
                                  (LPHANDLER_FUNCTION)NTServiceControl);

    if (!theService.sshStatusHandle)
        goto cleanup;

//  SERVICE_STATUS members that don't change in example

    theService.ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    theService.ssStatus.dwServiceSpecificExitCode = 0;

//  Report the status to Service Control Manager.

    if (!theService.ReportStatus(SERVICE_START_PENDING, // service state
                                   NO_ERROR,              // exit code
                                   1,                     // checkpoint
                                   3000))                 // wait hint
        goto cleanup;

//  Report the status to the service control manager.

    if (!theService.ReportStatus(SERVICE_START_PENDING, // service state
                                   NO_ERROR,              // exit code
                                   2,                     // checkpoint
                                   3000))                 // wait hint
        goto cleanup;

//  Report the status to the service control manager.

    if (!theService.ReportStatus(SERVICE_RUNNING, // service state
                                   NO_ERROR,        // exit code
                                   0,               // checkpoint
                                   0))              // wait hint
        goto cleanup;

//  Wait indefinitely until hServDoneEvent is signaled.

    dwWait = WaitForSingleObject(theService.hServDoneEvent, // event object
                                 INFINITE);					// wait indefinitely
cleanup:

//  When we error or terminate via service request we come through here.

    if (theService.hServDoneEvent != NULL)
        CloseHandle(theService.hServDoneEvent);

//  Try to report the stopped status to the service control manager.

    if (theService.sshStatusHandle != 0)
       (VOID)theService.ReportStatus(SERVICE_STOPPED,
                                      theService.dwGlobalErr,
                                      0,
                                      0);

//  exit, this takes down any remaining threads, like the application thread.

//	exit(0);

// ZS 14.7.1998 --- For calling theApp.ExitInstance()
	PostThreadMessage(theService.threadID, 
					  WM_QUIT,
					  0,
					  0); 
    return;
}
// end of NTServiceMain


/////////////////////////////////////////////////////////////////////////////
//	NTServiceControl

static VOID NTServiceControl(DWORD dwCtrlCode)
{
    DWORD  dwState = SERVICE_RUNNING;

//  Handle the requested control code.

    switch(dwCtrlCode)
    {

//  Pause the service if it is running.

        case SERVICE_CONTROL_PAUSE:

            if (theService.ssStatus.dwCurrentState == SERVICE_RUNNING)
            {
                SuspendThread(theService.threadHandle);
                dwState = SERVICE_PAUSED;
            }

            break;

//  Resume the paused service.

        case SERVICE_CONTROL_CONTINUE:

            if (theService.ssStatus.dwCurrentState == SERVICE_PAUSED)
            {
                ResumeThread(theService.threadHandle);
                dwState = SERVICE_RUNNING;
            }

            break;

//  Stop the service.

        case SERVICE_CONTROL_STOP:

            dwState = SERVICE_STOP_PENDING;

//  Report the status, specifying the checkpoint and waithint, before setting
//  the termination event.

            theService.ReportStatus(SERVICE_STOP_PENDING, // current state
                                    NO_ERROR,             // exit code
                                    1,                    // checkpoint
                                    3000);                // waithint

            SetEvent(theService.hServDoneEvent);

            return;

//  Update the service status.

        case SERVICE_CONTROL_INTERROGATE:

            break;

//  Invalid control code

        default:

            break;

    }

//  Send a status response.

    theService.ReportStatus(dwState, NO_ERROR, 0, 0);
}
// end of NTServiceControl


/////////////////////////////////////////////////////////////////////////////
//	NTServiceControlThread

static VOID NTServiceControlThread(VOID *notUsed)
{
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ TEXT(SERVICE_NAME),(LPSERVICE_MAIN_FUNCTION)NTServiceMain },
		{ NULL, NULL }
	};

    StartServiceCtrlDispatcher(dispatchTable);

    return;
}
// end of NTServiceControlThread



///////////////////////////////////////////////////////////////////////////
//incipit TTSepos.cpp
///////////////////////////////////////////////////////////////////////////

// TTSepos.cpp : Defines the class behaviors for the application.
//

//#include "stdafx.h"
#include "TTSepos.h"
#include "TTSeposService.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



HANDLE MCreateThread(
	 LPSECURITY_ATTRIBUTES lpThreadAttributes, // SD
	 DWORD dwStackSize,                        // initial stack size
	 LPTHREAD_START_ROUTINE lpStartAddress,    // thread function
	 LPVOID lpParameter,                       // thread argument
	 DWORD dwCreationFlags,                    // creation option
	 LPDWORD lpThreadId                        // thread identifier
	 ) {
	return CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
}

/////////////////////////////////////////////////////////////////////////////
// CTTSeposApp

BEGIN_MESSAGE_MAP(CTTSeposApp, CWinApp)
	//{{AFX_MSG_MAP(CTTSeposApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTTSeposApp construction

CTTSeposApp::CTTSeposApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTTSeposApp object

CTTSeposApp theApp;

/////////////////////////////////////////////////////////////////////////////
// InitInstance

BOOL CTTSeposApp::InitInstance()
{
	CString strCommandLine = theService.lpCmdLine;

	// _DEBUG - program argument -noservice
	if(!theService.fService)
	{

	}

	// Windows sockets support
	if (!AfxSocketInit())
	{
		return FALSE;
	}

	m_threadHandle = MCreateThread(NULL, 0, ThreadGo, (LPVOID) this,0,NULL);
	
	return TRUE;
}
// end of InitInstance


/////////////////////////////////////////////////////////////////////////////
// ExitInstance

int CTTSeposApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class

	return CWinApp::ExitInstance();
}
// end of ExitInstance



DWORD WINAPI CTTSeposApp::ThreadGo(LPVOID pData) {
	
	MessageBox(NULL,"dobry den jsem tu","Ahoj",MB_OK);
	
	while(true) {
		Beep(2000,50);
		Sleep(500);
	}
	
	//od->SamplingThread();
	return NOERROR;
}







