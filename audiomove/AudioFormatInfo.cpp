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

#include "audiomove/AudioFormatInfo.h"

namespace audiomove {

bool DoesFormatSupportSampleRate(uint32 format, uint32 sampleRate)
{
   switch(format)
   {
      case AUDIO_FORMAT_MP3:
         return ((sampleRate >= AUDIO_RATE_48000)&&(sampleRate < NUM_AUDIO_RATES));

      case AUDIO_FORMAT_OGGOPUS:
         switch(sampleRate)
         {
            case AUDIO_RATE_8000:
            case AUDIO_RATE_12000:
            case AUDIO_RATE_16000:
            case AUDIO_RATE_24000:
            case AUDIO_RATE_48000:
               return true;

            default:
               return false;
         }
      break;

      default:
         // empty
      break;
   }
   return true;
}

bool DoesFormatSupportSampleWidth(uint32 format, uint32 sampleWidth)
{
   switch(format)
   {
      case AUDIO_FORMAT_CAF_ALAC:
         switch(sampleWidth)
         {
            case AUDIO_WIDTH_INT24:
            case AUDIO_WIDTH_INT16:
            case AUDIO_WIDTH_INT32:
               return true;  // ALAC also supports 20 bits per sample, but AudioMove currently doesn't so we'll ignore that

            default:
               return false;
         }
      break;

      case AUDIO_FORMAT_FLAC: case AUDIO_FORMAT_PAF_LE: case AUDIO_FORMAT_PAF_BE:
         switch(sampleWidth)
         {
            case AUDIO_WIDTH_INT24:
            case AUDIO_WIDTH_INT16:
            case AUDIO_WIDTH_INT8:
               return true;  // these are the ONLY sample-widths that FLAC and PAF support!

            default:
               return false;
         }
      break;

      default:
         // empty
      break;
   }
   return true;
}

bool DoesFormatSupportSampleWidths(uint32 fileFormat)
{
   switch(fileFormat)
   {
      case AUDIO_FORMAT_OGGVORBIS:
      case AUDIO_FORMAT_OGGOPUS:
      case AUDIO_FORMAT_MP3:
         return false;

      default:
         return true;
   }
}

bool DoesFormatSupportReadWriteMode(uint32 fileFormat)
{
   switch(fileFormat)
   {
      case AUDIO_FORMAT_FLAC:
      case AUDIO_FORMAT_CAF_ALAC:
      case AUDIO_FORMAT_OGGVORBIS:
      case AUDIO_FORMAT_OGGOPUS:
      case AUDIO_FORMAT_MP3:
         return false;  // lossy compression and read-write mode generally don't mix

      default:
         return true;
   }
}

const char * GetAudioFormatFileNameExtensions(uint32 format)
{
   switch(format)
   {
      case AUDIO_FORMAT_WAV:       return ".wav";
      case AUDIO_FORMAT_AIFF:      return ".aiff;.aif";
      case AUDIO_FORMAT_MP3:       return ".mp3";
      case AUDIO_FORMAT_FLAC:      return ".flac;.fla";
      case AUDIO_FORMAT_OGGVORBIS: return ".ogg;.oga";
      case AUDIO_FORMAT_PAF_BE:    return ".paf";
      case AUDIO_FORMAT_PAF_LE:    return ".paf";
      case AUDIO_FORMAT_WAV64:     return ".w64";
      case AUDIO_FORMAT_CAF:       return ".caf";
      case AUDIO_FORMAT_CAF_ALAC:  return ".caf";  // we're storing ALAC data in a CAF container for now (because that's what libsndfile's example code does)
      case AUDIO_FORMAT_RF64:      return ".rf64";
      case AUDIO_FORMAT_OGGOPUS:   return ".opus";
      default:                     return "";
   }
}

const char * GetAudioFormatName(uint32 format)
{
   switch(format)
   {
      case AUDIO_FORMAT_SOURCE:     return "Source";
      case AUDIO_FORMAT_WAV:        return ".WAV";
      case AUDIO_FORMAT_AIFF:       return ".AIFF";
      case AUDIO_FORMAT_MP3:        return ".MP3";
      case AUDIO_FORMAT_FLAC:       return ".FLAC";
      case AUDIO_FORMAT_OGGVORBIS:  return ".OGG";
      case AUDIO_FORMAT_PAF_BE:     return ".PAF (B.E.)";
      case AUDIO_FORMAT_PAF_LE:     return ".PAF (L.E.)";
      case AUDIO_FORMAT_WAV64:      return ".W64";
      case AUDIO_FORMAT_CAF:        return ".CAF";
      case AUDIO_FORMAT_CAF_ALAC:   return ".CAF (ALAC)";
      case AUDIO_FORMAT_RF64:       return ".RF64";
      case AUDIO_FORMAT_OGGOPUS:    return ".OPUS";
      case AUDIO_FORMAT_NORMALIZED: return "Normalized";
      default:                      return "";
   }
}

};  // end namespace audiomove
