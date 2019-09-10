CONFIG     += qt warn_on release thread static rtti link_prl

greaterThan(QT_MAJOR_VERSION, 4) {
   QT += widgets
}

SUBMODULESDIR = ../submodules

win32:DEFINES  += _MT WIN32

win32:RC_FILE  = ./audiomove.rc
mac:RC_FILE    = ./audiomove.icns

win32:LIBS += $$SUBMODULESDIR/libsamplerate/samplerate.lib $$SUBMODULESDIR/libsndfile/sndfile.lib

mac:LIBS += $$SUBMODULESDIR/libsamplerate/src/.libs/libsamplerate.a $$SUBMODULESDIR/libsndfile/src/.libs/libsndfile.a # -lsndfile -lsamplerate
mac:LIBS += -lz -framework IOKit -framework Carbon

linux:LIBS += $$SUBMODULESDIR/libsamplerate/src/.libs/libsamplerate.a $$SUBMODULESDIR/libsndfile/src/.libs/libsndfile.a # -lsndfile -lsamplerate

DEFINES    += QT_NO_CAST_ASCII MUSCLE_ENABLE_ZLIB_ENCODING

MOC_DIR     = moc

HEADERS     = AudioMoveWindow.h \
              CreateLCSDisk.h

TARGET      = AudioMove

INCLUDEPATH = .. $$SUBMODULESDIR $$SUBMODULESDIR/muscle $$SUBMODULESDIR/libsndfile/src $$SUBMODULESDIR/libsamplerate/src $(QTDIR)/include

OBJECTS_DIR = objects

HEADERS    += AudioMoveWindow.h       \
              CreateLCSDisk.h         \
              AudioMoveFileDialog.h   \ 
              AudioMovePopupMenu.h    \
              AudioMoveMessageBox.h

!mac:SOURCES += $$SUBMODULESDIR/muscle/zlib/zlib/adler32.c  \
                $$SUBMODULESDIR/muscle/zlib/zlib/deflate.c  \
                $$SUBMODULESDIR/muscle/zlib/zlib/trees.c    \
                $$SUBMODULESDIR/muscle/zlib/zlib/zutil.c    \
                $$SUBMODULESDIR/muscle/zlib/zlib/inflate.c  \
                $$SUBMODULESDIR/muscle/zlib/zlib/inftrees.c \
                $$SUBMODULESDIR/muscle/zlib/zlib/inffast.c

SOURCES    += audiomove.cpp           \
              AudioMoveWindow.cpp     \
              AudioMoveThread.cpp     \
              CreateLCSDisk.cpp       \
              LibSndFileIOThread.cpp  \
              MiscFunctions.cpp       \
              SampleRateThread.cpp    \
              AudioMoveFileDialog.cpp \
              AudioMoveMessageBox.cpp \
              AudioMovePopupMenu.cpp  \
              $$SUBMODULESDIR/muscle/dataio/FileDescriptorDataIO.cpp  \
              $$SUBMODULESDIR/muscle/message/Message.cpp              \
              $$SUBMODULESDIR/muscle/regex/StringMatcher.cpp          \
              $$SUBMODULESDIR/muscle/syslog/SysLog.cpp                \
              $$SUBMODULESDIR/muscle/system/SetupSystem.cpp           \
              $$SUBMODULESDIR/muscle/system/SystemInfo.cpp            \
              $$SUBMODULESDIR/muscle/system/SharedMemory.cpp          \
              $$SUBMODULESDIR/muscle/system/Thread.cpp                \
              $$SUBMODULESDIR/muscle/util/ByteBuffer.cpp              \
              $$SUBMODULESDIR/muscle/util/FilePathInfo.cpp            \
              $$SUBMODULESDIR/muscle/util/SocketMultiplexer.cpp       \
              $$SUBMODULESDIR/muscle/util/Directory.cpp               \
              $$SUBMODULESDIR/muscle/util/MiscUtilityFunctions.cpp    \
              $$SUBMODULESDIR/muscle/util/NetworkUtilityFunctions.cpp \
              $$SUBMODULESDIR/muscle/util/String.cpp                  \
              $$SUBMODULESDIR/muscle/zlib/ZLibCodec.cpp               \
              $$SUBMODULESDIR/muscle/zlib/ZLibUtilityFunctions.cpp


