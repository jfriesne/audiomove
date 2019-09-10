# Microsoft Developer Studio Project File - Name="lcssndfile" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=lcssndfile - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lcssndfile.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lcssndfile.mak" CFG="lcssndfile - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lcssndfile - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lcssndfile - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lcssndfile - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LCSSNDFILE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\libsndfile\Win32" /I "..\..\libsndfile\src" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBSNDFILE_EXPORTS" /D _WIN32_WINNT=0x0500 /D inline=__inline /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "lcssndfile - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "lcssndfile___Win32_Debug"
# PROP BASE Intermediate_Dir "lcssndfile___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "lcssndfile___Win32_Debug"
# PROP Intermediate_Dir "lcssndfile___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LCSSNDFILE_EXPORTS" /YX /FD /GZ  /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\libsndfile\Win32" /I "..\..\libsndfile\src" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBSNDFILE_EXPORTS" /D _WIN32_WINNT=0x0500 /D inline=__inline /YX /FD /GZ  /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug/lcssndfile.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "lcssndfile - Win32 Release"
# Name "lcssndfile - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "libsndfile"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\add.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\aiff.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\alaw.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\au.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\au.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\au_g72x.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\avr.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\code.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\command.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\common.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\common.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\config.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\config.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\decode.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\dither.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\double64.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\dwd.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\dwvw.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\file_io.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\float32.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\float_cast.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\G72x\g721.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\G72x\g723_16.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\G72x\g723_24.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\G72x\g723_40.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\G72x\g72x.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\G72x\g72x.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\G72x\g72x_priv.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\gsm.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\gsm610.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\gsm610_priv.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\gsm_create.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\gsm_decode.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\gsm_destroy.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\gsm_encode.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\gsm_option.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\htk.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\ima_adpcm.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\interleave.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\ircam.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\long_term.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\lpc.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\mat4.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\mat5.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\ms_adpcm.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\nist.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\ogg.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\paf.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\pcm.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\preprocess.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\pvf.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\raw.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\rpe.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\rx2.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\sd2.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\sds.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\sf_unistd.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\sfendian.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\sfendian.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\short_term.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\sndfile.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\sndfile.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\strings.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\svx.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\GSM610\table.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\txw.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\ulaw.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\voc.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\vox_adpcm.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\w64.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\wav.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\wav_w64.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\wav_w64.h
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\wve.c
# End Source File
# Begin Source File

SOURCE=..\..\libsndfile\src\xi.c
# End Source File
# End Group
# End Target
# End Project
