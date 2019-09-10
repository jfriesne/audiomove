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

#ifndef SampleRateThread_h
#define SampleRateThread_h

#include "samplerate.h"  // from libsamplerate/src/include
#include "audiomove/AudioMoveThread.h"

namespace audiomove {

/** This class knows how to change the sampling rate of the data that passes through it.  */
class SampleRateThread : public AudioMoveThread
{
public:
   /** Constructor. */
   SampleRateThread(uint32 inputRate, uint32 outputRate, uint32 quality, uint8 numStreams);

   /** Destructor. */
   virtual ~SampleRateThread();

   virtual status_t OpenFile();
   virtual void CloseFile(uint32 closeFlags);
   virtual status_t GetFilesThatWillBeOverwritten(Message &, const String &) {return B_NO_ERROR;}
   virtual const String & GetFileName()      const {return GetEmptyString();}
   virtual uint32 GetInputFileFormat()       const {return AUDIO_FORMAT_NORMALIZED;}
   virtual uint32 GetOutputFileFormat()      const {return AUDIO_FORMAT_NORMALIZED;}
   virtual uint32 GetInputFileSampleRate()   const {return _inputRate;}
   virtual uint32 GetOutputFileSampleRate()  const {return _outputRate;}
   virtual uint32 GetInputFileSampleWidth()  const {return AUDIO_WIDTH_FLOAT;}
   virtual uint32 GetOutputFileSampleWidth() const {return AUDIO_WIDTH_FLOAT;}
   virtual uint8 GetFileStreams()            const {return _numStreams;}
   virtual uint64 GetFileFrames()            const {return 0;}
   virtual ByteBufferRef ProcessBuffer(const ByteBufferRef & buf, QString & retErrStr, bool isLastBuffer);

   /* SampleRateThreads never write to the disk. */
   virtual bool IsOutput() const {return false;}

   /** Returns our quality level, as specified in the constructor. */
   uint32 GetConversionQuality() const {return _quality;}

private:
   void CloseFileAux();
   int32 GenerateNextBlock(float * outBuffer, uint32 outBufferFrames, bool isLastBuffer, QString & retErrStr);
   ByteBufferRef OutOfMemory(QString & retErrString);

   uint32 _inputRate;
   uint32 _outputRate;
   uint32 _quality;
   uint8 _numStreams;

   Queue<ByteBufferRef> _inputBuffers;  // our store of received input data awaiting procesing
   uint32 _inputFrame;   // current frames offset into the first BB in _inputBuffers
   uint32 _availFrames;  // tally of total number of frames currently available in _inputBuffers

   SRC_STATE * _codec;
   SRC_DATA _data;
};

};  // end namespace audiomove

#endif
