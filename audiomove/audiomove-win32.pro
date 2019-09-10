CONFIG     += qt warn_on release thread static rtti link_prl

win32:DEFINES  += _MT WIN32

win32:RC_FILE  = ./audiomove.rc
mac:RC_FILE    = ./audiomove.icns

win32:LIBS += ../libsamplerate/samplerate.lib ../libsndfile/sndfile.lib

mac:LIBS += ../libsamplerate/src/.libs/libsamplerate.a ../libsndfile/src/.libs/libsndfile.a # -lsndfile -lsamplerate
mac:LIBS += -lz -framework IOKit -framework Carbon

linux:LIBS += ../libsamplerate/src/.libs/libsamplerate.a ../libsndfile/src/.libs/libsndfile.a # -lsndfile -lsamplerate

DEFINES    += QT_NO_CAST_ASCII MUSCLE_ENABLE_ZLIB_ENCODING

MOC_DIR     = moc

HEADERS     = ../audiomove/AudioMoveWindow.h \
              ../audiomove/CreateLCSDisk.h

TARGET      = AudioMove

INCLUDEPATH = . .. ../../muscle ../../libsndfile/src ../../libsamplerate/src $(QTDIR)/include

OBJECTS_DIR = objects

HEADERS    += ../audiomove/AudioMoveWindow.h       \
              ../audiomove/CreateLCSDisk.h         \
              ../audiomove/AudioMoveFileDialog.h   \ 
              ../audiomove/AudioMovePopupMenu.h    \
              ../audiomove/AudioMoveMessageBox.h

!mac:SOURCES += ../../muscle/zlib/zlib/adler32.c  \
                ../../muscle/zlib/zlib/deflate.c  \
                ../../muscle/zlib/zlib/trees.c    \
                ../../muscle/zlib/zlib/zutil.c    \
                ../../muscle/zlib/zlib/inflate.c  \
                ../../muscle/zlib/zlib/inftrees.c \
                ../../muscle/zlib/zlib/inffast.c

SOURCES    += ../audiomove/audiomove.cpp          \
              ../audiomove/AudioMoveWindow.cpp     \
              ../audiomove/AudioMoveThread.cpp     \
              ../audiomove/CreateLCSDisk.cpp       \
              ../audiomove/LibSndFileIOThread.cpp  \
              ../audiomove/MiscFunctions.cpp       \
              ../audiomove/SampleRateThread.cpp    \
              ../audiomove/AudioMoveFileDialog.cpp \
              ../audiomove/AudioMoveMessageBox.cpp \
              ../audiomove/AudioMovePopupMenu.cpp  \
              ../../muscle/dataio/FileDescriptorDataIO.cpp    \
              ../../muscle/message/Message.cpp     \
              ../../muscle/regex/StringMatcher.cpp \
              ../../muscle/syslog/SysLog.cpp       \
              ../../muscle/system/SetupSystem.cpp  \
              ../../muscle/system/SystemInfo.cpp   \
              ../../muscle/system/SharedMemory.cpp \
              ../../muscle/system/Thread.cpp       \
              ../../muscle/util/ByteBuffer.cpp     \
              ../../muscle/util/FilePathInfo.cpp   \
              ../../muscle/util/SocketMultiplexer.cpp       \
              ../../muscle/util/Directory.cpp               \
              ../../muscle/util/MiscUtilityFunctions.cpp    \
              ../../muscle/util/NetworkUtilityFunctions.cpp \
              ../../muscle/util/String.cpp                  \
              ../../muscle/zlib/ZLibCodec.cpp               \
              ../../muscle/zlib/ZLibUtilityFunctions.cpp
