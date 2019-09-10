CONFIG     += qt warn_on release thread static rtti link_prl

win32:DEFINES += _MT _WIN32_WINNT=0x0501 NDEBUG __WIN32__ _USE_MATH_DEFINES _CRT_SECURE_NO_DEPRECATE

win32:RC_FILE  = ./audiomove.rc
mac:RC_FILE    = ./audiomove.icns

win32:LIBS    += .\akrip\akrip32.lib ..\libsndfile\libsndfile-1.lib ..\libsamplerate\libsamplerate-0.lib shlwapi.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib winmm.lib ws2_32.lib winspool.lib delayimp.lib iphlpapi.lib
unix:LIBS     += ../libsamplerate/temp_install/lib/libsamplerate.a ../libsndfile/temp_install/lib/libsndfile.a ../libvorbis/temp_install/lib/libvorbis.a ../libvorbis/temp_install/lib/libvorbisfile.a ../libvorbis/temp_install/lib/libvorbisenc.a ../flac/temp_install/lib/libFLAC.a ../libogg/temp_install/lib/libogg.a 
mac:LIBS      += -lz -framework IOKit -framework Carbon

DEFINES    += QT_NO_CAST_ASCII MUSCLE_ENABLE_ZLIB_ENCODING

MOC_DIR     = moc

HEADERS     = ../audiomove/AudioMoveWindow.h       \
              ../audiomove/CreateLCSDisk.h         \
              ../audiomove/AudioMoveFileDialog.h   \ 
              ../audiomove/AudioMovePopupMenu.h    \
              ../audiomove/AudioMoveMessageBox.h

TARGET      = AudioMove

INCLUDEPATH = . .. ../muscle ../libsndfile ../libsndfile/src ../libsamplerate/src

OBJECTS_DIR = objects

win32:SOURCES += ../muscle/zlib/zlib/compress.c                     \
                 ../muscle/winsupport/Win32FileHandleDataIO.cpp     \
                 ../audiomove/AKRipThread.cpp                       \
                 ../audiomove/Win32dirent.cpp

!mac:SOURCES += ../muscle/zlib/zlib/adler32.c  \
                ../muscle/zlib/zlib/deflate.c  \
                ../muscle/zlib/zlib/trees.c    \
                ../muscle/zlib/zlib/zutil.c    \
                ../muscle/zlib/zlib/inflate.c  \
                ../muscle/zlib/zlib/inftrees.c \
                ../muscle/zlib/zlib/inffast.c  \
                ../muscle/zlib/zlib/crc32.c

SOURCES    += ../audiomove/audiomove.cpp           \
              ../audiomove/AudioMoveWindow.cpp     \
              ../audiomove/AudioMoveThread.cpp     \
              ../audiomove/CreateLCSDisk.cpp       \
              ../audiomove/LibSndFileIOThread.cpp  \
              ../audiomove/MiscFunctions.cpp       \
              ../audiomove/AudioMoveFileDialog.cpp \
              ../audiomove/AudioMoveMessageBox.cpp \
              ../audiomove/AudioMovePopupMenu.cpp  \
              ../audiomove/SampleRateThread.cpp    \
              ../muscle/message/Message.cpp        \
              ../muscle/regex/StringMatcher.cpp    \
              ../muscle/syslog/SysLog.cpp          \
              ../muscle/system/SetupSystem.cpp     \
              ../muscle/system/SystemInfo.cpp      \
              ../muscle/system/SharedMemory.cpp    \
              ../muscle/system/Thread.cpp          \
              ../muscle/util/ByteBuffer.cpp        \
              ../muscle/util/MiscUtilityFunctions.cpp    \
              ../muscle/util/FilePathInfo.cpp      \
              ../muscle/util/SocketMultiplexer.cpp \
              ../muscle/util/Directory.cpp         \
              ../muscle/util/NetworkUtilityFunctions.cpp \
              ../muscle/util/String.cpp                  \
              ../muscle/zlib/ZLibCodec.cpp               \
              ../muscle/zlib/ZLibUtilityFunctions.cpp
