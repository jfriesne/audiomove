/*
** Copyright (C) 2021 Level Control Systems <jaf@meyersound.com>
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

#include "audiomove/AudioMoveConstants.h"

namespace audiomove {

/** Returns true iff the given audio format supports the specified sample rate
  * @param format an AUDIO_FORMAT_* value
  * @param format an AUDIO_SAMPLE_RATE_* value
  * @returns true iff the format supports the sample rate
  */
bool DoesFormatSupportSampleRate(uint32 format, uint32 sampleRate);

/** Returns true iff the given audio format supports the specified sample width
  * @param format an AUDIO_FORMAT_* value
  * @param format an AUDIO_SAMPLE_WIDTH_* value
  * @returns true iff the format supports the sample rate
  */
bool DoesFormatSupportSampleWidth(uint32 format, uint32 sampleWidth);

/** Returns true iff the given audio format supports the concept of explicit sample widths
  * (some compressed formats don't, as they use variable-length representations for audio instead)
  * @param an AUDIO_FORMAT_* value
  * @returns true iff the format supports explicit sample widths
  */
bool DoesFormatSupportSampleWidths(uint32 fileFormat);

/** Returns true iff we can open files of this format in read/write mode in order to edit/update
  * them.  Returns false for formats that only support read-only xor write-only modes.
  * @param an AUDIO_FORMAT_* value
  * @returns true iff the format supports read-write mode
  */
bool DoesFormatSupportReadWriteMode(uint32 fileFormat);

/** Returns a string containing semicolon-separated filename extensions associated
  * with the given audio format (e.g. ".aiff;.aif" for AUDIO_FORMAT_AIFF.  If none
  * are known, an empty string ("") is returned.
  * @param format an AUDIO_FORMAT_* value
  * @returns a string containing zero or more filename extension suffixes, semicolon-separated
  */
const char * GetAudioFormatFileNameExtensions(uint32 format);

/** Returns a human-readable name for the given format, or "" if none is known.
  * @param an AUDIO_FORMAT_* value
  * @returns e.g. ".WAV" for AUDIO_FORMAT_WAV
  */
const char * GetAudioFormatName(uint32 format);

};  // end namespace audiomove
