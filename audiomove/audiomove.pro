CONFIG     += qt warn_on release thread rtti link_prl

mac {
   BUILD_UNIVERSAL_BINARY = $$(BUILD_UNIVERSAL_BINARY)
   isEmpty(BUILD_UNIVERSAL_BINARY) {
      warning("Environment variable BUILD_UNIVERSAL_BINARY not detected:  Building for native architecture only")
   } else {
      warning("Environment variable BUILD_UNIVERSAL_BINARY detected:  Building for both x86_64 and arm64 architectures")
      mac:QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64
   }
}

ENABLE_ADDRESS_SANITIZER = $$(ENABLE_ADDRESS_SANITIZER)
!isEmpty(ENABLE_ADDRESS_SANITIZER) {
   warning("Environment variable ENABLE_ADDRESS_SANITIZER detected:  Enabling clang sanitizer flags")
   QMAKE_CXXFLAGS += -fsanitize=address,undefined -g
   QMAKE_LFLAGS   += -fsanitize=address,undefined
}

ENABLE_THREAD_SANITIZER = $$(ENABLE_THREAD_SANITIZER)
!isEmpty($$FLAGSDIR/ENABLE_THREAD_SANITIZER) {
   warning("Environment variable ENABLE_THREAD_SANITIZER detected:  Enabling clang thread-sanitizer flags")
   QMAKE_CXXFLAGS += -fsanitize=thread -g
   QMAKE_LFLAGS   += -fsanitize=thread
}

greaterThan(QT_MAJOR_VERSION, 4) {
   QT += widgets
}

SUBMODULESDIR = ../submodules

win32:DEFINES += _MT _WIN32_WINNT=0x0501 NDEBUG __WIN32__ _USE_MATH_DEFINES _CRT_SECURE_NO_DEPRECATE

win32:RC_FILE  = ./audiomove.rc
mac:RC_FILE    = ./audiomove.icns

win32:LIBS    += $$SUBMODULESDIR\libsndfile\temp_install\lib\sndfile.lib $$SUBMODULESDIR\libsamplerate\temp_install\lib\samplerate.lib shlwapi.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib winmm.lib ws2_32.lib winspool.lib delayimp.lib iphlpapi.lib psapi.lib
unix:LIBS     += $$SUBMODULESDIR/libsamplerate/temp_install/lib/libsamplerate.a $$SUBMODULESDIR/libsndfile/temp_install/lib/libsndfile.a $$SUBMODULESDIR/vorbis/temp_install/lib/libvorbis.a $$SUBMODULESDIR/vorbis/temp_install/lib/libvorbisfile.a $$SUBMODULESDIR/vorbis/temp_install/lib/libvorbisenc.a $$SUBMODULESDIR/flac/temp_install/lib/libFLAC.a $$SUBMODULESDIR/ogg/temp_install/lib/libogg.a $$SUBMODULESDIR/opus/temp_install/lib/libopus.a $$SUBMODULESDIR/lame/temp_install/lib/libmp3lame.a $$SUBMODULESDIR/mpg123/temp_install/lib/libmpg123.a
mac:LIBS      += -lz -framework IOKit -framework Carbon -framework SystemConfiguration

DEFINES    += QT_NO_CAST_ASCII MUSCLE_ENABLE_ZLIB_ENCODING

MOC_DIR     = moc

HEADERS     = AudioMoveWindow.h       \
              AudioMoveFileDialog.h   \ 
              AudioMovePopupMenu.h    \
              AudioMoveMessageBox.h

TARGET      = AudioMove

INCLUDEPATH = .. $$SUBMODULESDIR $$SUBMODULESDIR/muscle $$SUBMODULESDIR/libsndfile/temp_install/include $$SUBMODULESDIR/libsndfile/src $$SUBMODULESDIR/libsamplerate/temp_install/include

OBJECTS_DIR = objects

win32:INCLUDEPATH += $$SUBMODULESDIR/muscle/regex/regex $$SUBMODULESDIR/muscle/zlib/zlib

win32:SOURCES += $$SUBMODULESDIR/muscle/zlib/zlib/compress.c                 \
                 $$SUBMODULESDIR/muscle/platform/win32/Win32FileHandleDataIO.cpp \
                 $$SUBMODULESDIR/muscle/regex/regex/regcomp.c                \
                 $$SUBMODULESDIR/muscle/regex/regex/regerror.c               \
                 $$SUBMODULESDIR/muscle/regex/regex/regexec.c                \
                 $$SUBMODULESDIR/muscle/regex/regex/regfree.c                \
                 $$SUBMODULESDIR/muscle/zlib/zlib/gzclose.c                  \
                 $$SUBMODULESDIR/muscle/zlib/zlib/gzlib.c                    \
                 $$SUBMODULESDIR/muscle/zlib/zlib/gzread.c                   \
                 $$SUBMODULESDIR/muscle/zlib/zlib/gzwrite.c                  \
                 Win32dirent.cpp

!mac:SOURCES += $$SUBMODULESDIR/muscle/zlib/zlib/adler32.c  \
                $$SUBMODULESDIR/muscle/zlib/zlib/deflate.c  \
                $$SUBMODULESDIR/muscle/zlib/zlib/trees.c    \
                $$SUBMODULESDIR/muscle/zlib/zlib/zutil.c    \
                $$SUBMODULESDIR/muscle/zlib/zlib/inflate.c  \
                $$SUBMODULESDIR/muscle/zlib/zlib/inftrees.c \
                $$SUBMODULESDIR/muscle/zlib/zlib/inffast.c  \
                $$SUBMODULESDIR/muscle/zlib/zlib/crc32.c

SOURCES    += audiomove.cpp           \
              AudioFormatInfo.cpp     \
              AudioMoveWindow.cpp     \
              AudioMoveThread.cpp     \
              LibSndFileIOThread.cpp  \
              MiscFunctions.cpp       \
              AudioMoveFileDialog.cpp \
              AudioMoveMessageBox.cpp \
              AudioMovePopupMenu.cpp  \
              SampleRateThread.cpp    \
              $$SUBMODULESDIR/muscle/dataio/FileDataIO.cpp            \
              $$SUBMODULESDIR/muscle/message/Message.cpp              \
              $$SUBMODULESDIR/muscle/regex/StringMatcher.cpp          \
              $$SUBMODULESDIR/muscle/syslog/SysLog.cpp                \
              $$SUBMODULESDIR/muscle/system/SetupSystem.cpp           \
              $$SUBMODULESDIR/muscle/system/StackTrace.cpp            \
              $$SUBMODULESDIR/muscle/system/SystemInfo.cpp            \
              $$SUBMODULESDIR/muscle/system/SharedMemory.cpp          \
              $$SUBMODULESDIR/muscle/system/Thread.cpp                \
              $$SUBMODULESDIR/muscle/util/ByteBuffer.cpp              \
              $$SUBMODULESDIR/muscle/util/MiscUtilityFunctions.cpp    \
              $$SUBMODULESDIR/muscle/util/FilePathInfo.cpp            \
              $$SUBMODULESDIR/muscle/util/SocketMultiplexer.cpp       \
              $$SUBMODULESDIR/muscle/util/Directory.cpp               \
              $$SUBMODULESDIR/muscle/util/NetworkUtilityFunctions.cpp \
              $$SUBMODULESDIR/muscle/util/String.cpp                  \
              $$SUBMODULESDIR/muscle/util/StringTokenizer.cpp         \
              $$SUBMODULESDIR/muscle/zlib/ZLibCodec.cpp               \
              $$SUBMODULESDIR/muscle/zlib/ZLibUtilityFunctions.cpp


