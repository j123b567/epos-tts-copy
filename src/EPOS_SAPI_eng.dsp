# Microsoft Developer Studio Project File - Name="EPOS_SAPI_eng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=EPOS_SAPI_eng - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EPOS_SAPI_eng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EPOS_SAPI_eng.mak" CFG="EPOS_SAPI_eng - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EPOS_SAPI_eng - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "EPOS_SAPI_eng - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "EPOS_SAPI_eng - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "EPOS_SAPI_eng - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "EPOS_SAPI_eng - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "EPOS_SAPI_eng - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "SAPI_Debug"
# PROP Intermediate_Dir "SAPI_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "sapi\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD MTL /I "sapi/IDL"
# SUBTRACT MTL /nologo /mktyplib203 /Oicf
# ADD BASE RSC /l 0x405 /d "_DEBUG"
# ADD RSC /l 0x405 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib advapi32.lib ole32.lib ws2_32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:sept /libpath:"sapi/lib/i386"
# Begin Custom Build - Performing registration
OutDir=.\SAPI_Debug
TargetPath=.\SAPI_Debug\EPOS_SAPI_eng.dll
InputPath=.\SAPI_Debug\EPOS_SAPI_eng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD MTL /I "sapi/IDL"
# ADD BASE RSC /l 0x405 /d "_DEBUG"
# ADD RSC /l 0x405 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=.\DebugU\EPOS_SAPI_eng.dll
InputPath=.\DebugU\EPOS_SAPI_eng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinSize"
# PROP BASE Intermediate_Dir "ReleaseMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinSize"
# PROP Intermediate_Dir "ReleaseMinSize"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /I "sapi\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR /Yu"stdafx.h" /FD /c
# ADD MTL /I "sapi/IDL"
# ADD BASE RSC /l 0x405 /d "NDEBUG"
# ADD RSC /l 0x405 /i "sapi/include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib advapi32.lib ole32.lib libcmt.lib ws2_32.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"sapi/lib/i386"
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\EPOS_SAPI_eng.dll
InputPath=.\ReleaseMinSize\EPOS_SAPI_eng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinDependency"
# PROP BASE Intermediate_Dir "ReleaseMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinDependency"
# PROP Intermediate_Dir "ReleaseMinDependency"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD MTL /I "sapi/IDL"
# ADD BASE RSC /l 0x405 /d "NDEBUG"
# ADD RSC /l 0x405 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\EPOS_SAPI_eng.dll
InputPath=.\ReleaseMinDependency\EPOS_SAPI_eng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinSize"
# PROP BASE Intermediate_Dir "ReleaseUMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinSize"
# PROP Intermediate_Dir "ReleaseUMinSize"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD MTL /I "sapi/IDL"
# ADD BASE RSC /l 0x405 /d "NDEBUG"
# ADD RSC /l 0x405 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\EPOS_SAPI_eng.dll
InputPath=.\ReleaseUMinSize\EPOS_SAPI_eng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinDependency"
# PROP BASE Intermediate_Dir "ReleaseUMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "SAPI_Debug"
# PROP Intermediate_Dir "SAPI_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /ZI /Od /I "sapi\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /FD /c
# ADD MTL /I "sapi/IDL"
# ADD BASE RSC /l 0x405 /d "NDEBUG"
# ADD RSC /l 0x405 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libcmtd.lib libcmt.lib /nologo /subsystem:windows /dll /incremental:yes /debug /machine:I386 /pdbtype:sept /libpath:"sapi/lib/i386"
# SUBTRACT LINK32 /map /nodefaultlib
# Begin Custom Build - Performing registration
OutDir=.\SAPI_Debug
TargetPath=.\SAPI_Debug\EPOS_SAPI_eng.dll
InputPath=.\SAPI_Debug\EPOS_SAPI_eng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "EPOS_SAPI_eng - Win32 Debug"
# Name "EPOS_SAPI_eng - Win32 Unicode Debug"
# Name "EPOS_SAPI_eng - Win32 Release MinSize"
# Name "EPOS_SAPI_eng - Win32 Release MinDependency"
# Name "EPOS_SAPI_eng - Win32 Unicode Release MinSize"
# Name "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\EPOS_SAPI_eng.cpp

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

# PROP Intermediate_Dir "Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\EPOS_SAPI_eng.def

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

# PROP Intermediate_Dir "Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\EPOS_SAPI_eng.idl

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"
# ADD MTL /tlb ".\EPOS_SAPI_eng.tlb" /h "EPOS_SAPI_eng.h" /iid "EPOS_SAPI_eng_i.c" /Oicf

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

# ADD MTL /tlb ".\EPOS_SAPI_eng.tlb" /h "EPOS_SAPI_eng.h" /iid "EPOS_SAPI_eng_i.c" /Oicf

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

# ADD MTL /tlb ".\EPOS_SAPI_eng.tlb" /h "EPOS_SAPI_eng.h" /iid "EPOS_SAPI_eng_i.c" /Oicf

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

# ADD MTL /tlb ".\EPOS_SAPI_eng.tlb" /h "EPOS_SAPI_eng.h" /iid "EPOS_SAPI_eng_i.c" /Oicf

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

# ADD MTL /tlb ".\EPOS_SAPI_eng.tlb" /h "EPOS_SAPI_eng.h" /iid "EPOS_SAPI_eng_i.c" /Oicf

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

# PROP Intermediate_Dir "Debug"
# ADD MTL /tlb ".\EPOS_SAPI_eng.tlb" /h "EPOS_SAPI_eng.h" /iid "EPOS_SAPI_eng_i.c" /Oicf

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\EPOS_SAPI_eng.rc

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"
# ADD BASE RSC /l 0x405
# ADD RSC /l 0x405

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

# PROP Intermediate_Dir "Debug"
# ADD BASE RSC /l 0x405
# ADD RSC /l 0x405

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

# PROP Intermediate_Dir "Debug"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TTS_Engine.cpp

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

# PROP Intermediate_Dir "Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Resource.h

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sapi_client.h
# End Source File
# Begin Source File

SOURCE=.\say_client.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TTS_Engine.h

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\TTS_Engine.rgs

!IF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Debug"

# PROP Intermediate_Dir "SAPI_Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "EPOS_SAPI_eng - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
