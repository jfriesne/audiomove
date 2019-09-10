# Microsoft Developer Studio Project File - Name="AudioMove" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=AudioMove - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "AudioMove.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AudioMove.mak" CFG="AudioMove - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AudioMove - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "AudioMove - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "AudioMove - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "objects"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "objects"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /MD /W3 /GR /O1 /I "$(QTDIR)\include" /I "..\..\qtnet\audiomove" /I "..\..\libsamplerate\Win32" /I "c:\qt\3.3.3\include" /I "." /I ".." /I "..\..\muscle" /I "..\..\libsndfile\src" /I "..\..\libsamplerate\src" /I "moc" /I "c:\qt\3.3.3\mkspecs\win32-msvc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "QT_THREAD_SUPPORT" /D "QT_NO_DEBUG" /D "UNICODE" /D "_MT" /D "QT_NO_CAST_ASCII" /D CPU_IS_LITTLE_ENDIAN=1 /D CPU_IS_BIG_ENDIAN=0 /D _WIN32_WINNT=0x0500 /D inline=__inline /D "LIBSNDFILE_IMPORTS" /FD -Zm200 /c
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 delayimp.lib "qt-mt.lib" "qtmain.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "imm32.lib" "winmm.lib" "wsock32.lib" "winspool.lib" /nologo /subsystem:windows /machine:IX86 /libpath:"$(QTDIR)\lib" /DELAYLOAD:comdlg32.dll /DELAYLOAD:oleaut32.dll /DELAYLOAD:winmm.dll /DELAYLOAD:wsock32.dll /DELAYLOAD:winspool.dll

!ELSEIF  "$(CFG)" == "AudioMove - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /MDd /W3 /GR /Zi /Od /I "..\..\libsamplerate\Win32" /I "c:\qt\3.3.3\include" /I "." /I ".." /I "..\..\muscle" /I "..\..\libsndfile\src" /I "..\..\libsamplerate\src" /I "moc" /I "c:\qt\3.3.3\mkspecs\win32-msvc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "QT_THREAD_SUPPORT" /D "QT_NO_DEBUG" /D "UNICODE" /D "_MT" /D "QT_NO_CAST_ASCII" /D CPU_IS_LITTLE_ENDIAN=1 /D CPU_IS_BIG_ENDIAN=0 /D _WIN32_WINNT=0x0500 /D inline=__inline /D "LIBSNDFILE_IMPORTS" /FD /GZ -Zm200 /c
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 "qt-mt.lib" "qtmain.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "imm32.lib" "winmm.lib" "wsock32.lib" "winspool.lib" /nologo /subsystem:windows /debug /machine:IX86 /pdbtype:sept /libpath:"$(QTDIR)\lib"

!ENDIF 

# Begin Target

# Name "AudioMove - Win32 Release"
# Name "AudioMove - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\audiomove\audiomove.cpp
# End Source File
# Begin Source File

SOURCE=audiomove.rc
# End Source File
# Begin Source File

SOURCE=..\audiomove\AudioMoveThread.cpp
# End Source File
# Begin Source File

SOURCE=..\audiomove\AudioMoveWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\util\ByteBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\audiomove\ErrorIOThread.cpp
# End Source File
# Begin Source File

SOURCE=..\audiomove\LibSndFileIOThread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\message\Message.cpp
# End Source File
# Begin Source File

SOURCE=..\audiomove\MiscFunctions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\util\MiscUtilityFunctions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\util\NetworkUtilityFunctions.cpp
# End Source File
# Begin Source File

SOURCE=..\audiomove\RawIOThread.cpp
# End Source File
# Begin Source File

SOURCE=..\audiomove\SampleRateThread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\system\SetupSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\system\SharedMemory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\util\String.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\syslog\SysLog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\system\Thread.cpp
# End Source File
# Begin Source File

SOURCE=.\win32dirent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\muscle\winsupport\Win32FileHandleDataIO.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AudioMoveConstants.h
# End Source File
# Begin Source File

SOURCE=.\AudioMoveNameSpace.h
# End Source File
# Begin Source File

SOURCE=.\AudioMoveThread.h
# End Source File
# Begin Source File

SOURCE=..\audiomove\AudioMoveWindow.h

!IF  "$(CFG)" == "AudioMove - Win32 Release"

USERDEP__AUDIO=""$(QTDIR)\bin\moc.exe""	
# Begin Custom Build - Moc'ing ..\audiomove\AudioMoveWindow.h...
InputPath=..\audiomove\AudioMoveWindow.h

"moc\moc_AudioMoveWindow.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(QTDIR)\bin\moc ..\audiomove\AudioMoveWindow.h -o moc\moc_AudioMoveWindow.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "AudioMove - Win32 Debug"

USERDEP__AUDIO=""$(QTDIR)\bin\moc.exe""	
# Begin Custom Build - Moc'ing ..\audiomove\AudioMoveWindow.h...
InputPath=..\audiomove\AudioMoveWindow.h

"moc\moc_AudioMoveWindow.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(QTDIR)\bin\moc ..\audiomove\AudioMoveWindow.h -o moc\moc_AudioMoveWindow.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\muscle\util\ByteBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\dataio\DataIO.h
# End Source File
# Begin Source File

SOURCE=.\ErrorIOThread.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\dataio\FileDescriptorDataIO.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\util\FlatCountable.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\support\Flattenable.h
# End Source File
# Begin Source File

SOURCE=.\LibSndFileIOThread.h
# End Source File
# Begin Source File

SOURCE=.\MiscFunctions.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\support\MuscleSupport.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\support\Point.h
# End Source File
# Begin Source File

SOURCE=.\RawIOThread.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\support\Rect.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\util\RefCount.h
# End Source File
# Begin Source File

SOURCE=.\SampleRateThread.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\dataio\TCPSocketDataIO.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\support\Tuple.h
# End Source File
# Begin Source File

SOURCE=.\win32dirent.h
# End Source File
# Begin Source File

SOURCE=..\..\muscle\winsupport\Win32FileHandleDataIO.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Generated"

# PROP Default_Filter ""
# Begin Source File

SOURCE=moc\moc_AudioMoveWindow.cpp
# End Source File
# End Group
# Begin Group "libsamplerate"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\libsamplerate\src\common.h
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\config.h
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\fastest_coeffs.h
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\float_cast.h
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\high_qual_coeffs.h
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\mid_qual_coeffs.h
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\samplerate.c
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\samplerate.h
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\src_linear.c
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\src_sinc.c
# End Source File
# Begin Source File

SOURCE=..\..\libsamplerate\src\src_zoh.c
# End Source File
# End Group
# End Target
# End Project
