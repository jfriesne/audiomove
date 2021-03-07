/*
** Copyright (C) 2004 Level Control Systems <jaf@lcsaudio.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
*/

#if defined(WIN32)
# include <winsock2.h>
# include "win32dirent.h"
#else
# include <dirent.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <QApplication>
#include <QBoxLayout>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QObject>
#include <QComboBox>
#include <QDesktopWidget>
#include <QAbstractButton>
#include <QPainter>
#include <QPixmap>
#include <QMenu>
#include <QMouseEvent>

#include "message/Message.h"
#include "util/ByteBuffer.h"
#include "util/NetworkUtilityFunctions.h"
#include "system/SharedMemory.h"
#include "util/StringTokenizer.h"
#include "util/MiscUtilityFunctions.h"
#include "zlib/ZLibCodec.h"
#include "audiomove/MiscFunctions.h"

// Miscellaneous functions used by AudioMove... 
namespace audiomove {

status_t ReadSettingsFile(const char * prefsPath, Message & settings)
{
   QFile file(LocalToQ(prefsPath));
   if (file.open(QIODevice::ReadOnly))
   {
      QByteArray array = file.readAll();
      if (array.data())
      {
         ByteBuffer buf; buf.AdoptBuffer(array.size(), (uint8 *)array.data());
         ByteBufferRef bufRef(&buf, false);

#ifdef MUSCLE_ENABLE_ZLIB_ENCODING
         ZLibCodec inflater;
         ByteBufferRef decompressedRef = inflater.Inflate(*bufRef());
         if (decompressedRef()) bufRef = decompressedRef;
#endif

         status_t ret = (bufRef()) ? settings.Unflatten(bufRef()->GetBuffer(), bufRef()->GetNumBytes()) : B_ERROR;
         (void) buf.ReleaseBuffer();  // the QByteArray class will free it so we need to not free it ourself
         return ret;
      }
   }
   return B_IO_ERROR;
}

status_t WriteSettingsFile(const char * prefsPath, const Message & settings, int compressionLevel)
{
   ByteBufferRef bufRef = GetByteBufferFromPool(settings.FlattenedSize());
   if (bufRef())
   {
      settings.Flatten(bufRef()->GetBuffer());
      if (compressionLevel > 0)
      {
#ifdef MUSCLE_ENABLE_ZLIB_ENCODING
         ZLibCodec deflater(compressionLevel);
         ByteBufferRef compressedRef = deflater.Deflate(*bufRef(), true);
         if (compressedRef()) bufRef = compressedRef;
#endif
      }
      if (bufRef())
      {
         QFile file(LocalToQ(prefsPath));
         if ((file.open(QIODevice::WriteOnly))&&(file.write((const char *)bufRef()->GetBuffer(), bufRef()->GetNumBytes()) == (int32)bufRef()->GetNumBytes())) return B_NO_ERROR;
      }
   }
   return B_IO_ERROR;
}

QDir GetSettingsFolder(const QString & subFolderName, bool createIfNecessary)
{
   const char * SETTINGS_FOLDER_NAME = ".audiomove_settings";

   QDir dir = QDir::home();
   if (dir.exists())
   {
      if (createIfNecessary) (void) dir.mkdir(LocalToQ(SETTINGS_FOLDER_NAME));
      dir = QDir(dir.absolutePath() + LocalToQ("/") + LocalToQ(SETTINGS_FOLDER_NAME));

      if (subFolderName.length() > 0)
      {
         if (createIfNecessary) (void) dir.mkdir(subFolderName);
         dir = QDir(dir.absolutePath() + LocalToQ("/") + subFolderName);
      }
   }
   return dir;
}

static const char * REGISTRY_FILE_PREFIX = "key_";
static const char * REGISTRY_DIR_NAME    = "LCS_Registry";

status_t SaveMessageToRegistry(const char * subkey, const Message & msg)
{
   SharedMemory memLock;  // used as a read/write lock to ensure the file isn't corrupted
   String temp(REGISTRY_FILE_PREFIX); temp += subkey;
   if ((memLock.SetArea(REGISTRY_DIR_NAME, 1).IsOK())&&(memLock.LockAreaReadWrite().IsOK()))
   {
      QDir dir = GetSettingsFolder(LocalToQ(REGISTRY_DIR_NAME), true);
      if (dir.exists())
      {
         String path = LocalFromQ(dir.absolutePath()); path += "/"; path += temp;
         return WriteSettingsFile(path(), msg, 0);  // don't worry, SharedMemory dtor unlocks the area for us
      }
   }
   return B_IO_ERROR;
}

status_t LoadMessageFromRegistry(const char * subkey, Message & msg)
{
   SharedMemory memLock;  // used as a read/write lock to ensure the file isn't corrupted
   String temp(REGISTRY_FILE_PREFIX); temp += subkey;
   if ((memLock.SetArea(REGISTRY_DIR_NAME, 1).IsOK())&&(memLock.LockAreaReadOnly().IsOK()))
   {
      QDir dir = GetSettingsFolder(LocalToQ(REGISTRY_DIR_NAME), false);
      if (dir.exists())
      {
         String path = LocalFromQ(dir.absolutePath()); path += "/"; path += temp;
         return ReadSettingsFile(path(), msg);  // don't worry, SharedMemory dtor unlocks the area for us
      }
   }
   return B_IO_ERROR;
}

void ShowDialog(QWidget * w)
{
   if (w->isHidden()) w->show();
   w->raise();
   if (w->isMinimized())
   {
      // Why is all this rigamarole necessary?  :^P
      w->hide();
      w->showNormal();
   }
}

status_t SaveWindowPositionToArchive(const QWidget * window, Message & archive)
{
   QDesktopWidget * desktop = QApplication::desktop();
   if ((desktop)&&(desktop->isVirtualDesktop() == false)) MRETURN_ON_ERROR(archive.AddInt32("screen", desktop->screenNumber((QWidget *)window)));

   // see file:/usr/local/qt/doc/html/geometry.html (at the bottom)
   QPoint p = window->pos();
   QSize  s = window->size();
   return archive.AddRect("geometry", Rect(p.x(), p.y(), p.x()+s.width(), p.y()+s.height()));
}

status_t RestoreWindowPositionFromArchive(QWidget * window, const Message & archive, bool allowResize)
{
   QDesktopWidget * desktop = QApplication::desktop();

   Rect r;
   MRETURN_ON_ERROR(archive.FindRect("geometry", r));

   QRect qr(muscleRintf(r.left()), muscleRintf(r.top()), muscleRintf(r.Width()), muscleRintf(r.Height()));
   qr.setWidth( muscleClamp(qr.width(),  window->minimumWidth(),  window->maximumWidth()));
   qr.setHeight(muscleClamp(qr.height(), window->minimumHeight(), window->maximumHeight()));

   bool onScreen = false;
   {
      int numScreens = desktop->numScreens();
      for (int i=0; i<numScreens; i++)
      {
         // Make sure we are at least partially visible on the screen!
         QRect screenRect = desktop->availableGeometry(i);
         if (qr.intersects(screenRect))
         {
            onScreen = true;
            break;
         }
      }
   }

   // If it's not visible anywhere, move it to the center of the primary screen.
   // and the user can decide where to put it.
   if (onScreen == false) qr.moveCenter(desktop->availableGeometry().center());
   if (allowResize) window->resize(QSize(qr.width(), qr.height()));
   window->move(qr.left(), qr.top());
   return B_NO_ERROR;
}

status_t EnsureFileFolderExists(const String & fileName, bool createIfNecessary)
{
#if defined(WIN32)
   String path;
#else
   String path = GetPathSepChar();
#endif

   StringTokenizer tok(fileName(), GetPathSepChar()());
   const char * next;
   while((next = tok()) != NULL)
   {
      if (tok.GetRemainderOfString())  // don't do this for the file name itself!
      {
         path += next;
#if defined(WIN32)
         if ((GetFileAttributesA(path()) == INVALID_FILE_ATTRIBUTES)&&((createIfNecessary == false)||(CreateDirectoryA(path(), 0) == FALSE))) return B_IO_ERROR;
#else
         DIR * dir = opendir(path());
         if (dir) closedir(dir);
         else 
         {
            if (createIfNecessary == false) return B_ACCESS_DENIED;
            (void) mkdir(path(), S_IRWXU|S_IRWXG|S_IRWXO);
         }
#endif
         path += GetPathSepChar();
      }
   }
   return B_NO_ERROR;
}

String GetPathSepChar()
{
   static const String PATH_SEP_CHAR = "/";
   return PATH_SEP_CHAR;
}

bool IsRightClickEvent(QMouseEvent * evt)
{
   bool ret = (evt->button() == RightButton);
#ifdef __APPLE__
   if (evt->modifiers() & MetaModifier) ret = true;
#endif
   return ret;
}

static void DoBoxCustomization(QBoxLayout * bl, int margin, int spacing)
{
   if (margin >= 0)
   {
      bl->setMargin(margin);
      bl->setSpacing((spacing>=0)?spacing:margin);
   }
   else if (spacing >= 0) bl->setSpacing(spacing);
   else
   {
      bl->setSpacing(0);
      bl->setMargin(0);
   }
}

QBoxLayout * NewVBoxLayout(QWidget * parent, int margin, int spacing)
{
   QBoxLayout * vbl = new QBoxLayout(QBoxLayout::TopToBottom, parent);
   DoBoxCustomization(vbl, margin, spacing);
   return vbl;
}

QBoxLayout * NewHBoxLayout(QWidget * parent, int margin, int spacing)
{
   QBoxLayout * hbl = new QBoxLayout(QBoxLayout::LeftToRight, parent);
   DoBoxCustomization(hbl, margin, spacing);
   return hbl;
}

};  // end namespace audiomove
