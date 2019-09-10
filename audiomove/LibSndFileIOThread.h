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

#ifndef LibSndFileIOThread_h
#define LibSndFileIOThread_h

#include "sndfile.h"
#include "audiomove/AudioMoveThread.h"

namespace audiomove {

/** This class knows how to read or write sound files using the libsndfile library */
class LibSndFileIOThread : public AudioMoveThread
{
public:
   /** Constructor for reading a file from disk. */
   LibSndFileIOThread(const String & readPath);

   /** Constructor for writing a file to disk. */
   LibSndFileIOThread(const String & checkPath, const String & writePath, const String & optInPlaceBasePath, uint32 writeFormat, uint32 writeSampleWidth, uint32 writeSampleRate, uint8 writeNumStreams, bool splitFiles, const SF_BROADCAST_INFO * optBI);

   /** Destructor. */
   virtual ~LibSndFileIOThread();

   virtual status_t OpenFile();

   virtual void CloseFile(uint32 closeFlags);
   virtual status_t GetFilesThatWillBeOverwritten(Message & msg, const String & fn);
   
   /** Returns our audio file's name */
   virtual const String & GetFileName() const {return _fileName;}

   /** Returns our audio input format code */
   virtual uint32 GetInputFileFormat() const {return _inputFileFormat;}

   /** Returns our audio output format code */
   virtual uint32 GetOutputFileFormat() const {return _outputFileFormat;}

   /** Returns our audio sampling rate */
   virtual uint32 GetInputFileSampleRate()  const {return _fileSampleRate;}
   virtual uint32 GetOutputFileSampleRate() const {return _fileSampleRate;}

   /** Returns our audio sample width */
   virtual uint32 GetInputFileSampleWidth()  const {return (_numWriteStreams > 0) ? (uint32)AUDIO_WIDTH_FLOAT : _fileSampleWidth;}
   virtual uint32 GetOutputFileSampleWidth() const {return (_numWriteStreams > 0) ? _fileSampleWidth : (uint32)AUDIO_WIDTH_FLOAT;}

   /** Returns the number of streams in our file.  Only valid when file is open. */
   virtual uint8 GetFileStreams() const {return _numStreams;}

   /** Returns the number of sample-frames in our file.  Only valid when file is open. */
   virtual uint64 GetFileFrames() const {return _numFrames;}

   virtual bool IsOutput() const {return (_numWriteStreams > 0);}

   status_t GetBroadcastInfo(SF_BROADCAST_INFO & bi);

protected:
   /** Reads or writes the next buffer of audio */
   virtual ByteBufferRef ProcessBuffer(const ByteBufferRef & inputRef, QString & retErrStr, bool isLastBuffer);

private:
   status_t RescaleAudioSegment(uint64 startOffset, uint64 numSamples, float scaleBy, float * tempBuf, uint32 tempBufSize);
   void RescaleAudioBuffer(float * buf, uint32 numFloats, float scaleBy) const;
   SNDFILE * OpenFileForWriting(String & fileName, const SF_INFO & info) const;
   status_t DoReadFromFiles(float * samples, uint32 numSamples);
   status_t DoWriteToFiles(const float * samples, uint32 numSamples);
   status_t DoSeekFiles(uint64 offset);
   void DoCloseFiles(uint32 closeFlags);
   status_t PutOutputFile(const String & fn, SNDFILE * file);
   void DeleteTempFiles();
   bool IsOkayToRescale() const;

   String _checkPath;  // used only in output mode
   String _fileName;
   String _optInPlaceBasePath;  // set only when we're doing a split, in-place conversion (so we'll remember to delete the original base filename)
   uint8 _numWriteStreams;
   bool _splitFiles;
   Hashtable<String, SNDFILE *> _files; // our currently open file handles
   Hashtable<String, bool> _tempFiles;  // keep track of all the temp files we've created, so we can be sure to clean them up at the end
   uint32 _inputFileFormat;
   uint32 _outputFileFormat;
   uint32 _fileSampleRate;
   uint32 _fileSampleWidth;
   int64 _numFrames;
   uint8 _numStreams;
   bool _isComplete;

   Hashtable<int64, float> _maxSamplesRecord;  // the max-sample-value-setting that various regions were written using
   float _currentMaxOutputSample;

   ByteBuffer _splitBuf;

   SF_BROADCAST_INFO _bi;
   bool _biValid;
};

};  // end namespace audiomove

#endif
