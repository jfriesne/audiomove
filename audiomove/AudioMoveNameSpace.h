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

#ifndef AudioMoveNameSpace_h
#define AudioMoveNameSpace_h

#include <QString>
#include <QFile>

#include "support/MuscleSupport.h"

// The audiomove namespace is a superset of the muscle namespace
namespace audiomove
{
   using namespace muscle;
   using namespace Qt;

   /** Convert a C string to a QString.  All AudioMove code should use this wrapper
    *  wrapper function so that if we ever want to switch from UTF8 to another
    *  encoding, only two lines of AudioMove code will need to be changed.
    *  @param cs The UTF8 C string to import.
    *  @returns An equivalent QString.
    */
   inline QString ToQ(const char * cs) {return QString::fromUtf8(cs);}

   /** Convert a filesystem C string to a QString.  Any file names or file paths
    *  taken from the file system should be converted to QStrings using this function.
    *  (This separate function is necessary under Windows because Windows doesn't
    *  store its file names in UTF8 format -- it uses its own native encoding instead)
    *  @param cs The C string to import, in the file system's native encoding.
    *  @returns An equivalent QString.
    */
   inline QString LocalToQ(const char * cs) {return QFile::decodeName(cs);}
};

/** Convenience macro for getting the UTF8 bytes from a QString easily.
 *  Note that the bytes must be copied out from the returned pointer IMMEDIATELY,
 *  since the returned pointer will become invalid as soon as the implicitly
 *  created QByteArray object goes away.
 */
#define FromQ(qs) ((qs).toUtf8().constData())

/** Same as above, but for filesystem-native encodings */
#define LocalFromQ(qs) (QFile::encodeName(qs).constData())

#endif
