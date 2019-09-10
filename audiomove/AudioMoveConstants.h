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

#ifndef AudioMoveConstants_h
#define AudioMoveConstants_h

#include "audiomove/AudioMoveNameSpace.h"

namespace audiomove {

#define VERSION_STRING "1.21"

enum {
   AUDIO_FORMAT_SOURCE = 0,  // use same format as source file
   AUDIO_FORMAT_WAV,         // convert to WAV format
   AUDIO_FORMAT_AIFF,        // convert to AIFF format
   AUDIO_FORMAT_FLAC,        // convert to FLAC format
   AUDIO_FORMAT_OGGVORBIS,   // convert to Ogg Vorbis format
   AUDIO_FORMAT_PAF_BE,      // convert to big-endian PAF format
   AUDIO_FORMAT_PAF_LE,      // convert to little-endian PAF format
   AUDIO_FORMAT_WAV64,       // convert to WAV64 format
   AUDIO_FORMAT_CAF,         // convert to CAF format
   AUDIO_FORMAT_RF64,        // convert to RF64 format
   AUDIO_FORMAT_OGGOPUS,     // convert to Ogg Opus format
   AUDIO_FORMAT_NORMALIZED,  // normalized big-endian floats
   NUM_AUDIO_FORMATS         // guard value
};

enum {
   AUDIO_RATE_SOURCE = 0,    // use same rate as source file
   AUDIO_RATE_192000,        // 192kHz
   AUDIO_RATE_176400,        // 176.4kHz
   AUDIO_RATE_96000,         // 96kHz
   AUDIO_RATE_88200,         // 88.2kHz
   AUDIO_RATE_48000,         // 48kHz
   AUDIO_RATE_44100,         // 44.1kHz
   AUDIO_RATE_24000,         // 24kHz
   AUDIO_RATE_22050,         // 22.05kHz
   AUDIO_RATE_16000,         // 16kHz
   AUDIO_RATE_12000,         // 12kHz
   AUDIO_RATE_11025,         // 11.025kHz
   AUDIO_RATE_8000,          // 8kHz
   NUM_AUDIO_RATES           // guard value
};

enum {
   AUDIO_WIDTH_SOURCE = 0,   // use same width as source file
   AUDIO_WIDTH_FLOAT,        // 32-bit floating point samples
   AUDIO_WIDTH_INT32,        // 32-bit integer samples
   AUDIO_WIDTH_INT24,        // 24-bit integer samples
   AUDIO_WIDTH_INT16,        // 16-bit integer samples
   AUDIO_WIDTH_INT8,         // 8-bit integer samples
   AUDIO_WIDTH_DOUBLE,       // 64-bit floating point samples
   NUM_AUDIO_WIDTHS          // guard value
};

enum {
   AUDIO_CONVERSION_QUALITY_BEST = 0, // take your time, make it sound good
   AUDIO_CONVERSION_QUALITY_BETTER,   // still sound good but go faster
   AUDIO_CONVERSION_QUALITY_GOOD,     // go fast, quality is mediocre
   NUM_AUDIO_CONVERSION_QUALITIES,

   DEFAULT_AUDIO_CONVERSION_QUALITY = AUDIO_CONVERSION_QUALITY_GOOD  // SteveE says it's good enough and it's faster
};

/** Given an AUDIO_RATE_* code, returns the literal sample rate (in audioframes/second) for that code (e.g. AUDIO_RATE_48000 -> 48000).  If (rateCode) is unknown, returns (rateCode) (since it may be a literal value in Hz). */
extern uint32 GetSampleRateValueFromCode(uint32 rateCode);

/** Given a sample rate (e.g. 48000), returns the corresponding AUDIO_RATE_* code, or NUM_AUDIO_RATES if there is no corresponding code. */
extern uint32 GetSampleRateCodeFromValue(uint32 value);

enum {
   AUDIO_STREAMS_SOURCE = 0  // magic value, indicates that we should be reading
};

enum {
   MAX_SIMULTANEOUS_PROCESSES = 9  // more than this is just silly
};

#define AUDIOMOVE_NAME_BUF      "buf"  // ByteBuffers are stored in our processing messages here
#define AUDIOMOVE_NAME_STATUS   "stat" // status string sent back by worker threads when necessary
#define AUDIOMOVE_NAME_ISLAST   "last" // boolean field; present if this is the last buffer in the stream
#define AUDIOMOVE_NAME_ORIGSIZE "osz"  // int32 field:  number of bytes originally present in Message
#define AUDIOMOVE_NAME_OPENFILE "opn"  // bool field:  if present, tells the threads to open their files now

};  // end namespace audiomove

#endif
