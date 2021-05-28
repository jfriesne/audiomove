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

#include <math.h>

#include "audiomove/SampleRateThread.h"
#include "util/NetworkUtilityFunctions.h"

namespace audiomove {

SampleRateThread :: SampleRateThread(uint32 inputRate, uint32 outputRate, uint32 quality, uint8 numStreams) : _inputRate(inputRate), _outputRate(outputRate), _quality(quality), _numStreams(numStreams), _codec(NULL)
{
   // empty
}

SampleRateThread :: ~SampleRateThread()
{
   CloseFileAux();
}

void SampleRateThread :: CloseFileAux()
{
   if (_codec)
   {
      src_delete(_codec);
      _codec = NULL;
   }
}

status_t SampleRateThread :: OpenFile()
{
   if (_inputRate != _outputRate)
   {
      if ((_numStreams == 0)||(_outputRate == 0)) return B_BAD_OBJECT;  // paranoia:  avoid divide-by-zero

      CloseFileAux();  // paranoia
      _inputFrame = _availFrames = 0;

      int q; 
      switch(_quality)
      {
         default:  // fall thru!                              
         case AUDIO_CONVERSION_QUALITY_BEST:   q = SRC_SINC_BEST_QUALITY;   break;
         case AUDIO_CONVERSION_QUALITY_BETTER: q = SRC_SINC_MEDIUM_QUALITY; break;
         case AUDIO_CONVERSION_QUALITY_GOOD:   q = SRC_SINC_FASTEST;        break;
      }

      int err;
      _codec = src_new(q, _numStreams, &err);
      if (_codec == NULL) return B_LOGIC_ERROR;

      // Set this field here -- all the other fields will be set inside GenerateBlock().
      _data.src_ratio = ((double)_outputRate) / ((double)_inputRate);
   }
   return B_NO_ERROR;
}

void SampleRateThread :: CloseFile(uint32 /*closeFlags*/)
{
   CloseFileAux();
}

ByteBufferRef SampleRateThread :: OutOfMemory(QString & retErrString)
{
   retErrString = qApp->translate("SampleRateThread", "Out of Memory");
   CloseFile(CLOSE_FLAG_ERROR);
   return ByteBufferRef();
}

ByteBufferRef SampleRateThread :: ProcessBuffer(const ByteBufferRef & buf, QString & retErrStr, bool isLastBuffer)
{
   if (_inputRate == _outputRate) return buf;  // easy case -- no conversion necessary

   // First, queue up the new input buffer
   {
      if ((buf() == NULL)||(_inputBuffers.AddTail(buf).IsError())) return OutOfMemory(retErrStr);
      _availFrames += buf()->GetNumBytes()/(sizeof(float)*_numStreams);
   }

   // We'll store our generated output frames into outBuf...
   int32 outBufFrames = (uint32) ((_availFrames+128)*_data.src_ratio);  // +128 is just paranoia to avoid round-off-induced shortages

   ByteBufferRef outBuf = GetByteBufferFromPool(outBufFrames*_numStreams*sizeof(float));
   if (outBuf() == NULL) return OutOfMemory(retErrStr);

   float * outPtr = (float *) outBuf()->GetBuffer();
   int32 framesWritten = 0;
   while(framesWritten < outBufFrames)
   {
      int32 genFrames = GenerateNextBlock(outPtr, outBufFrames-framesWritten, ((isLastBuffer)&&(_inputBuffers.GetNumItems() <= 1)), retErrStr);
      if (genFrames > 0)
      {
         outPtr        += (genFrames*_numStreams);
         framesWritten += genFrames;
      }
      else if (genFrames < 0) 
      {
         CloseFile(CLOSE_FLAG_ERROR);
         return ByteBufferRef();
      }
      else break;
   }

   (void) outBuf()->SetNumBytes(framesWritten*sizeof(float)*_numStreams, true);  // only truncates, so this should never fail
   return outBuf;
}

int32 SampleRateThread::GenerateNextBlock(float * outBuffer, uint32 outBufferFrames, bool isLastBuffer, QString & retErrStr)
{
   if (_inputBuffers.GetNumItems() > 0)
   {
      const ByteBuffer & bb = *_inputBuffers.Head()();

      _data.data_in       = ((float *)bb.GetBuffer()) + (_inputFrame*_numStreams);
      _data.input_frames  = (bb.GetNumBytes()/(_numStreams*sizeof(float)))-_inputFrame;
      _data.data_out      = outBuffer;
      _data.output_frames = outBufferFrames;
      _data.end_of_input  = isLastBuffer ? 1 : 0;

      int result = src_process(_codec, &_data);
      if (result != 0)
      {
         retErrStr = ToQ(src_strerror(result));
         return -1;
      }

      _availFrames -= _data.input_frames_used;
      _inputFrame  += _data.input_frames_used;
      if ((_inputFrame*_numStreams*sizeof(float)) == bb.GetNumBytes())
      {
         _inputBuffers.RemoveHead();
         _inputFrame = 0;
      }
      return _data.output_frames_gen;
   }
   return 0;
}

};  // end namespace audiomove

