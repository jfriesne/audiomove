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

#ifndef AKRipThread_h
#define AKRipThread_h

#include "akrip/akrip32.h"
#include "audiomove/AudioMoveThread.h"

namespace audiomove {

/** This is a Windows-only class that knows how to read audio from a CD via the akrip library */
class AKRipThread : public AudioMoveThread
{
public:
   /** Constructor for reading a file from disk. 
     * @param readPath the path to the track on a CD, e.g. "D:\Track 1.cda"
     */
   AKRipThread(const String & readPath);

   /** Destructor. */
   virtual ~AKRipThread();

   virtual status_t OpenFile();

   virtual void CloseFile(uint32 closeFlags);
   virtual status_t GetFilesThatWillBeOverwritten(Message &, const String &) {return B_NO_ERROR;}
   
   /** Returns our audio file's name */
   virtual const String & GetFileName() const {return _fileName;}

   /** Returns our audio input format code.  Set to WAV so that 
     * when the user selects "Audio Format = Source", he'll get a WAV file.
     */
   virtual uint32 GetInputFileFormat() const {return AUDIO_FORMAT_WAV;}

   /** Returns our audio output format code */
   virtual uint32 GetOutputFileFormat() const {return AUDIO_FORMAT_NORMALIZED;}

   /** Returns our audio sampling rate */
   virtual uint32 GetInputFileSampleRate()  const {return 44100;}
   virtual uint32 GetOutputFileSampleRate() const {return 44100;}

   /** Returns our audio sample width */
   virtual uint32 GetInputFileSampleWidth()  const {return AUDIO_WIDTH_INT16;}
   virtual uint32 GetOutputFileSampleWidth() const {return AUDIO_WIDTH_FLOAT;}

   /** Returns the number of streams in our file.  Only valid when file is open. */
   virtual uint8 GetFileStreams() const {return 2;}

   /** Returns the number of sample-frames in our file.  Only valid when file is open. */
   virtual uint64 GetFileFrames() const {return _numFrames;}

   virtual bool IsOutput() const {return false;}

protected:
   /** Reads or writes the next buffer of audio */
   virtual ByteBufferRef ProcessBuffer(const ByteBufferRef & inputRef, QString & retErrStr, bool isLastBuffer);

private:
   status_t CacheCDSectors(uint32 firstSector, uint32 numSectors);
   status_t ReadCDAudio(float * outBuf, int64 readOffsetFrames, uint32 numFrames);

   String _fileName;
   char _driveLetter;
   int _trackIndex;

   class AKRipDriveInterface
   {
   public:
      AKRipDriveInterface(char driveLetter);
      ~AKRipDriveInterface();

      char GetDriveLetter() const {return _driveLetter;}
      bool IsValid() const {return (_cdHandle != NULL);} 
      int GetFirstTrack() const {return _toc.firstTrack;}
      int GetLastTrack() const {return _toc.lastTrack;}

      const TOCTRACK & GetTrackInfo(int trackIndex) {return _toc.tracks[trackIndex-GetFirstTrack()];}
      DWORD ReadCDAudio(LPTRACKBUF trackBuf);

   private:
      friend class AKRipThread;

      // Note that the thread safety of this refcount is ensured
      // by the _driveDirectoryMutex, so we don't need to worry
      // about that here.
      void IncrementRefCount() {_refCount++;}
      bool DecrementRefCount() {return (--_refCount==0);}

      char _driveLetter;
      Mutex _cdMutex;

      HCDROM _cdHandle;
      uint32 _refCount;

      TOC _toc;
   };
   AKRipDriveInterface * _driveInterface;

   static AKRipDriveInterface * OpenAKRipDriveInterface(char driveLetter);
   static void CloseAKRipDriveInterface(AKRipDriveInterface * driveInterface);

   static Mutex _driveDirectoryMutex;
   static Hashtable<char, AKRipDriveInterface *> _driveDirectory;

   LPTRACKBUF _trackBuf;
   int64 _readOffsetFrames;

   uint32 _trackStartSector;
   uint32 _numSectorsInTrack;

   int64 _numFrames;
   bool _isComplete;

   OrderedKeysHashtable<uint32, ByteBufferRef> _cachedCDAudio;  // sector -> audio
};

};  // end namespace audiomove

#endif
