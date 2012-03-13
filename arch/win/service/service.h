////////////////////////////////////////////////////////////////////////////
// CNTService Class

#ifndef CNTSERVICE_H
#define CNTSERVICE_H

#include <winsvc.h>

class CNTService
{
public:

//  Constructor/Destructor

    CNTService();
    ~CNTService();

public:

//  Stuff from the entry call to AfxWinMain

    HINSTANCE hInstance;
    HINSTANCE hPrevInstance;
    LPTSTR lpCmdLine;
    int nCmdShow;

//  Things we need to track for the service

    HANDLE hServDoneEvent;
    SC_HANDLE schService;
    SC_HANDLE schSCManager;
    SERVICE_STATUS ssStatus;            // current status of the service
    SERVICE_STATUS_HANDLE sshStatusHandle;
    BOOL fService;
    HANDLE threadHandle;
// ZS 14.7.1998
	DWORD threadID;
    DWORD dwGlobalErr;

//  Declare the routines.

    int AFXAPI AfxWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                          LPTSTR lpCmdLine, int nCmdShow);

    VOID StopService(VOID);
    VOID InstallService(LPCTSTR serviceName);
    VOID RemoveService(LPCTSTR serviceName);
    BOOL ReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                      DWORD dwCheckPoint, DWORD dwWaitHint);
    long CNTService::RegOpenKey(HKEY  *hKey);
    long CNTService::RegCloseKey(HKEY  hKey);
} ;

////////////////////////////////////////////////////////////////////////////
//  We only allow one per application so CNTSERVICE_DEF should only be defined
//  in service.cpp

#ifdef CNTSERVICE_DEF
    CNTService theService;
#else
    extern CNTService theService;
#endif

#endif // CNTSERVICE_H
