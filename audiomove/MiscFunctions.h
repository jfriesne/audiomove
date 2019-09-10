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

#include <QColor>
#include <QDir>

#include "audiomove/AudioMoveConstants.h"
#include "message/Message.h"
#include "util/String.h"

class QMouseEvent;
class QWidget;
class QBoxLayout;

namespace audiomove {

/**
  * Reads the given settings file into the given Message object.
  * @param filePath directory and file pathname for the settings file to read (Qt standard format)
  * @param settings the Message to read into
  * @return B_NO_ERROR on success, B_ERROR on failure (read error or out of memory)
  */
status_t ReadSettingsFile(const char * prefsPath, Message & settings);

/**
  * Writes the given Message object into the given settings file.
  * @param filePath directory and file pathname for the settings file to create or overwrite (Qt standard format)
  * @param settings the Message to write into the file.
  * @param compressionLevel How much to compress the file when saving it.  Zero means no compression, nine is maximum compression.
  * @return B_NO_ERROR on success, B_ERROR on failure (write error or out of memory)
  */
status_t WriteSettingsFile(const char * prefsPath, const Message & settings, int compressionLevel);

/** Returns the specified subfolder underneath the LCS_Settings folder.
 *  @param subFolderName name of the subfolder to return, or "" or null if you want the LCS_Settings folder itself.
 *  @param createIfNecessary Whether or not we should create folders if they don't currently exist.
 *  @returns a QDir object representing the desired directory, or a non-existant QDir object on failure.
 */
QDir GetSettingsFolder(const QString & subFolderName, bool createIfNecessary);

/** Save the given Message into the AudioMove portion of the registry, under the given name.
 *  @param subkey Key to save the Message under.
 *  @param msg The Message to save.
 *  @return B_NO_ERROR on success, or B_ERROR on failure.
 */
status_t SaveMessageToRegistry(const char * subkey, const Message & msg);

/** Load a given Message from the AudioMove portion of the registry, using the given name.
 *  @param subkey Key to look for the saved Message under.
 *  @param msg On success, the loaded Message will be copied into this object.
 *  @return B_NO_ERROR on success, or B_ERROR on failure (key didn't exist?).
 */
status_t LoadMessageFromRegistry(const char * subkey, Message & msg);

/** A utility function that does all the magic incantations necessary
 *  to get a dialog widget to show up reliably.  Why is it so difficult
 *  to do this right?  Nobody knows... :^P
 */
void ShowDialog(QWidget * w);

/** Saves position information about the given window into the given Message.
 *  Both the window's position on its desktop, and the desktop number are saved.
 *  @param window the top-level window to save position info for
 *  @param archive the Message to save the info into.
 *  @return B_NO_ERROR on success, or B_ERROR on failure (out of memory?).
 */
status_t SaveWindowPositionToArchive(const QWidget * window, Message & archive);

/** Restores the position of a window by reading the information out of a Message
 *  that previously had SaveWindowPositionToArchive() applied to it.
 *  @param window The top-level window to restore the position of.
 *  @param archive the Message to read the info from.
 *  @param allowResize If true, we'll resize the window as necessary.  Otherwise we'll just reposition.  Defaults to true.
 *  @return B_NO_ERROR on success, or B_ERROR if the position was not restored.
 */
status_t RestoreWindowPositionFromArchive(QWidget * window, const Message & archive, bool allowResize=true);

/** Makes sure that the folders in the specified path exist, so that the file can be created.
  * @param fileName Fully qualified file path of the file we want to create.
  * @param createIfNecessary If true, folders that do not exist will be created if possible.
  * returns B_NO_ERROR on success, or B_ERROR on failure (no write permission?)
  */
status_t EnsureFileFolderExists(const String & fileName, bool createIfNecessary);

/** Returns the path-separator char appropriate to the host operating system (e.g. "/" or "\\") */
String GetPathSepChar();

/** Platform-neutral front end to determine whether or not a given mouse-pressed event should
 *  be interpreted as a right-click event (e.g. to begin a drag or etc)
 */
bool IsRightClickEvent(QMouseEvent * event);

/** A handy drop-in replacement for the old-style QVBoxLayout ctor */
QBoxLayout * NewVBoxLayout(QWidget * parent, int margin = -1, int spacing = -1);

/** A handy drop-in replacement for the old-style QHBoxLayout ctor */
QBoxLayout * NewHBoxLayout(QWidget * parent, int margin = -1, int spacing = -1);

};  // end namespace audiomove

