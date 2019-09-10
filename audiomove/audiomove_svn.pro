CONFIG     += qt warn_on release thread static rtti link_prl

isEmpty( SVN_VERSION_STRING ) {
    DEFINES += SVN_VERSION_STRING=\\\"$$system(svnversion .)\\\"
}

win32:DEFINES += _MT _WIN32_WINNT=0x0501 NDEBUG __WIN32__ _USE_MATH_DEFINES _CRT_SECURE_NO_DEPRECATE

win32:RC_FILE  = ./audiomove.rc
mac:RC_FILE    = ./audiomove.icns

win32:LIBS    += .\akrip\akrip32.lib ..\..\libsndfile\win32\libsndfile.lib release\libsamplerate.lib shlwapi.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib winmm.lib wsock32.lib winspool.lib delayimp.lib iphlpapi.lib
unix:LIBS     += ../../libsamplerate/src/.libs/libsamplerate.a ../../libsndfile/src/.libs/libsndfile.a # -lsndfile -lsamplerate
mac:LIBS      += -lz -framework IOKit -framework Carbon

# For Linux debug mode, enable stack-trace-printing
unix:!mac:QMAKE_LFLAGS_DEBUG   += -rdynamic
unix:!mac:QMAKE_CFLAGS_DEBUG   += -rdynamic
unix:!mac:QMAKE_CXXFLAGS_DEBUG += -rdynamic

# Ah heck, For Linux release mode too
unix:!mac:QMAKE_LFLAGS   += -rdynamic
unix:!mac:QMAKE_CFLAGS   += -rdynamic
unix:!mac:QMAKE_CXXFLAGS += -rdynamic

DEFINES       += QT_NO_CAST_ASCII MUSCLE_ENABLE_ZLIB_ENCODING

MOC_DIR        = moc

TARGET         = AudioMove

INCLUDEPATH    = . .. ../../muscle ../../lxreleasever ../../libsndfile/src ../../libsamplerate/src

OBJECTS_DIR    = objects

win32:SOURCES += ../../muscle/zlib/zlib/compress.c                     \
                 ../../muscle/winsupport/Win32FileHandleDataIO.cpp     \
                 ../audiomove/AKRipThread.cpp                          \
                 ../audiomove/Win32dirent.cpp

!mac:SOURCES += ../../muscle/zlib/zlib/adler32.c  \
                ../../muscle/zlib/zlib/compress.c \
                ../../muscle/zlib/zlib/deflate.c  \
                ../../muscle/zlib/zlib/trees.c    \
                ../../muscle/zlib/zlib/zutil.c    \
                ../../muscle/zlib/zlib/inflate.c  \
                ../../muscle/zlib/zlib/inftrees.c \
                ../../muscle/zlib/zlib/inffast.c  \
                ../../muscle/zlib/zlib/crc32.c

HEADERS    += ../audiomove/AudioMoveWindow.h       \
              ../audiomove/CreateLCSDisk.h         \
              ../audiomove/AudioMoveFileDialog.h   \ 
              ../audiomove/AudioMovePopupMenu.h    \
              ../audiomove/AudioMoveMessageBox.h

SOURCES    += ../audiomove/audiomove.cpp           \
              ../audiomove/AudioMoveWindow.cpp     \
              ../audiomove/AudioMoveThread.cpp     \
              ../audiomove/CreateLCSDisk.cpp       \
              ../audiomove/LibSndFileIOThread.cpp  \
              ../audiomove/SampleRateThread.cpp    \
              ../audiomove/MiscFunctions.cpp       \
              ../audiomove/AudioMoveFileDialog.cpp \
              ../audiomove/AudioMoveMessageBox.cpp \
              ../audiomove/AudioMovePopupMenu.cpp  \
              ../../muscle/message/Message.cpp     \
              ../../muscle/regex/StringMatcher.cpp \
              ../../muscle/syslog/SysLog.cpp       \
              ../../muscle/system/SetupSystem.cpp  \
              ../../muscle/system/SharedMemory.cpp \
              ../../muscle/system/SystemInfo.cpp   \
              ../../muscle/system/Thread.cpp       \
              ../../muscle/util/ByteBuffer.cpp     \
              ../../muscle/util/FilePathInfo.cpp            \
              ../../muscle/util/SocketMultiplexer.cpp       \
              ../../muscle/util/Directory.cpp               \
              ../../muscle/util/MiscUtilityFunctions.cpp    \
              ../../muscle/util/NetworkUtilityFunctions.cpp \
              ../../muscle/util/String.cpp                  \
              ../../muscle/zlib/ZLibCodec.cpp               \
              ../../muscle/zlib/ZLibUtilityFunctions.cpp
