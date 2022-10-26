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

#ifndef AudioMoveThread_h
#define AudioMoveThread_h

#include <QApplication>

#include "system/Thread.h"
#include "util/ByteBuffer.h"
#include "audiomove/AudioMoveConstants.h"

namespace audiomove {

/** Interface class for an object that can accept ByteBuffers */
class IMessageAcceptor
{
public:
   IMessageAcceptor() {/* empty */}
   virtual ~IMessageAcceptor() {/* empty */}
   virtual status_t MessageReceivedFromUpstream(const MessageRef & bufRef) = 0;
};

enum {
   CLOSE_FLAG_FINAL = (1<<0), // set when this is the last time we will close the file
   CLOSE_FLAG_ERROR = (1<<1)  // set when an error was detected
};

/** Abstract base class for all threads in an audio move process */
class AudioMoveThread : public IMessageAcceptor, public Thread, public RefCountable
{
public:
   /** Default constructor */
   AudioMoveThread();

   /** Standard destructor */
   virtual ~AudioMoveThread();

   /** Opens the file specified in the constructor for reading or writing.
    *  @return B_NO_ERROR on success, or B_ERROR on failure.
    */
   virtual status_t OpenFile() = 0;

   /** Returns the name of the file we have open. */
   virtual const String & GetFileName() const = 0;

   /** Closes any file we have open. */
   virtual void CloseFile(uint32 flags) = 0;

   /** Should add to (msg) the full file paths of any existing file that we intend
     * to overwrite when OpenFile() is called.
     * @param msg the Message to add to
     * @param fn the field name to add the strings under.
     * @returns B_NO_ERROR on success, or B_ERROR on failure (out of memory?)
     */
   virtual status_t GetFilesThatWillBeOverwritten(Message & msg, const String & fn) = 0;

   /** Returns our audio format code */
   virtual uint32 GetInputFileFormat() const = 0;

   /** Returns our audio format code */
   virtual uint32 GetOutputFileFormat() const = 0;

   /** Returns our input audio sampling rate, in Hz. */
   virtual uint32 GetInputFileSampleRate() const = 0;

   /** Returns our output audio sampling rate, in Hz. */
   virtual uint32 GetOutputFileSampleRate() const = 0;

   /** Returns the AUDIO_WIDTH_* value indicating the sample width of the data we are bringing in. */
   virtual uint32 GetInputFileSampleWidth() const = 0;

   /** Returns the AUDIO_WIDTH_* value indicating the sample width of the data we are spitting out. */
   virtual uint32 GetOutputFileSampleWidth() const = 0;

   /** Returns the number of streams in our file.  Only valid when file is open. */
   virtual uint8 GetFileStreams() const = 0;

   /** Returns the number of sample-frames in our file.  Only valid when file is open. */
   virtual uint64 GetFileFrames() const = 0;

   /** Should return true iff this object is ever supposed to write to disk. */
   virtual bool IsOutput() const = 0;

   /** Our implementation of the ByteBufferAcceptor interface:  send it to our internal thread */
   virtual status_t MessageReceivedFromUpstream(const MessageRef & msg);

   /** Sets where we should send Messages to after we are done processing them. */
   void SetDownstreamAcceptor(IMessageAcceptor * dsa) {_downstreamAcceptor = dsa;}

   /** Convenience method -- returns the duration of our input file, in microseconds. */
   int64 GetInputFileDurationMicros() const
   {
      int64 isr = GetInputFileSampleRate();
      return (isr > 0) ? (1000000*GetFileFrames())/isr : 0;
   }

protected:
#ifndef WIN32
# ifdef QT_HAS_THREAD_PRIORITIES
   /** Overridden to make our worker threads be low priority, since they are a batch process anyway */
   virtual QThread::Priority GetInternalQThreadPriority() const {return QThread::LowestPriority;}
# endif
#endif

   /** Each subclass should implement this method to do the appropriate
    *  action with the given ByteBufferRef, and return the result.
    *  @param buf Reference to the buffer to operate on.
    *  @param retErrStr On error, you can write an informative error string here to show the user.
    *  @param isLastBuffer If set true, then (buf) is the last buffer in the stream.
    *  @returns The output data from this stage (may be buf), or a NULL reference if there was an error.
    */
   virtual ByteBufferRef ProcessBuffer(const ByteBufferRef & buf, QString & retErrStr, bool isLastBuffer) = 0;

   /** Overridden to call ProcessBuffer() */
   virtual status_t MessageReceivedFromOwner(const MessageRef & msg, uint32 numLeft);

private:
   IMessageAcceptor * _downstreamAcceptor;
};
DECLARE_REFTYPES(AudioMoveThread);

};  // end namespace audiomove

#endif
