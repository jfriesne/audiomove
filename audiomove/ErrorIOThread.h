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

#ifndef ErrorIOThread_h
#define ErrorIOThread_h

#include "dataio/DataIO.h"
#include "audiomove/AudioMoveThread.h"

namespace audiomove {

/** This class doesn't know how to do anything; it's just here so we can report errors nicely in the GUI */
class ErrorIOThread : public AudioMoveThread
{
public:
   /** Constructor. */
   ErrorIOThread(const String & file) : _filePath(file) {/* empty */}

   /** Destructor. */
   virtual ~ErrorIOThread() {/* empty */}

   virtual status_t OpenFile() {return B_ERROR;}
   virtual void CloseFile(uint32 /*closeFlags*/) {/* empty */}
   virtual status_t GetFilesThatWillBeOverwritten(Message & /*msg*/, const String & /*fn*/) {return B_NO_ERROR;}
   
   /** Returns our audio format code */
   virtual const String & GetFileName() const {return _filePath;}

   /** Returns our audio format code */
   virtual uint32 GetInputFileFormat() const {return NUM_AUDIO_FORMATS;}
   virtual uint32 GetOutputFileFormat() const {return NUM_AUDIO_FORMATS;}

   /** Returns our audio sampling rate */
   virtual uint32 GetInputFileSampleRate() const  {return 0;}
   virtual uint32 GetOutputFileSampleRate() const {return 0;}

   virtual uint32 GetInputFileSampleWidth() const {return NUM_AUDIO_WIDTHS;}
   virtual uint32 GetOutputFileSampleWidth() const {return NUM_AUDIO_WIDTHS;}

   /** Returns the number of streams in our file.  Only valid when file is open. */
   virtual uint8 GetFileStreams() const {return 0;}

   /** Returns the number of sample-frames in our file.  Only valid when file is open. */
   virtual uint64 GetFileFrames() const {return 0;}

   /** ErrorIOThreads never write to the disk. */
   virtual bool IsOutput() const {return false;}

protected:
   /** Reads or writes the next buffer of audio */
   virtual ByteBufferRef ProcessBuffer(const ByteBufferRef & /*inputRef*/, QString & /*retErrStr*/, bool /*isLastBuffer*/) {return ByteBufferRef();}

private:
   String _filePath;
};

};  // end namespace audiomove

#endif
