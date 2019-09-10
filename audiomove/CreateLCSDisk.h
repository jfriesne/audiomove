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

#include <QWidget>
#include <QLabel>

#include "audiomove/AudioMoveNameSpace.h"

class QLabel;

namespace audiomove {

/** Creates a .lcsDisk file from the specified directory.
  * @param targetDisk The path to the directory to do the archiving from
  * @param targetFile the file to write our output to.
  * @param parent QWidget who should be the parent of the progress dialog
  * @returns B_NO_ERROR on success, or B_ERROR on failure.
  */
status_t CreateVDiskFile(const char * targetDisk, const char * targetFile, QWidget * parent);

/** This is the progress dialog that the CreateVDiskFile() function displays.  It is only
  * declared here for Qt's benefit:  you shouldn't use this class directly.
  */
class CreateVDiskDialog : public QWidget
{
Q_OBJECT
public:
   CreateVDiskDialog(QWidget * parent, WindowFlags f = Dialog);

   void SetCurrentFileName(const QString & s);
   bool IsCancelled() const {return _cancelled;}

public slots:
   void Cancel() {_cancelled = true;}

private:
   QLabel * _status;
   bool _cancelled;
   uint64 _lastTime;
};

}; // end namespace audiomove
