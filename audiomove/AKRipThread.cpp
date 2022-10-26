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
#include "audiomove/AKRipThread.h"
#include "audiomove/MiscFunctions.h"

#define NUM_BYTES_PER_SECTOR   2352  // from the Red book standard, it seems
#define NUM_BYTES_PER_SAMPLE16 sizeof(uint16)
#define NUM_BYTES_PER_SAMPLEFL sizeof(float)
#define NUM_SAMPLES_PER_FRAME  2     // because it's always stereo
#define NUM_SAMPLES_PER_SECTOR (NUM_BYTES_PER_SECTOR/NUM_BYTES_PER_SAMPLE16)
#define NUM_FRAMES_PER_SECTOR  (NUM_SAMPLES_PER_SECTOR/NUM_SAMPLES_PER_FRAME)
#define NUM_BYTES_PER_FRAME16  (NUM_SAMPLES_PER_FRAME*NUM_BYTES_PER_SAMPLE16)
#define NUM_BYTES_PER_FRAMEFL  (NUM_SAMPLES_PER_FRAME*NUM_BYTES_PER_SAMPLEFL)

#define NUM_SECTORS_PER_TRACKBUF 26 // fairly arbitrary AFAICT

#define SENSE_LEN_SPTI          32   /* Sense length for ASPI is only 14 */
#define NUM_MAX_NTSCSI_DRIVES   26   /* a: ... z:         */
#define NUM_FLOPPY_DRIVES       2
#define NUM_MAX_NTSCSI_HA       NUM_MAX_NTSCSI_DRIVES
#define NTSCSI_HA_INQUIRY_SIZE  36
#define SCSI_CMD_INQUIRY        0x12
#define SCSI_IOCTL_DATA_IN      1
#define   IOCTL_SCSI_BASE         0x00000004

#define   IOCTL_SCSI_PASS_THROUGH        CTL_CODE(IOCTL_SCSI_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define   IOCTL_SCSI_MINIPORT            CTL_CODE(IOCTL_SCSI_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define   IOCTL_SCSI_GET_INQUIRY_DATA    CTL_CODE(IOCTL_SCSI_BASE, 0x0403, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define   IOCTL_SCSI_GET_CAPABILITIES    CTL_CODE(IOCTL_SCSI_BASE, 0x0404, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define   IOCTL_SCSI_PASS_THROUGH_DIRECT CTL_CODE(IOCTL_SCSI_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define   IOCTL_SCSI_GET_ADDRESS         CTL_CODE(IOCTL_SCSI_BASE, 0x0406, METHOD_BUFFERED, FILE_ANY_ACCESS)

// I have to make my own #define, because Microsoft's define
// collides with the Qt::HANDLE type.  :^P
#define AUDIOMOVE_INVALID_HANDLE_VALUE ((::HANDLE)(-1))

