# Microsoft Developer Studio Project File - Name="xmltest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=xmltest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xmltest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xmltest.mak" CFG="xmltest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xmltest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "xmltest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xmltest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x405 /d "NDEBUG"
# ADD RSC /l 0x405 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "xmltest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x405 /d "_DEBUG"
# ADD RSC /l 0x405 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "xmltest - Win32 Release"
# Name "xmltest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "stream"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\stream\bang_fstream.cpp
# End Source File
# Begin Source File

SOURCE=..\stream\bang_fstream.h
# End Source File
# Begin Source File

SOURCE=..\stream\bang_ios.h
# End Source File
# Begin Source File

SOURCE=..\stream\bang_iostream.cpp
# End Source File
# Begin Source File

SOURCE=..\stream\bang_iostream.h
# End Source File
# Begin Source File

SOURCE=..\stream\bang_istream.cpp
# End Source File
# Begin Source File

SOURCE=..\stream\bang_istream.h
# End Source File
# Begin Source File

SOURCE=..\stream\bang_ostream.cpp
# End Source File
# Begin Source File

SOURCE=..\stream\bang_ostream.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\enumstring.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=..\map.cpp
# End Source File
# Begin Source File

SOURCE=..\matrix.cpp
# End Source File
# Begin Source File

SOURCE=..\set.cpp
# End Source File
# Begin Source File

SOURCE=.\shriek.cpp
# End Source File
# Begin Source File

SOURCE=..\string.cpp
# End Source File
# Begin Source File

SOURCE=..\utils.cpp
# End Source File
# Begin Source File

SOURCE=..\xml.cpp
# End Source File
# Begin Source File

SOURCE=..\xml.y.tab.cpp
# End Source File
# Begin Source File

SOURCE=..\xmlutils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\string.h
# End Source File
# Begin Source File

SOURCE=..\types.h
# End Source File
# Begin Source File

SOURCE=..\xml.h
# End Source File
# Begin Source File

SOURCE=..\xml.y.tab.cpp.h
# End Source File
# Begin Source File

SOURCE=..\xmlutils.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\trproc1.txt
# End Source File
# Begin Source File

SOURCE=..\xml.y

!IF  "$(CFG)" == "xmltest - Win32 Release"

!ELSEIF  "$(CFG)" == "xmltest - Win32 Debug"

# Begin Custom Build - Bison on $(InputPath)
InputPath=..\xml.y
InputName=xml

"$(InputPath).tab.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	bison.exe -p$(InputName) -o $(InputPath).tab.cpp -d ../$(InputName).y

# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
