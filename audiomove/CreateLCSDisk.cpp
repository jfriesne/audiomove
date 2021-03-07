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
# include "win32dirent.h"
#else
# include <dirent.h>
#endif

#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QPushButton>

#include "audiomove/CreateLCSDisk.h"
#include "audiomove/LibSndFileIOThread.h"
#include "audiomove/MiscFunctions.h"
#include "dataio/FileDataIO.h"
#include "zlib/ZLibUtilityFunctions.h"

namespace audiomove {

enum {CUEMIXER_REPLY_VIRTUALDRIVEFILE = 1903063607};  // stolen from qtnet/protocol/QNetProtocol.h

static status_t CreateVDiskFileAux(const String & node, Message & msg, CreateVDiskDialog * dlg)
{
   if (dlg->IsCancelled()) return B_ERROR("User cancelled dialog");
   qApp->processEvents();

   DIR * dir = opendir(node());
   if (dir)
   {
      // If (node) is a directory, then read through it and recurse for each sub-node
      // that doesn't start with a dot.

      struct dirent * de;
      while((de = readdir(dir)) != NULL)
      {
         const String n = de->d_name;
         if (n.StartsWith('.') == false)
         {
            dlg->SetCurrentFileName(ToQ(node.Substring("/")()));
            MessageRef subMsg = GetMessageFromPool(CUEMIXER_REPLY_VIRTUALDRIVEFILE);
            MRETURN_ON_NULL(subMsg());
            MRETURN_ON_ERROR(CreateVDiskFileAux(node+n.Prepend("/"), *subMsg(), dlg));
            if (subMsg()->GetNumNames() > 0) MRETURN_ON_ERROR(msg.AddMessage(n, subMsg));
         }
      }
      closedir(dir);
   }
   else
   {
      // If (node) is a file, then just add the file's attributes to (msg).
      LibSndFileIOThread t(node);
      if (t.OpenFile().IsOK())
      {
         const uint8 numStreams = t.GetFileStreams();
         const int64 numSamples = t.GetFileFrames();
         t.CloseFile(CLOSE_FLAG_FINAL);

         status_t ret;
         if ((msg.CAddInt8(".c", numStreams, 1).IsError(ret))||(msg.AddInt64(".s", numSamples).IsError(ret))) return ret;
      }
   }
   return B_NO_ERROR;
}

CreateVDiskDialog :: CreateVDiskDialog(QWidget * parent, WindowFlags f) : QWidget(parent, f), _cancelled(false), _lastTime(0)
{
   setWindowTitle(tr("Virtual Drive File Creation"));

   QBoxLayout * vLayout = NewVBoxLayout(this, 5, 5);

   vLayout->addSpacing(10);

   _status = new QLabel(this);
   _status->setAlignment(AlignCenter);
   _status->setMinimumWidth(400);
   vLayout->addWidget(_status);

   QWidget * row2 = new QWidget(this);
   vLayout->addWidget(row2);

   QBoxLayout * hLayout = NewHBoxLayout(row2);
   hLayout->addStretch(10);

   QPushButton * cancel = new QPushButton(tr("Cancel"), row2);
   connect(cancel, SIGNAL(clicked()), this, SLOT(Cancel()));
   hLayout->addWidget(cancel);
}

void CreateVDiskDialog :: SetCurrentFileName(const QString & s) 
{
   if (OnceEvery(100000, _lastTime)) _status->setText(tr("Scanning: ") + s);
}

status_t CreateVDiskFile(const char * targetDisk, const char * targetFile, QWidget * parent)
{
   MessageRef msg = GetMessageFromPool(CUEMIXER_REPLY_VIRTUALDRIVEFILE);
   MRETURN_ON_NULL(msg());

   CreateVDiskDialog * dlg = new CreateVDiskDialog(parent);
   dlg->show();

   status_t ret;
   if (CreateVDiskFileAux(targetDisk, *msg(), dlg).IsError(ret)) 
   {
      bool wasCancelled = dlg->IsCancelled();
      delete dlg;

      if (wasCancelled) return B_NO_ERROR;  // avoid error dialog in this case
      else
      {
         printf("Error creating .lcsDisk Message! [%s]\n", ret());
         return ret;
      }
   }

   delete dlg;
  
   MessageRef compressedMsg = DeflateMessage(msg, 9);
   if (compressedMsg()) 
   {
      printf("Error compressing .lcsDisk Message!\n");
      return B_ZLIB_ERROR;
   }

   FILE * fpOut = fopen(targetFile, "w");
   if (fpOut == NULL)
   {
      printf("Error, couldn't open output file [%s]\n", targetFile);
      return B_IO_ERROR;
   }

   FileDataIO dio(fpOut);
   if (compressedMsg()->FlattenToDataIO(dio, false).IsError(ret))
   {
      printf("Error writing .lcsDisk data to output file [%s] [%s]\n", targetFile, ret());
      return ret;
   }
 
   return B_NO_ERROR;
}

}; // end namespace audiomove
