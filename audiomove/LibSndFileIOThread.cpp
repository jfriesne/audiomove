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

#include <fcntl.h>
#include "audiomove/AudioFormatInfo.h"   // for DoesFormatSupportSampleWidths()
#include "audiomove/LibSndFileIOThread.h"
#include "audiomove/MiscFunctions.h"

#define DEFAULT_ANTICLIP_MAXIMUM_SAMPLE_VALUE (0.99f)

#if !defined(fmaxf) && defined(_MSC_VER)
# define fmaxf muscleMax
#endif

namespace audiomove {

static const String AUDIOMOVE_TEMP_SUFFIX = ".audiomove_temp";

LibSndFileIOThread :: LibSndFileIOThread(const String & readPath) : _fileName(readPath), _numWriteStreams(AUDIO_STREAMS_SOURCE), _splitFiles(false), _inputFileFormat(AUDIO_FORMAT_SOURCE), _outputFileFormat(AUDIO_FORMAT_NORMALIZED), _fileSampleRate(AUDIO_RATE_SOURCE), _fileSampleWidth(AUDIO_WIDTH_SOURCE), _numFrames(0), _numStreams(0), _isComplete(false), _currentMaxOutputSample(1.0f/DEFAULT_ANTICLIP_MAXIMUM_SAMPLE_VALUE), _biValid(false)
{
   memset(&_bi, 0, sizeof(_bi));
}

LibSndFileIOThread :: LibSndFileIOThread(const String & checkPath, const String & writePath, const String & optInPlaceBasePath, uint32 writeFormat, uint32 writeSampleWidth, uint32 writeSampleRate, uint8 writeNumStreams, bool splitFiles, const SF_BROADCAST_INFO * optBI) : _checkPath(checkPath), _fileName(writePath), _optInPlaceBasePath(optInPlaceBasePath), _numWriteStreams(writeNumStreams), _splitFiles(splitFiles), _inputFileFormat(AUDIO_FORMAT_NORMALIZED), _outputFileFormat(writeFormat), _fileSampleRate(writeSampleRate), _fileSampleWidth(writeSampleWidth), _numFrames(0), _numStreams(writeNumStreams), _isComplete(false), _currentMaxOutputSample(1.0f/DEFAULT_ANTICLIP_MAXIMUM_SAMPLE_VALUE), _biValid(optBI != NULL)
{
   if (_biValid) memcpy(&_bi, optBI, sizeof(_bi));
            else memset(&_bi, 0,     sizeof(_bi));
}

LibSndFileIOThread :: ~LibSndFileIOThread()
{
   DoCloseFiles(CLOSE_FLAG_FINAL|(_isComplete?0:CLOSE_FLAG_ERROR));
   DeleteTempFiles();
}

void LibSndFileIOThread :: DeleteTempFiles()
{
   for (HashtableIterator<String, bool> iter(_tempFiles, HTIT_FLAG_NOREGISTER); iter.HasData(); iter++) (void) unlink(iter.GetKey()());
   _tempFiles.Clear();
}

status_t LibSndFileIOThread :: OpenFile()
{
   CloseFile(CLOSE_FLAG_FINAL);  // paranoia

   status_t ret;
   SF_INFO info; memset(&info, 0, sizeof(info));
   if (_numWriteStreams != AUDIO_STREAMS_SOURCE)
   {
      // Write mode: If we're going to create the file, then fill in our desired parameters for the file here
      info.samplerate = _fileSampleRate;
      info.channels   = _numWriteStreams;

      switch(_outputFileFormat)
      {
         // FogBugz #3579:  don't use big-endian for WAV: the rest of the world can't handle it yet!
         case AUDIO_FORMAT_WAV:  info.format = SF_FORMAT_WAV  | SF_ENDIAN_LITTLE;         break;
         case AUDIO_FORMAT_AIFF: info.format = SF_FORMAT_AIFF | SF_ENDIAN_BIG;            break;
         case AUDIO_FORMAT_MP3:  info.format = SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_III; break;
         case AUDIO_FORMAT_FLAC: info.format = SF_FORMAT_FLAC;                            break;

         case AUDIO_FORMAT_CAF_ALAC:
         {
            switch(_fileSampleWidth)
            {
               case AUDIO_WIDTH_INT16: info.format = SF_FORMAT_CAF | SF_FORMAT_ALAC_16; break;
               // 20-bit support could go here via AUDIO_WIDTH_INT20, but it's not implemented for now
               case AUDIO_WIDTH_INT24: info.format = SF_FORMAT_CAF | SF_FORMAT_ALAC_24; break;
               case AUDIO_WIDTH_INT32: info.format = SF_FORMAT_CAF | SF_FORMAT_ALAC_32; break;
            }
         }
         break;

         case AUDIO_FORMAT_OGGVORBIS:
            info.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
         break;

         case AUDIO_FORMAT_PAF_BE: case AUDIO_FORMAT_PAF_LE:
            info.format = SF_FORMAT_PAF | ((_outputFileFormat == AUDIO_FORMAT_PAF_BE) ? SF_ENDIAN_BIG : SF_ENDIAN_LITTLE);
         break;

         case AUDIO_FORMAT_WAV64:    info.format = SF_FORMAT_W64  | SF_ENDIAN_LITTLE; break;
         case AUDIO_FORMAT_CAF:      info.format = SF_FORMAT_CAF  | SF_ENDIAN_BIG;    break;
         case AUDIO_FORMAT_RF64:     info.format = SF_FORMAT_RF64 | SF_ENDIAN_LITTLE; break;
         case AUDIO_FORMAT_OGGOPUS:  info.format = SF_FORMAT_OGG  | SF_FORMAT_OPUS;   break;

         default:
            return B_ERROR("Unsupported Output file format");  // we only support the above-listed audio formats for output... for now anyway
      }

      if ((DoesFormatSupportSampleWidths(_outputFileFormat))&&(_outputFileFormat != AUDIO_FORMAT_CAF_ALAC)&&(_outputFileFormat != AUDIO_FORMAT_MP3))  // CAF_ALAC and MP3 already set their subtypes above
      {
         switch(_fileSampleWidth)
         {
            case AUDIO_WIDTH_DOUBLE: info.format |= SF_FORMAT_DOUBLE; break;
            case AUDIO_WIDTH_FLOAT:  info.format |= SF_FORMAT_FLOAT;  break;
            case AUDIO_WIDTH_INT32:  info.format |= SF_FORMAT_PCM_32; break;
            case AUDIO_WIDTH_INT24:  info.format |= SF_FORMAT_PCM_24; break;
            case AUDIO_WIDTH_INT16:  info.format |= SF_FORMAT_PCM_16; break;
            case AUDIO_WIDTH_INT8:
               switch(_outputFileFormat)
               {
                  case AUDIO_FORMAT_WAV: case AUDIO_FORMAT_WAV64: case AUDIO_FORMAT_RF64: // these format support only unsigned-8-bit, not signed-8-bit
                     info.format |= SF_FORMAT_PCM_U8;
                  break;

                  default:
                     info.format |= SF_FORMAT_PCM_S8;
                  break;
               }
            break;

            default:
               info.format |= SF_FORMAT_FLOAT;
            break;
         }
      }

      MRETURN_ON_ERROR(EnsureFileFolderExists(_checkPath, false));  // mount-point fix for v1.11
      (void) EnsureFileFolderExists(_fileName, true);  // if the base directory exists, then it's okay to put sub-dirs under it

      if ((_splitFiles)&&(_numWriteStreams > 1))
      {
          // New for AudioMove v1.15:  this mode writes each channel of audio to its own separate output file
          info.channels = 1;
          int lastDot = _fileName.LastIndexOf('.');
          for (uint32 i=0; i<_numWriteStreams; i++)
          {
             String fileName = (lastDot>=0) ? (_fileName.Substring(0, lastDot)+String("_c%1").Arg(i+1)+_fileName.Substring(lastDot)) : (_fileName+String("_c%1").Arg(i+1));
             SNDFILE * file = NULL;
             const status_t oRet = OpenFileForWriting(fileName, info, file);  // don't merge into the PutOutputFile() call!  This may change (fileName)!
             ret |= oRet;
             if ((oRet.IsOK())&&(PutOutputFile(fileName, file).IsError(ret))) break;
          }
          if (_files.GetNumItems() == _numWriteStreams)
          {
             _numFrames = 0;  // no data written, yet!
             return ret;
          }
      }
      else
      {
         String fn = _fileName;
         SNDFILE * file = NULL;
         const status_t oRet = OpenFileForWriting(fn, info, file);  // don't merge into the PutOutputFile() call!  This may change (fn)!
         ret |= oRet;
         if ((oRet.IsOK())&&(PutOutputFile(fn, file).IsOK(ret)))
         {
            _numFrames = 0;  // no data written, yet!
            return ret;
         }
      }
   }
   else
   {
      SNDFILE * file = sf_open(_fileName(), SFM_READ, &info);
      if (file)
      {
         if (_files.Put(_fileName, file).IsOK())
         {
            _fileSampleRate  = info.samplerate;
            _fileSampleWidth = NUM_AUDIO_WIDTHS;  // default (will hopefully be set better below)
            _numFrames       = info.frames;
            _numStreams      = info.channels;

            // Read mode:  extract attributes from libsndfile
            switch(info.format & SF_FORMAT_TYPEMASK)
            {
               case SF_FORMAT_WAV:  _inputFileFormat = AUDIO_FORMAT_WAV;   break;
               case SF_FORMAT_AIFF: _inputFileFormat = AUDIO_FORMAT_AIFF;  break;
               case SF_FORMAT_MPEG: _inputFileFormat = AUDIO_FORMAT_MP3;   break;
               case SF_FORMAT_FLAC: _inputFileFormat = AUDIO_FORMAT_FLAC;  break;
               case SF_FORMAT_PAF:  _inputFileFormat = ((info.format & SF_FORMAT_ENDMASK) == SF_ENDIAN_BIG) ? AUDIO_FORMAT_PAF_BE : AUDIO_FORMAT_PAF_LE; break;
               case SF_FORMAT_W64:  _inputFileFormat = AUDIO_FORMAT_WAV64; break;
               case SF_FORMAT_CAF:  _inputFileFormat = AUDIO_FORMAT_CAF;   break;
               case SF_FORMAT_RF64: _inputFileFormat = AUDIO_FORMAT_RF64;  break;
               default:             _inputFileFormat = NUM_AUDIO_FORMATS;  break; // must be some other format that libsndfile knows, but we don't!
            }

            switch(info.format & SF_FORMAT_SUBMASK)
            {
               case SF_FORMAT_DOUBLE: _fileSampleWidth = AUDIO_WIDTH_DOUBLE; break;
               case SF_FORMAT_FLOAT:  _fileSampleWidth = AUDIO_WIDTH_FLOAT;  break;
               case SF_FORMAT_PCM_32: _fileSampleWidth = AUDIO_WIDTH_INT32;  break;
               case SF_FORMAT_PCM_24: _fileSampleWidth = AUDIO_WIDTH_INT24;  break;
               case SF_FORMAT_PCM_16: _fileSampleWidth = AUDIO_WIDTH_INT16;  break;

               case SF_FORMAT_PCM_S8: case SF_FORMAT_PCM_U8:
                  _fileSampleWidth = AUDIO_WIDTH_INT8;
               break;

               case SF_FORMAT_OPUS:    _inputFileFormat = AUDIO_FORMAT_OGGOPUS;   break;
               case SF_FORMAT_VORBIS:  _inputFileFormat = AUDIO_FORMAT_OGGVORBIS; break;

               case SF_FORMAT_ALAC_16: case SF_FORMAT_ALAC_20: case SF_FORMAT_ALAC_24: case SF_FORMAT_ALAC_32:
               {
                  if (_inputFileFormat == AUDIO_FORMAT_CAF) _inputFileFormat = AUDIO_FORMAT_CAF_ALAC;
                  switch(info.format & SF_FORMAT_SUBMASK)
                  {
                     case SF_FORMAT_ALAC_16: _fileSampleWidth = AUDIO_WIDTH_INT16;     break;
                     case SF_FORMAT_ALAC_20: /*_fileSampleWidth = AUDIO_WIDTH_INT20;*/ break;  // AUDIO_WIDTH_INT20 not implemented for now
                     case SF_FORMAT_ALAC_24: _fileSampleWidth = AUDIO_WIDTH_INT24;     break;
                     case SF_FORMAT_ALAC_32: _fileSampleWidth = AUDIO_WIDTH_INT32;     break;
                  }
               }
               break;

               default:
                  // empty
               break;
            }
            return ret;
         }
         else sf_close(file);  // don't remove it, it was only opened for reading!
      }
   }
   CloseFile(CLOSE_FLAG_ERROR);

   return ret | B_ERROR;
}

status_t LibSndFileIOThread :: PutOutputFile(const String & fn, SNDFILE * file)
{
   if (file)
   {
      if (_files.Put(fn, file).IsOK())
      {
         if (_tempFiles.Put(fn, file).IsOK()) return B_NO_ERROR;  // success!
         (void) _files.Remove(fn);  // roll back
      }
      sf_close(file);
      unlink(fn());
   }
   return B_ERROR;
}

status_t LibSndFileIOThread :: GetBroadcastInfo(SF_BROADCAST_INFO & bi)
{
   // Try to get the broadcast-info from the file... this will only work with certain WAV files, of course
   if ((_files.HasItems())&&(sf_command(*_files.GetFirstValue(), SFC_GET_BROADCAST_INFO, &bi, sizeof(bi)) == SF_TRUE)) return B_NO_ERROR;
   else
   {
      memset(&bi, 0, sizeof(bi));
      return B_ERROR;
   }
}

void LibSndFileIOThread :: CloseFile(uint32 closeFlags)
{
   DoCloseFiles(closeFlags);
}

void LibSndFileIOThread :: RescaleAudioBuffer(float * buf, uint32 numFloats, float scaleBy) const
{
   if (scaleBy != 1.0f)
   {
      /* Do the bulk of the rescaling in an 8-way unrolled loop, for extra efficiency */
      uint32 leftover = numFloats&0x07;
      for (int32 i=(numFloats>>3)-1; i>=0; --i)
      {
         buf[0] *= scaleBy; buf[1] *= scaleBy; buf[2] *= scaleBy; buf[3] *= scaleBy;
         buf[4] *= scaleBy; buf[5] *= scaleBy; buf[6] *= scaleBy; buf[7] *= scaleBy;
         buf += 8;
      }

      /* And handle any leftover samples individually */
      while(leftover--) *buf++ *= scaleBy;
   }
}

status_t LibSndFileIOThread :: RescaleAudioSegment(uint64 startOffset, uint64 numFrames, float scaleBy, float * tempBuf, uint32 tempBufSize)
{
   if (scaleBy != 1.0f)  // unlikely, but why not check?
   {
      uint64 tempSizeFrames = tempBufSize/_numStreams;
      while(numFrames > 0)
      {
         const uint32 numFramesToRead = muscleMin(numFrames, tempSizeFrames);
         const uint32 numSamples      = numFramesToRead*_numStreams;

         MRETURN_ON_ERROR(DoSeekFiles(startOffset));
         MRETURN_ON_ERROR(DoReadFromFiles(tempBuf, numSamples));

         RescaleAudioBuffer(tempBuf, numSamples, scaleBy);

         MRETURN_ON_ERROR(DoSeekFiles(startOffset));
         MRETURN_ON_ERROR(DoWriteToFiles(tempBuf, numSamples));

         startOffset += numFramesToRead;
         numFrames   -= numFramesToRead;
      }
   }
   return B_NO_ERROR;
}

bool LibSndFileIOThread :: IsOkayToRescale() const
{
   if (DoesFormatSupportReadWriteMode(_outputFileFormat) == false) return false;

   // No need to rescale if our samples are floating point, since they can't clip anyway
   if ((_fileSampleWidth == AUDIO_WIDTH_FLOAT)||(_fileSampleWidth == AUDIO_WIDTH_DOUBLE)) return false;

   // Avoid stupid bug in PAF implementation of libsndfile where seek isn't supported in SFM_RDWR mode
   if (((_outputFileFormat == AUDIO_FORMAT_PAF_BE)||(_outputFileFormat == AUDIO_FORMAT_PAF_LE))&&(_fileSampleWidth == AUDIO_WIDTH_INT24)) return false;

   return true;
}

ByteBufferRef LibSndFileIOThread :: ProcessBuffer(const ByteBufferRef & buf, QString & retErrStr, bool isLastBuffer)
{
   if ((_files.HasItems())&&(buf()))
   {
      const uint32 numBytes = buf()->GetNumBytes();

      if ((numBytes % sizeof(float)) == 0)
      {
         float * samples = (float *) buf()->GetBuffer();
         const uint32 numSamples = numBytes/sizeof(float);

         if (_numWriteStreams == AUDIO_STREAMS_SOURCE)
         {
            // Read from the input file into the buffer
            SNDFILE * file = *_files.GetFirstValue();
            int32 readSamples = sf_read_float(file, samples, numSamples);
            if (sf_error(file) == SF_ERR_NO_ERROR)
            {
               MRETURN_ON_ERROR(buf()->SetNumBytes(readSamples*sizeof(float), true));
               if (isLastBuffer)
               {
                  _isComplete = true;
                  CloseFile(CLOSE_FLAG_FINAL);  // close file now so that it isn't locked
               }
               return buf;
            }
            else retErrStr = qApp->translate("LibSndFileIOThread", "Problem reading from file");
         }
         else
         {
            if (IsOkayToRescale())
            {
               // Gotta check the output for samples outside the allowed [-1.0f,1.0f] range!  If
               // we find any, we'll need to handle it by rescaling the audio; otherwise there will
               // be clipping in the output, which would be a bad thing
               float maxSampleValue = 0.0f;
               for (uint32 i=0; i<numSamples; i++) maxSampleValue = fmaxf(maxSampleValue, fabsf(samples[i]));
               if (maxSampleValue/_currentMaxOutputSample > 1.0f)
               {
                  // Whoops!  We need to rescale everything down a bit, or we'll suffer from clipping!
                  _currentMaxOutputSample = maxSampleValue;
                  MRETURN_ON_ERROR(_maxSamplesRecord.Put(_numFrames, _currentMaxOutputSample));
               }
               RescaleAudioBuffer(samples, numSamples, 1.0f/_currentMaxOutputSample);
            }

            if (DoWriteToFiles(samples, numSamples).IsOK())
            {
               _numFrames += (numSamples/_numStreams);
               if (isLastBuffer)
               {
                  if (_currentMaxOutputSample != DEFAULT_ANTICLIP_MAXIMUM_SAMPLE_VALUE)
                  {
                     static const uint32 TEMP_BUF_SIZE = 128*1024;
                     float * tempBuf = new float[TEMP_BUF_SIZE];
                     float finalScaling = 1.0f/_currentMaxOutputSample;
                     int64 prevOffset  = 0;
                     float prevMaxSample = DEFAULT_ANTICLIP_MAXIMUM_SAMPLE_VALUE;
                     for (HashtableIterator<int64, float> iter(_maxSamplesRecord); iter.HasData(); iter++)
                     {
                        int64 nextOffset    = iter.GetKey();
                        float nextMaxSample = iter.GetValue();
                        if (RescaleAudioSegment(prevOffset, nextOffset-prevOffset, finalScaling*prevMaxSample, tempBuf, TEMP_BUF_SIZE).IsError())
                        {
                           retErrStr = qApp->translate("LibSndFileIOThread", "Error rescaling segment");
                           break;
                        }

                        prevOffset    = nextOffset;
                        prevMaxSample = nextMaxSample;
                     }
                     delete [] tempBuf;
                  }
                  if (retErrStr.length() == 0) _isComplete = true;
                  CloseFile(((retErrStr.length()>0)?CLOSE_FLAG_ERROR:0)|CLOSE_FLAG_FINAL);  // close file now so that it is ready for use immediately
               }
               return (retErrStr.length() > 0) ? ByteBufferRef() : buf;
            }
            else retErrStr = qApp->translate("LibSndFileIOThread", "Problem writing to file");
         }
      }
      else retErrStr = qApp->translate("LibSndFileIOThread", "Bad chunk length %1").arg(numBytes);
   }

   if (isLastBuffer)
   {
      if (retErrStr.length() == 0) _isComplete = true;
      CloseFile(((retErrStr.length()>0)?CLOSE_FLAG_ERROR:0)|CLOSE_FLAG_FINAL);  // close file now so that it isn't locked
   }
   else CloseFile(CLOSE_FLAG_ERROR);

   return ByteBufferRef();  // error!
}

status_t LibSndFileIOThread :: OpenFileForWriting(String & userFileName, const SF_INFO & info, SNDFILE * & retFile) const
{
   // Try to find a temp-file name that doesn't already exist in the output folder
   String fileName = userFileName + AUDIOMOVE_TEMP_SUFFIX;
   {
      FILE * fpTest = fopen(fileName(), "r");
      if ((fpTest)||(_tempFiles.ContainsKey(fileName)))
      {
         if (fpTest) fclose(fpTest);

         // Hmm, there's already a temp file here by that name... lets try a bit harder to find something unique.
         bool okayToGo = false;
         for (uint32 i=2; i<10000; i++)
         {
            String fn = fileName + String("_%1").Arg(i);
            if (_tempFiles.ContainsKey(fn) == false)
            {
               FILE * fpTest2 = fopen(fn(), "r");
               if (fpTest2) fclose(fpTest2);
               else
               {
                  okayToGo = true;
                  fileName = fn;
                  break;
               }
            }
         }
         if (okayToGo == false) return B_ERROR("Unable to choose temp-file name");  // couldn't find a free temp file name!?
      }
   }
   userFileName = fileName;

   const int openMode = IsOkayToRescale() ? SFM_RDWR : SFM_WRITE;  // if fixed-point, we may need to re-scale later
   SF_INFO tempInfo = info;
   SNDFILE * file = sf_open(fileName(), openMode, &tempInfo);
   if (file == NULL)
   {
      // No luck?  Okay, let's try again, without the endian requirement
      tempInfo = info;
      tempInfo.format &= ~SF_FORMAT_ENDMASK;
      file = sf_open(fileName(), openMode, &tempInfo);
   }

   if ((file == NULL)&&(sf_format_check(&tempInfo) != SF_TRUE)) return B_ERROR("Unsupported Channel Count or Sample Rate");

   // If we have a valid broadcast-info chunk, try to write it to the file
   if ((_biValid)&&(file))
   {
      // Note that this command may fail with older versions of libsndfile, because
      // we're in SFM_RDWR mode and it requires SFM_WRITE.  You can fix the problem
      // by hand-tweaking line 1077 of libsndfile's sndfile.c, or wait for a new
      // version of libsndfile to come out (I've emailed Erik about the problem)
      (void) sf_command(file, SFC_SET_BROADCAST_INFO, (void *) &_bi, sizeof(_bi));
   }

   retFile = file;
   return B_NO_ERROR;
}

status_t LibSndFileIOThread :: GetFilesThatWillBeOverwritten(Message & msg, const String & fn)
{
   if (_numWriteStreams != AUDIO_STREAMS_SOURCE)  // we never plan to overwrite anything when in input mode, of course
   {
      if ((_splitFiles)&&(_numWriteStreams > 1))
      {
          // New for AudioMove v1.15:  this mode writes each channel of audio to its own separate output file
          int lastDot = _fileName.LastIndexOf('.');
          for (uint32 i=0; i<_numWriteStreams; i++)
          {
             String fileName = (lastDot>=0) ? (_fileName.Substring(0, lastDot)+String("_c%1").Arg(i+1)+_fileName.Substring(lastDot)) : (_fileName+String("_c%1").Arg(i+1));
             FILE * fpCheck = fopen(fileName(), "rb");
             if (fpCheck)
             {
                fclose(fpCheck);
                MRETURN_ON_ERROR(msg.AddString(fn, fileName));
             }
          }
      }
      else
      {
         FILE * fpCheck = fopen(_fileName(), "rb");
         if (fpCheck)
         {
            fclose(fpCheck);
            MRETURN_ON_ERROR(msg.AddString(fn, _fileName));
         }
      }
   }
   return B_NO_ERROR;
}

status_t LibSndFileIOThread :: DoReadFromFiles(float * samples, uint32 numSamples)
{
   const uint32 numStreams = _files.GetNumItems();
   switch(numStreams)
   {
      case 0:
         return B_BAD_OBJECT;

      case 1:
         // Easy case:  we are just reading from a single file
         return (sf_read_float(*_files.GetFirstValue(), samples, numSamples) == numSamples) ? B_NO_ERROR : B_ERROR("sf_read_float() failed A");

      default:
      {
         // Harder case:  we are merging in data from multiple sub-files; we need to read one stream from each
         const uint32 numFrames = numSamples/numStreams;
         MRETURN_ON_ERROR(_splitBuf.SetNumBytes(numFrames*sizeof(float), false));  // space for one channel

         float * in = (float *) _splitBuf.GetBuffer();
         uint32 i=0;
         for (HashtableIterator<String, SNDFILE *> iter(_files, HTIT_FLAG_NOREGISTER); iter.HasData(); iter++)
         {
            if (sf_read_float(iter.GetValue(), in, numFrames) != numFrames) return B_ERROR("sf_read_float() failed B");
            float * out = &samples[i];
            for (uint32 j=0; j<numFrames; j++) out[j*numStreams] = in[j];
            i++;
         }
         return B_NO_ERROR;
      }
   }
}

status_t LibSndFileIOThread :: DoWriteToFiles(const float * samples, uint32 numSamples)
{
   if (_outputFileFormat == AUDIO_FORMAT_OGGVORBIS)
   {
      const uint32 MAX_OGG_SAMPLES_PER_CALL = 8*1024;  // FogBugz #10795:  work-around for crash in libvorbis if we try to write too many frames at once!
      if (numSamples > MAX_OGG_SAMPLES_PER_CALL)
      {
         while(numSamples > 0)
         {
            const uint32 numSamplesToWrite = muscleMin(numSamples, MAX_OGG_SAMPLES_PER_CALL);
            MRETURN_ON_ERROR(DoWriteToFiles(samples, numSamplesToWrite));
            samples    += numSamplesToWrite;
            numSamples -= numSamplesToWrite;
         }
         return B_NO_ERROR;
      }
   }

   const uint32 numStreams = _files.GetNumItems();
   switch(numStreams)
   {
      case 0:
         return B_BAD_OBJECT;

      case 1:
         // Easy case:  we are just writing to a single file
         return (sf_write_float(*_files.GetFirstValue(), samples, numSamples) == numSamples) ? B_NO_ERROR : B_ERROR("sf_write_float() failed A");

      default:
      {
         // Harder case:  we are splitting out to multiple sub-files; we need to write one stream to each
         const uint32 numFrames = numSamples/numStreams;
         MRETURN_ON_ERROR(_splitBuf.SetNumBytes(numFrames*sizeof(float), false));  // space for one channel

         float * out = (float *) _splitBuf.GetBuffer();
         uint32 i = 0;
         for (HashtableIterator<String, SNDFILE *> iter(_files, HTIT_FLAG_NOREGISTER); iter.HasData(); iter++)
         {
            const float * in = &samples[i];
            for (uint32 j=0; j<numFrames; j++) out[j] = in[j*numStreams];
            if (sf_write_float(iter.GetValue(), out, numFrames) != numFrames) return B_ERROR("sf_write_float() failed B");
            i++;
         }
         return B_NO_ERROR;
      }
   }
}

status_t LibSndFileIOThread :: DoSeekFiles(uint64 offset)
{
   for (HashtableIterator<String, SNDFILE *> iter(_files, HTIT_FLAG_NOREGISTER); iter.HasData(); iter++)
      if (sf_seek(iter.GetValue(), offset, SEEK_SET) != (sf_count_t)offset) return B_ERROR("sf_seek() failed");
   return B_NO_ERROR;
}

void LibSndFileIOThread :: DoCloseFiles(uint32 closeFlags)
{
   const bool isFinal = ((closeFlags & CLOSE_FLAG_FINAL) != 0);
   const bool isError = ((closeFlags & CLOSE_FLAG_ERROR) != 0);
   for (HashtableIterator<String, SNDFILE *> iter(_files, HTIT_FLAG_NOREGISTER); iter.HasData(); iter++)
   {
      // close the file FIRST to avoid any file-locking problems in Windows, etc
      sf_close(iter.GetValue());

      // For output mode, we need to clean up by renaming/deleting the output files as appropriate
      if ((isFinal)&&(_numWriteStreams != AUDIO_STREAMS_SOURCE))
      {
         const String & tmpFileName = iter.GetKey();

         if (isError == false)
         {
            // Success!  We want to safely replace the old/original file with our new temp file
            const int32 suffixIdx = tmpFileName.LastIndexOf(AUDIOMOVE_TEMP_SUFFIX);
            if (suffixIdx >= 0)
            {
               const String origFileName = tmpFileName.Substring(0, suffixIdx);
               (void) unlink(origFileName());
               (void) rename(tmpFileName(), origFileName());
               (void) _tempFiles.Remove(tmpFileName);

               // For in-place operations where we split a file, be sure to delete the original/unsplit file also
               if ((_splitFiles)&&(_numWriteStreams > 1)&&(_optInPlaceBasePath.HasChars()))
               {
                  unlink(_optInPlaceBasePath());
                  _optInPlaceBasePath.Clear();
               }
            }
         }
      }
   }
   _files.Clear();
   if (isFinal) DeleteTempFiles();  // make sure they get deleted on final close, even if we didn't have any file handles still
}

};  // end namespace audiomove