namespace audiomove {

Mutex AKRipThread :: _driveDirectoryMutex;
Hashtable<char, AKRipThread::AKRipDriveInterface *> AKRipThread :: _driveDirectory;

AKRipThread :: AKRipThread(const String & readPath) : _fileName(readPath), _trackIndex(-1), _driveInterface(NULL), _trackBuf(NULL), _readOffsetFrames(0), _trackStartSector(0), _numSectorsInTrack(0), _numFrames(0), _isComplete(false)
{
   if (_fileName.EndsWith(".cda"))  // paranoia
   {
      const char * s = _fileName()+_fileName.Length()-5;
      while((s>=_fileName())&&(muscleInRange(*s, '0', '9'))) s--;
      s++;
      _trackIndex = atoi(s);

      _driveLetter = _fileName()[0];
      if (muscleInRange(_driveLetter, 'a', 'z')) _driveLetter -= ('a'-'A');
      if (muscleInRange(_driveLetter, 'A', 'Z') == false) _trackIndex = -1;
   }
}

AKRipThread :: ~AKRipThread()
{
   CloseFile(CLOSE_FLAG_FINAL|(_isComplete?0:CLOSE_FLAG_ERROR));
}

// jaf:  this code was snarfed out of scsi-wnt.c in cdrtools
typedef struct {
   BYTE   ha;              /* SCSI Bus #         */
   BYTE   tgt;             /* SCSI Target #      */
   BYTE   lun;             /* SCSI Lun #         */
   BYTE   PortNumber;      /* SCSI Card # (''.'SCSI%d)   */
   BYTE   PathId;          /* SCSI Bus/Channel # on card n   */
   BYTE   driveLetter;     /* Win32 drive letter (e.g. c:)   */
   BOOL   bUsed;           /* Win32 drive letter is used   */
   BYTE   inqData[NTSCSI_HA_INQUIRY_SIZE];
} DRIVE;

typedef struct {
   USHORT      Length;
   UCHAR      ScsiStatus;
   UCHAR      PathId;
   UCHAR      TargetId;
   UCHAR      Lun;
   UCHAR      CdbLength;
   UCHAR      SenseInfoLength;
   UCHAR      DataIn;
   ULONG      DataTransferLength;
   ULONG      TimeOutValue;
   PVOID      DataBuffer;
   ULONG      SenseInfoOffset;
   UCHAR      Cdb[16];
} SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;

typedef struct {
   SCSI_PASS_THROUGH_DIRECT spt;
   ULONG      Filler;
   UCHAR      ucSenseBuf[32];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

typedef struct {
   ULONG Length;
   UCHAR PortNumber;
   UCHAR PathId;
   UCHAR TargetId;
   UCHAR Lun;
} SCSI_ADDRESS, *PSCSI_ADDRESS;

static BOOL ShouldUseWindows2000StyleCreate()
{
   OSVERSIONINFO osver; memset(&osver, 0, sizeof (osver));
   osver.dwOSVersionInfoSize = sizeof (osver);
   GetVersionEx(&osver);
   if (osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
   {
      /* Win2000 is NT-5.0, Win-XP is NT-5.1 */
      if (osver.dwMajorVersion > 4) return true;
      if (osver.dwMajorVersion == 4)
      {
         /* NT-4.x */
         char * vers = (char *) osver.szCSDVersion;
         if (strlen(vers) == 0) return false;

         /*
          * Servicepack is installed, skip over non-digit part
          */
         while((*vers != '\0') && !isdigit(*vers)) vers++;
         if (*vers == '\0') return false;

         /* Fom Service Pack 4 */
         if (isdigit(vers[0]) && (atoi(vers) >= 4 || isdigit(vers[1]))) return true;  /* same as for W2K */
      }
   }
   return false;
}

/*
 * Universal function to get a file handle to the CD device.  Since
 * NT 4.0 wants just the GENERIC_READ flag, and Win2K wants both
 * GENERIC_READ and GENERIC_WRITE (why a read-only CD device needs
 * GENERIC_WRITE access is beyond me...), the easist workaround is to just
 * try them both.
 */
static ::HANDLE GetDriveFileHandle(BYTE i)
{
   DWORD   dwFlags = GENERIC_READ;
   DWORD   dwAccessMode = 0;

   dwAccessMode = FILE_SHARE_READ;
   if (ShouldUseWindows2000StyleCreate())
   {
      /* if Win2K or greater, add GENERIC_WRITE */
      dwFlags |= GENERIC_WRITE;
      dwAccessMode |= FILE_SHARE_WRITE;
   }
   char buf[12]; sprintf(buf, "\\\\.\\%c:", (char)('A'+i));

   ::HANDLE fh = CreateFileA(buf, dwFlags, dwAccessMode, NULL, OPEN_EXISTING, 0, NULL);
   if (fh == AUDIOMOVE_INVALID_HANDLE_VALUE)
   {
      /*
       * it went foobar somewhere, so try it with the GENERIC_WRITE
       * bit flipped
       */
      dwFlags ^= GENERIC_WRITE;
      dwAccessMode ^= FILE_SHARE_WRITE;
      fh = CreateFileA(buf, dwFlags, dwAccessMode, NULL, OPEN_EXISTING, 0, NULL);
   }
   return fh;
}

/*
 * fills in a pDrive structure with information from a SCSI_INQUIRY
 * and obtains the ha:tgt:lun values via IOCTL_SCSI_GET_ADDRESS
 */
static status_t GetDriveInformation(BYTE i, DRIVE * pDrive)
{
   ::HANDLE fh = GetDriveFileHandle(i);
   if (fh == AUDIOMOVE_INVALID_HANDLE_VALUE) return B_ERROR("GetDriveFileHandle() failed");

   /*
    * Get the drive inquiry data
    */
   SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER swb; memset(&swb, 0, sizeof (swb));
   BYTE inqData[NTSCSI_HA_INQUIRY_SIZE];     memset(inqData, 0, sizeof (inqData));
   swb.spt.Length      = sizeof (SCSI_PASS_THROUGH_DIRECT);
   swb.spt.CdbLength   = 6;
   swb.spt.SenseInfoLength   = 24;
   swb.spt.DataIn      = SCSI_IOCTL_DATA_IN;
   swb.spt.DataTransferLength = 100;
   swb.spt.TimeOutValue   = 2;
   swb.spt.DataBuffer   = inqData;
   swb.spt.SenseInfoOffset   = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
   swb.spt.Cdb[0]      = SCSI_CMD_INQUIRY;
   swb.spt.Cdb[4]      = NTSCSI_HA_INQUIRY_SIZE;

   ULONG returned;
   if (!DeviceIoControl(fh, IOCTL_SCSI_PASS_THROUGH_DIRECT, &swb, sizeof(swb), &swb, sizeof(swb), &returned, NULL)) {CloseHandle(fh); return B_ERROR("DeviceIOControl() failed A");}
   memcpy(pDrive->inqData, inqData, NTSCSI_HA_INQUIRY_SIZE);

   /*
    * get the address (path/tgt/lun) of the drive via IOCTL_SCSI_GET_ADDRESS
    */
   SCSI_ADDRESS scsiAddr; memset(&scsiAddr, 0, sizeof (SCSI_ADDRESS));
   scsiAddr.Length = sizeof (SCSI_ADDRESS);
   if (DeviceIoControl(fh, IOCTL_SCSI_GET_ADDRESS, NULL, 0, &scsiAddr, sizeof (SCSI_ADDRESS), &returned, NULL))
   {
      pDrive->bUsed       = true;
      pDrive->ha          = scsiAddr.PortNumber; /* preliminary */
      pDrive->PortNumber  = scsiAddr.PortNumber;
      pDrive->PathId      = scsiAddr.PathId;
      pDrive->tgt         = scsiAddr.TargetId;
      pDrive->lun         = scsiAddr.Lun;
      pDrive->driveLetter = i;
   }
   else
   {
      pDrive->bUsed   = false;
      CloseHandle(fh);
      return B_ERROR("DeviceIOControl() failed B");
   }
   CloseHandle(fh);
   return B_NO_ERROR;
}

static HCDROM OpenCDHandle(char driveLetter)
{
  CDLIST cdlist; memset(&cdlist, 0, sizeof(cdlist));
  GetCDList(&cdlist);
  if (cdlist.num == 0) return NULL;

  int whichDrive = -1;
  DRIVE drive; memset(&drive, 0, sizeof(drive));
  if (GetDriveInformation(driveLetter-'A', &drive).IsOK())
  {
     for (int i=0; i<cdlist.num; i++)
     {
        const CDREC & d = cdlist.cd[i];
        if ((d.ha == drive.ha)&&(d.tgt == drive.tgt)&&(d.lun == drive.lun))
        {
           whichDrive = i;
           break;
        }
     }
     if (whichDrive < 0) printf("Warning, couldn't find any drive with ha=%i, tgt=%i lun=%i in the %i drives listed!\n", drive.ha, drive.tgt, drive.lun, cdlist.num);
  }
  else printf("Warning, GetDriveInformation() failed!\n");

  if (whichDrive < 0) whichDrive = 0;  // if all else fails, just use the first drive
  GETCDHAND cdh; memset(&cdh, 0, sizeof(cdh));
  cdh.size     = sizeof(GETCDHAND);
  cdh.ver      = 1;
  cdh.ha       = cdlist.cd[whichDrive].ha;
  cdh.tgt      = cdlist.cd[whichDrive].tgt;
  cdh.lun      = cdlist.cd[whichDrive].lun;
  cdh.readType = CDR_ANY; // autodetect read method
  return GetCDHandle(&cdh);
}

AKRipThread::AKRipDriveInterface :: AKRipDriveInterface(char driveLetter) : _driveLetter(driveLetter), _cdHandle(NULL), _refCount(0)
{
   memset(&_toc, 0, sizeof(_toc));

   _cdHandle = OpenCDHandle(_driveLetter);
   if (_cdHandle)
   {
      ModifyCDParms(_cdHandle, CDP_MSF, false);
      if (ReadTOC(_cdHandle, &_toc) != SS_COMP)
      {
         memset(&_toc, 0, sizeof(_toc));
         CloseCDHandle(_cdHandle);
         _cdHandle = NULL;
      }
   }
}

AKRipThread::AKRipDriveInterface :: ~AKRipDriveInterface()
{
   if (_cdHandle) CloseCDHandle(_cdHandle);
}

DWORD AKRipThread::AKRipDriveInterface :: ReadCDAudio(LPTRACKBUF trackBuf)
{
   // I don't think akrip can handle multithreaded access,
   // so I'm serializing calls with a mutex.
   DWORD ret = SS_ERR;
   if (_cdMutex.Lock().IsOK())
   {
      ret = ReadCDAudioLBA(_cdHandle, trackBuf);
      _cdMutex.Unlock();
   }
   return ret;
}

AKRipThread::AKRipDriveInterface * AKRipThread::OpenAKRipDriveInterface(char driveLetter)
{
   AKRipDriveInterface * ret = NULL;
   if (_driveDirectoryMutex.Lock().IsOK())
   {
      if (_driveDirectory.Get(driveLetter, ret).IsError())
      {
         ret = new AKRipDriveInterface(driveLetter);
         if ((ret->IsValid() == false)||(_driveDirectory.Put(driveLetter, ret).IsError()))
         {
            delete ret;
            ret = NULL;
         }
      }
      if (ret) ret->IncrementRefCount();
      _driveDirectoryMutex.Unlock();
   }
   return ret;
}

void AKRipThread :: CloseAKRipDriveInterface(AKRipDriveInterface * di)
{
   if ((di)&&(_driveDirectoryMutex.Lock().IsOK()))
   {
      if (di->DecrementRefCount())
      {
         AKRipDriveInterface * paranoiaCheck;
         if (_driveDirectory.Remove(di->GetDriveLetter(), paranoiaCheck).IsOK())
         {
            if (paranoiaCheck != di) printf("ERROR, deleting wrong drive interface!  %p/%p\n", di, paranoiaCheck);
            delete di;
         }
         else printf("ERROR, couldn't find drive interface in table!\n");
      }
      _driveDirectoryMutex.Unlock();
   }
}

static uint32 InternalizeArg(const BYTE b[4])
{
  uint32 retVal;

  retVal = (DWORD)b[0];
  retVal = (retVal<<8) + (DWORD)b[1];
  retVal = (retVal<<8) + (DWORD)b[2];
  retVal = (retVal<<8) + (DWORD)b[3];
  return retVal;
}

status_t AKRipThread :: OpenFile()
{
   CloseFile(CLOSE_FLAG_FINAL);  // paranoia

   if (_trackIndex < 0) return B_BAD_OBJECT;

   const uint32 maxLen = NUM_SECTORS_PER_TRACKBUF*NUM_BYTES_PER_SECTOR;
   _trackBuf = (LPTRACKBUF) malloc(TRACKBUFEXTRA+maxLen);
   if (_trackBuf == NULL) return B_ERROR("malloc() failed");

   memset(_trackBuf, 0, TRACKBUFEXTRA);
   _trackBuf->startFrame  = 0;
   _trackBuf->numFrames   = 0;
   _trackBuf->maxLen      = maxLen;
   _trackBuf->status      = 0;
   _trackBuf->startOffset = 0;

   _driveInterface = OpenAKRipDriveInterface(_driveLetter);
   if ((_driveInterface)&&(muscleInRange(_trackIndex,_driveInterface->GetFirstTrack(),_driveInterface->GetLastTrack())))
   {
      const TOCTRACK & trackInfo = _driveInterface->GetTrackInfo(_trackIndex);
      if ((trackInfo.ADR & 0x04) == false)  // we don't convert data tracks!
      {
         const TOCTRACK & nextTrack = _driveInterface->GetTrackInfo(_trackIndex+1);

         _trackStartSector  = InternalizeArg(trackInfo.addr);
         _numSectorsInTrack = InternalizeArg(nextTrack.addr)-_trackStartSector;

         // The track before a data track is padded with 150 seconds of
         // leadout silence, which we don't want to encode.
         if (nextTrack.ADR & 0x04)
         {
            const uint32 LEADOUT = (150*75);
            if (_numSectorsInTrack > LEADOUT) _numSectorsInTrack -= LEADOUT;
         }

         _numFrames = ((int64)_numSectorsInTrack)*NUM_FRAMES_PER_SECTOR;
         if (_numFrames >= 0) return B_NO_ERROR;
      }
   }

   CloseFile(CLOSE_FLAG_ERROR);
   return B_ERROR;
}

void AKRipThread :: CloseFile(uint32 /*closeFlags*/)
{
   if (_trackBuf)
   {
      free(_trackBuf);
      _trackBuf = NULL;
   }
   if (_driveInterface)
   {
      CloseAKRipDriveInterface(_driveInterface);
      _driveInterface = NULL;
   }
   _cachedCDAudio.Clear();
   _readOffsetFrames = 0;
}

status_t AKRipThread :: CacheCDSectors(uint32 firstSector, uint32 numSectors)
{
   // Make sure we don't re-load any sectors that are already in memory
   while((numSectors > 0)&&(_cachedCDAudio.ContainsKey(firstSector)))
   {
      firstSector++;
      numSectors--;
   }

   uint32 curSector = firstSector;
   while(numSectors > 0)
   {
      uint32 retries = 3;
      DWORD dwStatus = SS_ERR;
      while((retries--)&&(dwStatus != SS_COMP))
      {
         _trackBuf->numFrames   = muscleMin(numSectors, (uint32)NUM_SECTORS_PER_TRACKBUF);
         _trackBuf->startOffset = 0;
         _trackBuf->len         = 0;
         _trackBuf->startFrame  = _trackStartSector+curSector;
         dwStatus = _driveInterface->ReadCDAudio(_trackBuf);
      }
      if (dwStatus != SS_COMP) return B_ERROR;

      const uint32 numSectorsRead = _trackBuf->numFrames;
      if (numSectorsRead == 0) return B_ERROR("numSectorsRead was 0");  // paranoia

      for (uint32 i=0; i<numSectorsRead; i++)
      {
         ByteBufferRef bufRef = GetByteBufferFromPool(NUM_SAMPLES_PER_SECTOR*NUM_BYTES_PER_SAMPLEFL);
         MRETURN_OOM_ON_NULL(bufRef());

         float * out = (float *) bufRef()->GetBuffer();
         const int16 * in = (const int16 *)(_trackBuf->buf+_trackBuf->startOffset+(i*NUM_BYTES_PER_SECTOR));
         for (uint32 j=0; j<NUM_SAMPLES_PER_SECTOR; j++) out[j] = (in[j]*(1.0f/0x8000));

         MRETURN_ON_ERROR(_cachedCDAudio.Put(curSector+i, bufRef));
      }
      curSector  += numSectorsRead;
      numSectors -= numSectorsRead;
   }
   return B_NO_ERROR;
}

status_t AKRipThread :: ReadCDAudio(float * outBuf, int64 curFrame, uint32 numFrames)
{
   uint32 firstSector = curFrame/NUM_FRAMES_PER_SECTOR;
   uint32 lastSector  = (curFrame+numFrames)/NUM_FRAMES_PER_SECTOR;
   if (((curFrame+numFrames)%NUM_FRAMES_PER_SECTOR) != 0) lastSector++;  // round up!

   // First, free up any old cached sectors that we know we don't need anymore
   while((_cachedCDAudio.HasItems())&&(*_cachedCDAudio.GetFirstKey() < firstSector)) _cachedCDAudio.RemoveFirst();

   // Now make sure that our required sectors are loaded
   MRETURN_ON_ERROR(CacheCDSectors(firstSector, lastSector-firstSector));

   // Now we'll go through the sectors and assemble their data into (outBuf)
   while(numFrames > 0)
   {
      const ByteBufferRef * bbr = _cachedCDAudio.Get(curFrame/NUM_FRAMES_PER_SECTOR);
      if (bbr == NULL) return B_DATA_NOT_FOUND;

      const float * sBuf     = (const float *) bbr->GetItemPointer()->GetBuffer();
      uint32 offsetFrames    = curFrame%NUM_FRAMES_PER_SECTOR;
      uint32 numFramesToCopy = muscleMin(numFrames, (uint32) (NUM_FRAMES_PER_SECTOR-offsetFrames));
      memcpy(outBuf, &sBuf[offsetFrames*NUM_SAMPLES_PER_FRAME], numFramesToCopy*NUM_BYTES_PER_FRAMEFL);

      curFrame  += numFramesToCopy;
      numFrames -= numFramesToCopy;
      outBuf    += numFramesToCopy*NUM_SAMPLES_PER_FRAME;
   }
   return B_NO_ERROR;
}

ByteBufferRef AKRipThread :: ProcessBuffer(const ByteBufferRef & buf, QString & retErrStr, bool isLastBuffer)
{
   if ((_driveInterface)&&(buf()))
   {
      const uint32 numBytes = buf()->GetNumBytes();
      if ((numBytes % NUM_BYTES_PER_SAMPLEFL) == 0)
      {
         const uint32 numFramesToRead = muscleMin((uint32)(numBytes/NUM_BYTES_PER_FRAMEFL), (uint32)(_numFrames-_readOffsetFrames));
         if (ReadCDAudio((float *)buf()->GetBuffer(), _readOffsetFrames, numFramesToRead).IsOK())
         {
            (void) buf()->SetNumBytes(numFramesToRead*NUM_BYTES_PER_FRAMEFL, true);
            if (isLastBuffer)
            {
               _isComplete = true;
               CloseFile(CLOSE_FLAG_FINAL);  // close file now so that it isn't locked
            }
            _readOffsetFrames += numFramesToRead;
            return buf;
         }
         else retErrStr = qApp->translate("AKRipThread", "Problem reading from CD");
      }
      else retErrStr = qApp->translate("AKRipThread", "Bad chunk length %1").arg(numBytes);
   }

   if (isLastBuffer)
   {
      if (retErrStr.length() == 0) _isComplete = true;
      CloseFile(((retErrStr.length()>0)?CLOSE_FLAG_ERROR:0)|CLOSE_FLAG_FINAL);  // close file now so that it isn't locked
   }
   else CloseFile(CLOSE_FLAG_ERROR);

   return ByteBufferRef();  // error!
}

};  // end namespace audiomove
