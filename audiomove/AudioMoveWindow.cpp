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

#include <QApplication>
#include <QPainter>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QTreeWidget>
#include <QLayout>
#include <QAbstractButton>
#include <QPushButton>
#include <QScrollBar>
#include <QStatusBar>
#include <QTextLayout>
#include <QTimer>
#include <QUrl>
#include <QResizeEvent>
#include <QPixmap>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QEvent>
#include <QDropEvent>
#include <QEvent>
#include <QItemDelegate>
#include <QStackedWidget>
#include <QToolButton>

#include "audiomove/AudioMoveConstants.h"
#include "audiomove/AudioMoveMessageBox.h"
#include "audiomove/AudioMovePopupMenu.h"
#include "audiomove/AudioMoveWindow.h"
#include "audiomove/AudioMoveFileDialog.h"
#include "audiomove/CreateLCSDisk.h"
#include "audiomove/ErrorIOThread.h"
#include "audiomove/LibSndFileIOThread.h"
#include "audiomove/SampleRateThread.h"
#include "audiomove/MiscFunctions.h"
#include "util/StringTokenizer.h"

#ifdef WIN32
# include "win32dirent.h"
# include "audiomove/AKRipThread.h"
#else
# include <dirent.h>
#endif

bool _isReadBroadcastInfoEnabled = true;

namespace audiomove {

enum {
   MOVE_COLUMN_JOB_ID,
   MOVE_COLUMN_SOURCE_FILE,
   MOVE_COLUMN_DEST_FILE,
   MOVE_COLUMN_PERCENT_DONE,
   MOVE_COLUMN_STATUS,
   MOVE_COLUMN_LENGTH,
   MOVE_COLUMN_CHANNELS,
   MOVE_COLUMN_SOURCE_FORMAT,
   MOVE_COLUMN_SOURCE_RATE,
   MOVE_COLUMN_SOURCE_WIDTH,
   MOVE_COLUMN_DEST_FORMAT,
   MOVE_COLUMN_DEST_RATE,
   MOVE_COLUMN_DEST_WIDTH,
   MOVE_COLUMN_QUALITY,
   NUM_MOVE_COLUMNS
};

#define BUFFER_RETURNED_EVENT_TYPE ((QEvent::Type)(QEvent::User+600))

class BufferReturnedEvent : public QEvent
{
public:
   BufferReturnedEvent(const MessageRef & msg, bool isFromSetup) : QEvent(BUFFER_RETURNED_EVENT_TYPE), _msg(msg), _isFromSetup(isFromSetup) {/* empty */}

   MessageRef GetMessage() const {return _msg;}

   bool IsFromSetup() const {return _isFromSetup;}

private:
   MessageRef _msg;
   bool _isFromSetup;
};

enum {
   SETUP_COMMAND_SETUP = 1000,
   SETUP_REPLY_RESULTS
};

#define SETUP_NAME_SETUPRESULT      "rslt"
#define SETUP_NAME_SOURCEFILE       "file"
#define SETUP_NAME_TARGETFORMAT     "tfmt"
#define SETUP_NAME_TARGETRATE       "trat"
#define SETUP_NAME_TARGETWIDTH      "twid"
#define SETUP_NAME_DESTDIR          "ddir"
#define SETUP_NAME_INPUTTHREAD      "ithr"
#define SETUP_NAME_OUTPUTTHREAD     "othr"
#define SETUP_NAME_QUALITY          "qual"
#define SETUP_NAME_ERROR            "errr"
#define SETUP_NAME_SPLITFILES       "split"
#define SETUP_NAME_FILESTOBEOVERWRITTEN "ftbo"

static bool ParseBool(const String & text, bool defVal)
{
   String s = text.ToUpperCase();
   if ((s.IndexOf('1') >= 0)||(s.IndexOf('Y') >= 0)||(s.IndexOf('T') >= 0)) return true;
   if ((s.IndexOf('0') >= 0)||(s.IndexOf('N') >= 0)||(s.IndexOf('F') >= 0)) return false;
   return defVal;
}

// Returns (dir + GetPathSepChar() + file) properly, even if dir and file already have GetPathSepChar() in them
static String AddPath(const String & dir, const String & file)
{
   String d = dir;
   while(d.EndsWith(GetPathSepChar())) d = d.Substring(0, d.Length()-GetPathSepChar().Length());

   String f = file;
   while(f.StartsWith(GetPathSepChar())) f = f.Substring(GetPathSepChar().Length());
  
   return d + GetPathSepChar() + f; 
}

/** This thread handles the initial opening and setup of file items, so that the GUI isn't blocked during setup */
class AudioSetupThread : public Thread
{
public:
   AudioSetupThread(AudioMoveWindow * win) : _window(win) {/* empty */}

protected:
   virtual status_t MessageReceivedFromOwner(const MessageRef & msgRef, uint32 numLeft)
   {
      Message * msg = msgRef();
      if ((msg)&&(msg->what == SETUP_COMMAND_SETUP))
      {
         String sourceFile, destDir;
         (void) msg->FindString(SETUP_NAME_DESTDIR, destDir);   // if not found, do an in-place conversion (FogBugz #3398)

         uint32 targetFormat, targetRate, targetWidth, quality;
         bool splitFiles;
         if ((msg->FindString(SETUP_NAME_SOURCEFILE, sourceFile)              == B_NO_ERROR)&&
             (msg->FindInt32(SETUP_NAME_TARGETFORMAT, (int32*) &targetFormat) == B_NO_ERROR)&&
             (msg->FindInt32(SETUP_NAME_TARGETRATE,   (int32*) &targetRate)   == B_NO_ERROR)&&
             (msg->FindInt32(SETUP_NAME_TARGETWIDTH,  (int32*) &targetWidth)  == B_NO_ERROR)&&
             (msg->FindInt32(SETUP_NAME_QUALITY,      (int32*) &quality)      == B_NO_ERROR)&&
             (msg->FindBool(SETUP_NAME_SPLITFILES,   &splitFiles)             == B_NO_ERROR)) DoSetup(destDir, sourceFile, destDir, targetFormat, targetRate, targetWidth, quality, splitFiles);
         return B_NO_ERROR;
      }
      else return Thread::MessageReceivedFromOwner(msgRef, numLeft);
   }

   void DoSetup(const String & origDestDir, const String & sourceFile, const String & destDir, uint32 targetFormat, uint32 targetSampleRate, uint32 targetSampleWidth, uint32 quality, bool splitFiles)
   {
      MessageRef ret = GetMessageFromPool(SETUP_REPLY_RESULTS);
      if (ret())
      {
         DoSetupAux(origDestDir, *ret(), sourceFile, destDir, targetFormat, targetSampleRate, targetSampleWidth, quality, splitFiles);
         QApplication::postEvent(_window, new BufferReturnedEvent(ret, true));
      }
   }

   void DoSetupAux(const String & origDestDir, Message & addMsgTo, const String & sourceFile, const String & destDir, uint32 targetFormat, uint32 targetSampleRate, uint32 targetSampleWidth, uint32 quality, bool splitFiles)
   {
      // First see if the sourceFile is really a directory.  If so, then we will recurse to its contents
      DIR * dir = opendir(sourceFile());
      if (dir)
      {
         String newDestDir = AddPath(destDir, (sourceFile.EndsWith("/")?sourceFile.Substring(0,sourceFile.Length()-1):sourceFile).Substring("/"));
         struct dirent * de;
         while((de = readdir(dir)) != NULL)
         {
            const char * n = de->d_name;
            if (n[0] != '.') DoSetupAux(origDestDir, addMsgTo, AddPath(sourceFile, n), newDestDir, targetFormat, targetSampleRate, targetSampleWidth, quality, splitFiles);
         }
         closedir(dir);
      }
      else
      {
         MessageRef subMsg = GetMessageFromPool();
         if (subMsg())
         {
            QString errorString;
            AudioMoveThreadRef inputThreadRef;

#ifdef WIN32
            /* For Windows, CD audio has to be read using a completely
             * different API.  Thanks, Microsoft!
             */
            if (sourceFile.EndsWith(".cda")) inputThreadRef.SetRef(new AKRipThread(sourceFile));
#endif

            /** In most cases, we just read the file using libsndfile */
            LibSndFileIOThread * inputThread = NULL;
            if (inputThreadRef()==NULL) 
            {
               inputThread = new LibSndFileIOThread(sourceFile);
               inputThreadRef.SetRef(inputThread);
            }
            
            SF_BROADCAST_INFO bi; memset(&bi, 0, sizeof(bi));
            bool biValid = false;
            if (inputThreadRef()->OpenFile() == B_NO_ERROR)
            {
               if ((inputThread)&&(_isReadBroadcastInfoEnabled)) biValid = (inputThread->GetBroadcastInfo(bi) == B_NO_ERROR);
            }
            else
            {
               inputThreadRef.SetRef(new ErrorIOThread(sourceFile));
               inputThread = NULL;
               errorString = qApp->translate("AudioSetupThread", "Couldn't open input file");
            }

            if (targetFormat      == AUDIO_FORMAT_SOURCE) targetFormat      = inputThreadRef()->GetInputFileFormat();
            if (targetSampleRate  == AUDIO_RATE_SOURCE)   targetSampleRate  = inputThreadRef()->GetInputFileSampleRate();
            if (targetSampleWidth == AUDIO_WIDTH_SOURCE)  targetSampleWidth = inputThreadRef()->GetInputFileSampleWidth();

            String dd;
            if (origDestDir.HasChars()) dd = destDir;
            else
            {
               // FogBugz #3398:  Use the source file's directory as the destination dir also
               dd = sourceFile;
               int32 lastSlash = dd.LastIndexOf('/');
               if (lastSlash >= 0) dd = dd.Substring(0, lastSlash+1);
            }

            if (dd.EndsWith("/") == false) dd += '/';
            String baseFileName = sourceFile.Substring("/");
            String destFile = dd + baseFileName;

            const char * newExt = origDestDir.HasChars() ? GetAudioFormatExtensions(targetFormat) : NULL;
            if (newExt) destFile = ReplaceExtension(destFile, newExt);
#ifdef WIN32  
            // This is so that e.g. adding D:/ as a folder will work;
            // without this change we'd try to create a folder named "D:"
            // and Windows would refuse to do it, causing the conversion to
            // fail (output error)
            if ((destFile.Length()>3)&&(destFile[1]==':')&&(destFile[2]=='/'))
            {
               String temp = destFile.Substring(3);
               temp.Replace(":/", "/");
               destFile = destFile.Substring(0,3)+temp;
            }
            else destFile.Replace(":/", "/");
#endif

            if (targetSampleRate == AUDIO_RATE_SOURCE)  targetSampleRate  = inputThreadRef()->GetInputFileSampleRate();
            if (targetSampleRate == AUDIO_WIDTH_SOURCE) targetSampleWidth = inputThreadRef()->GetInputFileSampleWidth();

            targetSampleRate = GetSampleRateValueFromCode(targetSampleRate);  // ensure that we have the actually value in Hz

            inputThreadRef()->CloseFile(0);  // we'll open it again when we are ready to do the actual conversion

            AudioMoveThreadRef outputThread;
            if (errorString.length() > 0) outputThread.SetRef(new ErrorIOThread(destFile));
            else
            {
               switch(targetFormat)
               {
                  case AUDIO_FORMAT_WAV:
                  case AUDIO_FORMAT_AIFF: 
                  case AUDIO_FORMAT_FLAC:
                  case AUDIO_FORMAT_OGGVORBIS:
                  case AUDIO_FORMAT_PAF_BE:
                  case AUDIO_FORMAT_PAF_LE:
                  case AUDIO_FORMAT_WAV64:
                  case AUDIO_FORMAT_CAF:
                  case AUDIO_FORMAT_RF64:
                  case AUDIO_FORMAT_OGGOPUS:
                  {
                     String odd = origDestDir;
                     if (odd.EndsWith("/") == false) odd += '/';
                     odd += "dummy";  // because EnsureFileFolderExists() expects to see a filename at the end... it doesn't matter what

                     // If we are converting sample rates, we need to update our time-reference count to match 
                     if (biValid)
                     {
                        uint32 inputSampleRate = inputThreadRef()->GetInputFileSampleRate();
                        if ((targetSampleRate > 0)&&(inputSampleRate > 0)&&(inputSampleRate != targetSampleRate))
                        {
                           int64 oldTimeRef = (((int64)bi.time_reference_high)<<32)|(((int64)bi.time_reference_low)&(0xFFFFFFFF));
                           int64 newTimeRef = (oldTimeRef*targetSampleRate)/inputSampleRate;
                           bi.time_reference_high = (uint32)(newTimeRef>>32); 
                           bi.time_reference_low  = (uint32)(newTimeRef&0xFFFFFFFF);
                        }
                     }

                     outputThread.SetRef(new LibSndFileIOThread(odd, destFile, (sourceFile==destFile)?sourceFile:"", targetFormat, targetSampleWidth, targetSampleRate, inputThreadRef()->GetFileStreams(), splitFiles, biValid?&bi:NULL));
                     if ((outputThread())&&(origDestDir.HasChars())) (void) outputThread()->GetFilesThatWillBeOverwritten(*subMsg(), SETUP_NAME_FILESTOBEOVERWRITTEN);
                  }
                  break;

                  default:  
                     errorString = qApp->translate("AudioSetupThread", "Unknown output format");
                     outputThread.SetRef(new ErrorIOThread(destFile));
                  break;
               }
            }

            if ((subMsg()->AddInt32(SETUP_NAME_QUALITY,    quality)                             == B_NO_ERROR)&&
                (subMsg()->AddTag(SETUP_NAME_INPUTTHREAD,  inputThreadRef.GetRefCountableRef()) == B_NO_ERROR)&&
                (subMsg()->AddTag(SETUP_NAME_OUTPUTTHREAD, outputThread.GetRefCountableRef())   == B_NO_ERROR)&&
                ((errorString.length() == 0)||(subMsg()->AddString(SETUP_NAME_ERROR, FromQ(errorString)) == B_NO_ERROR))) (void) addMsgTo.AddMessage(SETUP_NAME_SETUPRESULT, subMsg);
         }
      }
   }

   const char * GetAudioFormatExtensions(uint32 format) const
   {
      switch(format)
      {
         case AUDIO_FORMAT_WAV:       return ".wav";
         case AUDIO_FORMAT_AIFF:      return ".aiff;.aif";
         case AUDIO_FORMAT_FLAC:      return ".flac;.fla";
         case AUDIO_FORMAT_OGGVORBIS: return ".ogg;.oga";
         case AUDIO_FORMAT_PAF_BE:    return ".paf";
         case AUDIO_FORMAT_PAF_LE:    return ".paf";
         case AUDIO_FORMAT_WAV64:     return ".w64";
         case AUDIO_FORMAT_CAF:       return ".caf";
         case AUDIO_FORMAT_RF64:      return ".rf64";
         case AUDIO_FORMAT_OGGOPUS:   return ".opus";
      }
      return NULL;
   }

   String ReplaceExtension(const String & ss, const String & ext) const
   {
      String s = ss;
      bool useUpperCase = false;

#ifdef WIN32
      // A special case for Windows CD files
      if (ReplaceExtensionAux(s, ".cda", useUpperCase) != B_NO_ERROR)
#endif
      {
         // First, remove any existing extension...
         for (uint32 i=0; i<NUM_AUDIO_FORMATS; i++)
         {
            const char * e = GetAudioFormatExtensions(i);
            if (e)
            {
               StringTokenizer tok(e, ";");
               const char * nextExt;
               while((nextExt = tok()) != NULL)
               {
                  if (ReplaceExtensionAux(s, nextExt, useUpperCase) == B_NO_ERROR) break;
               }
            }
         }
      }

      // Then add our new extension
      return s + (useUpperCase ? ext.ToUpperCase() : ext.ToLowerCase()).Substring(0, ";");
   }
 
   status_t ReplaceExtensionAux(String & s, const String & es, bool & useUpperCase) const
   {
      if (s.EndsWithIgnoreCase(es) == false) return B_ERROR;

      uint32 extStart = s.Length()-es.Length();
      useUpperCase = muscleInRange(s[extStart+1], 'A', 'Z');
      s = s.Substring(0, extStart);
      return B_NO_ERROR;
   }

   AudioMoveWindow * _window;
};

AudioMoveItem :: AudioMoveItem(uint32 tag, AudioMoveWindow * owner, QTreeWidget * lv, const AudioMoveThreadRef & inputThread, const AudioMoveThreadRef & convertThread, const AudioMoveThreadRef & outputThread) : QTreeWidgetItem(lv), _tag(tag), _internalThreadsStarted(false), _status(MOVE_STATUS_CONFIRMING), _samplesComplete(0), _totalSamples(inputThread()->GetFileFrames()*inputThread()->GetFileStreams()), _numActiveBuffers(0), _owner(owner), _lastUpdateTime(0), _isFirstUpdate(true)
{
   _samplesLeft                = _totalSamples;
   _threads[STAGE_SOURCE]      = inputThread;
   _threads[STAGE_CONVERSION]  = convertThread;
   _threads[STAGE_DESTINATION] = outputThread;

   for (uint32 i=0; i<ARRAYITEMS(_threads)-1; i++) _threads[i]()->SetDownstreamAcceptor(_threads[i+1]());
   _threads[ARRAYITEMS(_threads)-1]()->SetDownstreamAcceptor(owner);
}

AudioMoveItem :: ~AudioMoveItem()
{
   ShutdownInternalThreads();
}

void AudioMoveItem :: ShutdownInternalThreads()
{
   // Make sure our worker threads all get shut down safely
   for (uint32 i=0; i<ARRAYITEMS(_threads); i++) _threads[i]()->ShutdownInternalThread();
}

void AudioMoveItem :: SetStatus(uint32 s)
{
   if (s != _status)
   {
      _status = s;
      Update(true);
      _owner->UpdateButtons();
   }
}

QColor AudioMoveItem :: GetColorForStatus(uint32 status) const
{
   QColor color;
   switch(status)
   {
      case MOVE_STATUS_CONFIRMING: return QColor(100,255,255);
      case MOVE_STATUS_WAITING:    return QColor(255,255,255);
      case MOVE_STATUS_PROCESSING: return QColor(255,255,200);
      case MOVE_STATUS_COMPLETE:   return QColor(000,255,000);
      case MOVE_STATUS_ERROR:      return QColor(255,100,100);
      default:                     return white;
   }
}

status_t AudioMoveItem :: StartInternalThreads()
{
   if (_internalThreadsStarted == false)
   {
      MessageRef startup = GetMessageFromPool(_tag);
      if ((startup())&&(startup()->AddBool(AUDIOMOVE_NAME_OPENFILE, true) == B_NO_ERROR)&&(_threads[STAGE_SOURCE]()->MessageReceivedFromUpstream(startup) == B_NO_ERROR))
      {
         _numActiveBuffers++;  // the OPENFILE command counts as a buffer!
         for (uint32 i=0; i<ARRAYITEMS(_threads); i++) 
         {
            if (_threads[i]()->StartInternalThread() != B_NO_ERROR) 
            {
               SetStatus(MOVE_STATUS_ERROR);
               ShutdownInternalThreads();
               SetStatusString(qApp->translate("AudioMoveItem", "Couldn't spawn worker thread!"));
               Update(true);
               return B_ERROR;
            }
         }
         _internalThreadsStarted = true;
      }
      else return B_ERROR;
   }
   return B_NO_ERROR;
}

void AudioMoveItem :: SetStatusString(const QString & s)
{
   _statusString = s;
   Update(true);
}

void AudioMoveItem :: Update(bool full)
{
   if (full)
   {
      const AudioMoveThread  * input  = _threads[STAGE_SOURCE]();
      const SampleRateThread * conv   = (SampleRateThread *) _threads[STAGE_CONVERSION]();
      const AudioMoveThread  * output = _threads[STAGE_DESTINATION]();

      setText(MOVE_COLUMN_JOB_ID,        ToQ("%1").arg(_tag));
      setText(MOVE_COLUMN_SOURCE_FILE,   LocalToQ(input->GetFileName()()));
      setText(MOVE_COLUMN_SOURCE_FORMAT, _owner->GetFileFormatName(input->GetInputFileFormat()));
      setText(MOVE_COLUMN_SOURCE_RATE,   _owner->GetSampleRateName(input->GetInputFileSampleRate(), true));
      setText(MOVE_COLUMN_SOURCE_WIDTH,  _owner->GetSampleWidthName(input->GetInputFileSampleWidth(), true));
      setText(MOVE_COLUMN_LENGTH,        _owner->GetFileDurationName(input->GetFileFrames(), input->GetInputFileSampleRate()));
      setText(MOVE_COLUMN_CHANNELS,      ToQ("%1").arg(input->GetFileStreams()));
      setText(MOVE_COLUMN_DEST_FILE,     LocalToQ(output->GetFileName()()));
      setText(MOVE_COLUMN_DEST_FORMAT,   _owner->GetFileFormatName(output->GetOutputFileFormat()));
      setText(MOVE_COLUMN_DEST_RATE,     _owner->GetSampleRateName(output->GetOutputFileSampleRate(), true));
      setText(MOVE_COLUMN_DEST_WIDTH,    _owner->GetSampleWidthName(output->GetOutputFileSampleWidth(), true));
      setText(MOVE_COLUMN_QUALITY,       _owner->GetConversionQualityName(conv->GetConversionQuality()));
      setText(MOVE_COLUMN_STATUS,        (_statusString.length() > 0) ? _statusString : _owner->GetStatusName(GetStatus()));
   }
   char buf[64]; sprintf(buf, "%.01f%%", GetPercentDone()*100.0f);
   setText(MOVE_COLUMN_PERCENT_DONE, ToQ(buf));
   static_cast<AudioMoveTreeWidget *>(treeWidget())->UpdateRow(this);
}

bool AudioMoveItem :: operator<(const QTreeWidgetItem & item) const
{
   const AudioMoveItem & rhs = static_cast<const AudioMoveItem &>(item);
   int col = treeWidget()->sortColumn();
   const AudioMoveThread * myInput   = _threads[STAGE_SOURCE]();
   const AudioMoveThread * myOutput  = _threads[STAGE_DESTINATION]();
   const SampleRateThread * myConv   = (SampleRateThread *) _threads[STAGE_CONVERSION]();
   const AudioMoveThread * hisInput  = rhs._threads[STAGE_SOURCE]();
   const AudioMoveThread * hisOutput = rhs._threads[STAGE_DESTINATION]();
   const SampleRateThread * hisConv  = (SampleRateThread *) rhs._threads[STAGE_CONVERSION]();
   switch(col)
   {
      case MOVE_COLUMN_JOB_ID:        return _tag                                  < rhs._tag;
      case MOVE_COLUMN_SOURCE_FILE:   return myInput->GetFileName()                < hisInput->GetFileName();
      case MOVE_COLUMN_SOURCE_FORMAT: return myInput->GetInputFileFormat()         < hisInput->GetInputFileFormat();
      case MOVE_COLUMN_SOURCE_RATE:   return myInput->GetInputFileSampleRate()     < hisInput->GetInputFileSampleRate();
      case MOVE_COLUMN_SOURCE_WIDTH:  return myInput->GetInputFileSampleWidth()    < hisInput->GetInputFileSampleWidth();
      case MOVE_COLUMN_LENGTH:        return myInput->GetInputFileDurationMicros() < hisInput->GetInputFileDurationMicros();
      case MOVE_COLUMN_CHANNELS:      return myInput->GetFileStreams()             < hisInput->GetFileStreams();
      case MOVE_COLUMN_DEST_FILE:     return myOutput->GetFileName()               < hisOutput->GetFileName();
      case MOVE_COLUMN_DEST_FORMAT:   return myOutput->GetOutputFileFormat()       < hisOutput->GetOutputFileFormat();
      case MOVE_COLUMN_DEST_RATE:     return myOutput->GetOutputFileSampleRate()   < hisOutput->GetOutputFileSampleRate();
      case MOVE_COLUMN_DEST_WIDTH:    return myOutput->GetOutputFileSampleWidth()  < hisOutput->GetOutputFileSampleWidth();
      case MOVE_COLUMN_QUALITY:       return myConv->GetConversionQuality()        < hisConv->GetConversionQuality();
      case MOVE_COLUMN_PERCENT_DONE:  return GetPercentDone()                      < rhs.GetPercentDone();
      case MOVE_COLUMN_STATUS:        
      {
         int ret = muscleCompare(GetStatus(), rhs.GetStatus());
         return ret ? (ret<0) : (_statusString<rhs._statusString);
      }
   }
   return QTreeWidgetItem::operator<(rhs);
}

void AudioMoveItem :: Halt()
{
   if (_status == MOVE_STATUS_PROCESSING) _status = MOVE_STATUS_WAITING;
}

status_t AudioMoveItem :: SendBuffers(bool allowChangeStatus)
{
   if (_status != MOVE_STATUS_ERROR)
   {
      if (allowChangeStatus == false)
      {
         // only allow us to do anything if we are one of the first
         // (n) active items.
         bool isOkay = false;
         uint32 maxNumActive = _owner->GetMaxProcesses();
         uint32 numActive = 0;
         for (HashtableIterator<uint32, AudioMoveItem *> iter(_owner->_moveItems); ((numActive<maxNumActive)&&(iter.HasData())); iter++)
         {
            AudioMoveItem * next = iter.GetValue();
            if (next->GetStatus() == MOVE_STATUS_PROCESSING) numActive++;
            if (next == this) {isOkay = true; break;}
         }
         if (isOkay == false)
         {
            // Whoops!  Our priority got reduced.  Go back to waiting
            // and let the earlier guys finish first.
            if (GetStatus() != MOVE_STATUS_WAITING)
            {
               SetStatus(MOVE_STATUS_WAITING);
               Update(true);
            }
            return B_NO_ERROR;
         }
      }

      if (_samplesComplete >= _totalSamples)
      {
         if (_totalSamples == 0)
         {
            // Special case for 0-sample files:  send a Message just to flush everyone's temp-files out
            MessageRef msg = GetMessageFromPool(_tag);
            if ((msg())&&(msg()->AddBool(AUDIOMOVE_NAME_ISLAST, true) == B_NO_ERROR)) (void) _threads[STAGE_SOURCE]()->MessageReceivedFromUpstream(msg);
         }
         SetStatus(MOVE_STATUS_COMPLETE);  // nothing more to do!
         ShutdownInternalThreads();
      }
      else if (((_status == MOVE_STATUS_PROCESSING)||(_status == MOVE_STATUS_WAITING))&&(_owner->_pause->isChecked() == false))
      {
         static const uint32 MAX_BUFFERS_PER_PROCESS = 5;
         static const uint32 MAX_BUFFER_SIZE_SAMPLES = 64*1024;  // 256KB buffers (i.e. 64K*sizeof(float)) ought to be large enough

         // FogBugz #4511:  make sure buffer is a multiple of the input file's frame size!
         AudioMoveThread * input = _threads[STAGE_SOURCE]();
         const uint32 frameSizeSamples = input->GetFileStreams();
         if (frameSizeSamples > 0)
         {
            if (allowChangeStatus) SetStatus(MOVE_STATUS_PROCESSING);
            while((_samplesLeft > 0)&&(_numActiveBuffers < MAX_BUFFERS_PER_PROCESS))
            {
               MessageRef msg = GetMessageFromPool(_tag);
               uint32 bufSamples = (((uint32) muscleMin((uint64)MAX_BUFFER_SIZE_SAMPLES, _samplesLeft))/frameSizeSamples)*frameSizeSamples;  // FogBugz #4511, FogBugz #4530
               ByteBufferRef buf = GetByteBufferFromPool(bufSamples * sizeof(float));
               if ((msg())&&(buf())&&(bufSamples>0)                                                             &&
                   ((bufSamples < _samplesLeft)||(msg()->AddBool(AUDIOMOVE_NAME_ISLAST, true) == B_NO_ERROR))   &&
                   (msg()->AddInt32(AUDIOMOVE_NAME_ORIGSIZE, bufSamples) == B_NO_ERROR)                         &&
                   (msg()->AddFlat(AUDIOMOVE_NAME_BUF, FlatCountableRef(buf.GetRefCountableRef(), true)) == B_NO_ERROR) &&
                   (input->MessageReceivedFromUpstream(msg) == B_NO_ERROR)) 
               {
                  _samplesLeft -= bufSamples;
                  _numActiveBuffers++;
               }
               else break;  // avoid infinite loop!
            }
            return (_numActiveBuffers > 0) ? B_NO_ERROR : B_ERROR;
         }
      }
   }
   return B_ERROR;
}

status_t AudioMoveItem :: MessageReceivedFromUpstream(const MessageRef & msg)
{
   if (msg())
   {
      if (msg()->HasName(AUDIOMOVE_NAME_OPENFILE))
      {
         if (msg()->HasName(AUDIOMOVE_NAME_STATUS))
         {
            SetStatus(MOVE_STATUS_ERROR);
            ShutdownInternalThreads();
         }
      }
      else
      {
         int32 osize;
         if ((msg()->HasName(AUDIOMOVE_NAME_BUF))&&(msg()->FindInt32(AUDIOMOVE_NAME_ORIGSIZE, &osize) == B_NO_ERROR)) _samplesComplete += osize;
         else 
         {
            if (_totalSamples > 0) SetStatus((_totalSamples>0)?MOVE_STATUS_ERROR:MOVE_STATUS_COMPLETE);
            ShutdownInternalThreads();
         }
      }

      const char * s;
      if (msg()->FindString(AUDIOMOVE_NAME_STATUS, &s) == B_NO_ERROR) SetStatusString(ToQ(s));
   }

   _numActiveBuffers--;

   // limit the updates to 5 per second, to avoid spending too much CPU time updating the GUI
   uint64 now = GetRunTime64();
   if (now > _lastUpdateTime+500000)
   {
      _lastUpdateTime = now;
      Update(_isFirstUpdate);  // do a more complete update the first time, in case FLAC output forced a destination sample width change
      _isFirstUpdate = false;
   }

   if (_status == MOVE_STATUS_PROCESSING) (void) SendBuffers(false);
   return B_NO_ERROR;
}

class AudioMoveTreeItemDelegate : public QItemDelegate
{
public:
   AudioMoveTreeItemDelegate(AudioMoveTreeWidget * tree, QObject * parent) : QItemDelegate(parent), _tree(tree) {/* empty */}

   virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
   {
      _paintingRow = index.row();
      _paintingCol = index.column();
      _isSelected  = (option.state & QStyle::State_Selected);

      TextElideMode textElideMode = option.textElideMode;
      switch(index.column())
      {  
         case MOVE_COLUMN_SOURCE_FILE:
         case MOVE_COLUMN_DEST_FILE:
            textElideMode = ElideLeft;
         break;
      }
      QStyleOptionViewItem to(option);
      to.textElideMode = textElideMode;
      to.state &= ~QStyle::State_Selected;  // so that we can choose our own colors
      QItemDelegate::paint(painter, to, index);
   }

   // I overloaded drawCheck() instead of drawBackground() because for some reason QItemDelegate::drawBackground()
   // is not declared virtual and therefore my implementation here isn't called if I override it.
   virtual void drawCheck(QPainter *painter, const QStyleOptionViewItem &option, const QRect & /*r*/, Qt::CheckState /*state*/) const
   {
      QColor bc = white;
      float p = _tree->GetProcessingPercentageForRow(_paintingRow, bc);

      if (_isSelected) bc = MakeHighlightedColor(bc);

      const QRect & rect = option.rect;
      if ((p >= 0.0f)&&(p < 1.0f))
      {
         // Find my offset within the scrolled contents
         int myOffset   = -1;
         int totalWidth = 0;
         QHeaderView * h = _tree->header();
         for (int i=0; i<_tree->columnCount(); i++)
         {
            int logicalIndex = h->logicalIndex(i);
            if (h->isSectionHidden(logicalIndex) == false)
            {
               if (logicalIndex == _paintingCol) myOffset = totalWidth;
               totalWidth += h->sectionSize(logicalIndex);
            }
         }

         // Convert that to an offset within the viewport's width
         int viewportWidth = muscleMin(_tree->viewport()->width(), totalWidth);
         myOffset -= _tree->horizontalScrollBar()->value();

         int x = muscleClamp(muscleRintf(viewportWidth*p)-myOffset, 0, rect.width());
         if (x > 0)
         {
            QRect leftRect(rect.left(), rect.top(), x, rect.height());
            QColor y = yellow;
            if (_isSelected) y = MakeHighlightedColor(y);
            painter->fillRect(leftRect, y);
         }
         if (x < rect.width())
         {
            QRect rightRect(rect.left()+x, rect.top(), rect.width()-x, rect.height());
            painter->fillRect(rightRect, bc);
         }
      }
      else painter->fillRect(rect, bc);
   }

private:
   QColor MakeHighlightedColor(const QColor & c) const
   { 
      QColor hc = c;
           if (hc.red()   > 75) hc.setRed((hc.red()*4)/5);
      else if (hc.green() > 75) hc.setGreen((hc.green()*4)/5);
      else if (hc.blue()  > 75) hc.setBlue((hc.blue()*4)/5);
      return hc;
   }

   AudioMoveTreeWidget * _tree;

   // these are valid only during calls to drawCheck()
   mutable int _paintingRow;
   mutable int _paintingCol;
   mutable bool _isSelected;
};

AudioMoveTreeWidget :: AudioMoveTreeWidget(QWidget * parent) : QTreeWidget(parent), _displayMenu(NULL)
{
   setSortingEnabled(true);
   setRootIsDecorated(false);
   setItemDelegate(new AudioMoveTreeItemDelegate(this, this));

   setAcceptDrops(true);
   setSelectionMode(ExtendedSelection);
   setSelectionBehavior(SelectRows);
   setUniformRowHeights(true);
 
   setColumnCount(NUM_MOVE_COLUMNS);
   header()->setStretchLastSection(false);
   header()->installEventFilter(this);

   QStringList headerLabels;
   for (uint32 i=0; i<NUM_MOVE_COLUMNS; i++) headerLabels.push_back(GetMoveColumnName(i));
   setHeaderLabels(headerLabels);
}

enum {
   DISPLAY_MENU_SHOW_ALL_COLUMNS = 6000,
   DISPLAY_MENU_HIDE_ALL_COLUMNS,
   DISPLAY_MENU_RESET_COLUMN_ORDERING
};

void AudioMoveTreeWidget :: GetDisplayMenuInfo(bool & canResetOrdering, int & visCount) const
{
   QHeaderView * h = header();
   canResetOrdering = false;
   visCount = 0;
   for (int i=MOVE_COLUMN_JOB_ID+1; i<h->count(); i++) 
   {
      if (h->visualIndex(i) != i) canResetOrdering = true;
      if (!h->isSectionHidden(i)) visCount++;
   }
}

bool AudioMoveTreeWidget :: eventFilter(QObject * o, QEvent * e)
{
   const QHeaderView * h = header();
   if ((o == h)&&(e->type() == QEvent::ContextMenu))
   {
      delete _displayMenu;

      _displayMenu = new AudioMovePopupMenu(false, this, SLOT(DisplayMenuItemChosen(int, int, int)), this);
      _displayMenu->setTearOffEnabled(true);

      int visCount;
      bool canResetOrdering;
      GetDisplayMenuInfo(canResetOrdering, visCount);

      for (int i=MOVE_COLUMN_JOB_ID+1; i<h->count(); i++) _displayMenu->InsertCheckboxItem(!h->isSectionHidden(i), tr("Show") + ToQ(" ") + GetMoveColumnName(i), i);

      _displayMenu->InsertSeparator();
      _displayMenu->InsertCommandItem(tr("Show All Columns"), DISPLAY_MENU_SHOW_ALL_COLUMNS, ((visCount+MOVE_COLUMN_JOB_ID+1)<h->count()));
      _displayMenu->InsertCommandItem(tr("Hide All Columns"), DISPLAY_MENU_HIDE_ALL_COLUMNS, (visCount>0));
      _displayMenu->InsertSeparator();
      _displayMenu->InsertCommandItem(tr("Reset Column Ordering"), DISPLAY_MENU_RESET_COLUMN_ORDERING, canResetOrdering);
      _displayMenu->Go(0, 0, header()->logicalIndexAt(static_cast<QMouseEvent *>(e)->pos()));
      return true;
   }
   return QTreeWidget::eventFilter(o, e);
}

void AudioMoveTreeWidget :: UpdateDisplayMenu()
{
   if ((_displayMenu)&&(_displayMenu->isTearOffMenuVisible()))
   {
      bool canResetOrdering = false;
      int visCount = 0;
      GetDisplayMenuInfo(canResetOrdering, visCount);
      for (int32 i=header()->count()-1; i>MOVE_COLUMN_JOB_ID; i--) _displayMenu->SetItemChecked(i, !header()->isSectionHidden(i));

      _displayMenu->SetItemEnabled(DISPLAY_MENU_SHOW_ALL_COLUMNS,      ((int)visCount<header()->count()));
      _displayMenu->SetItemEnabled(DISPLAY_MENU_HIDE_ALL_COLUMNS,      (visCount>0));
      _displayMenu->SetItemEnabled(DISPLAY_MENU_RESET_COLUMN_ORDERING, canResetOrdering);
   }
}

void AudioMoveTreeWidget :: SetSectionHidden(int which, bool hidden)
{
   // To avoid weirdness, we never hide the Job ID column
   if (which != MOVE_COLUMN_JOB_ID) header()->setSectionHidden(which, hidden);
}

void AudioMoveTreeWidget :: DisplayMenuItemChosen(int, int, int menuItemID)
{
   QHeaderView * h = header();
   switch(menuItemID)
   {
      case DISPLAY_MENU_SHOW_ALL_COLUMNS:
         for (int i=0; i<h->count(); i++) SetSectionHidden(i, false);
      break;

      case DISPLAY_MENU_HIDE_ALL_COLUMNS:
         for (int i=0; i<h->count(); i++) SetSectionHidden(i, true);
      break;

      case DISPLAY_MENU_RESET_COLUMN_ORDERING:
         for (int i=0; i<h->count(); i++) h->moveSection(h->visualIndex(i), i);
      break;

      default:
         SetSectionHidden(menuItemID, h->isSectionHidden(menuItemID) == false);
      break;
   }
   UpdateDisplayMenu();
}

void AudioMoveTreeWidget :: UpdateRow(AudioMoveItem * ami) 
{
   QHeaderView * h = header();
   int lastIndex = NUM_MOVE_COLUMNS-1;
   while((lastIndex > 0)&&(h->isSectionHidden(h->logicalIndex(lastIndex)))) lastIndex--;

   viewport()->update(visualRect(indexFromItem(ami,h->logicalIndex(0))) | visualRect(indexFromItem(ami,h->logicalIndex(lastIndex))));
}

float AudioMoveTreeWidget :: GetProcessingPercentageForRow(int row, QColor & retBackgroundColor) const
{
   AudioMoveItem * ami = static_cast<AudioMoveItem *>(topLevelItem(row));
   if (ami) retBackgroundColor = ami->GetColorForStatus(ami->GetStatus());
   return ami ? ami->GetPercentDone() : -1.0f;
}

QString AudioMoveTreeWidget :: GetMoveColumnName(uint32 i) const
{
   switch(i)
   {
      case MOVE_COLUMN_JOB_ID:        return tr("Job #");
      case MOVE_COLUMN_SOURCE_FILE:   return tr("Src File");
      case MOVE_COLUMN_SOURCE_FORMAT: return tr("Src Format");
      case MOVE_COLUMN_SOURCE_RATE:   return tr("Src Rate");
      case MOVE_COLUMN_SOURCE_WIDTH:  return tr("Src Width");
      case MOVE_COLUMN_LENGTH:        return tr("Duration");
      case MOVE_COLUMN_CHANNELS:      return tr("Chans");
      case MOVE_COLUMN_DEST_FILE:     return tr("Dest File");
      case MOVE_COLUMN_DEST_FORMAT:   return tr("Dest Format");
      case MOVE_COLUMN_DEST_RATE:     return tr("Dest Rate");
      case MOVE_COLUMN_DEST_WIDTH:    return tr("Dest Width");
      case MOVE_COLUMN_QUALITY:       return tr("Quality");
      case MOVE_COLUMN_PERCENT_DONE:  return tr("Completed");
      case MOVE_COLUMN_STATUS:        return tr("Status");
      default:                        return tr("Unknown");
   }
}

void AudioMoveTreeWidget :: dragEnterEvent(QDragEnterEvent * e)
{
   if (e->mimeData()->hasFormat("text/uri-list")) e->acceptProposedAction();
                                             else QTreeWidget::dragEnterEvent(e);
}

void AudioMoveTreeWidget :: dropEvent(QDropEvent * e)
{
   if ((e->mimeData()->hasFormat("text/uri-list"))&&(e->mimeData()->hasUrls()))
   {
      e->acceptProposedAction();
      QList<QUrl> urls = e->mimeData()->urls();
      QStringList sl;
      for (int i=0; i<urls.size(); i++) 
      {
         QString s = urls[i].toLocalFile();
         if (s.length() > 0) sl.push_back(s);  // for some reason Qt4 adds an empty URL
      }
      emit FilesDropped(sl);
   }
   else QTreeWidget::dropEvent(e);
}

void AudioMoveTreeWidget :: resizeEvent(QResizeEvent * e)
{
   QTreeWidget::resizeEvent(e);
   update();
}

#define DEFAULT_LCSDISK_NAME "wtrx-1-scsi-1.lcsDisk"

AudioMoveWindow :: AudioMoveWindow(const Message & args, QWidget * parent, WindowFlags f) 
   : QMainWindow(parent, f)
   , _chooseDestDir(NULL)
   , _addFiles(NULL)
   , _addFolders(NULL)
   , _quitOnIdle(false)
   , _forceQuit(false)
   , _numInitializing(0)
   , _createLCSDisk(NULL)
   , _lcsDiskFile(LocalToQ(DEFAULT_LCSDISK_NAME))
   , _tagCounter(0)
   , _confirmationDialog(NULL)
   , _audioSetupThread(NULL)
   , _updateComboBoxBackgroundsPending(false)
{
   // Setup GUI
   QWidget * cw = new QWidget(this);
   setCentralWidget(cw);

   QBoxLayout * mainLayout = NewVBoxLayout(cw, 3);
   {
      QFrame * settingsGroup = new QFrame(cw);
      {
         QBoxLayout * settingsGroupLayout = NewVBoxLayout(settingsGroup, 3);

         QWidget * lineOne = new QWidget(settingsGroup);
         {
            QBoxLayout * lineOneLayout = NewHBoxLayout(lineOne, 0, 5);

            _targetFormat = CreateSettingsComboBox(tr("&Output Format:"), lineOne, lineOneLayout);
            connect(_targetFormat, SIGNAL(activated(int)), this, SLOT(UpdateButtons()));
            for (uint32 i=0; i<NUM_AUDIO_FORMATS; i++) if (i != AUDIO_FORMAT_NORMALIZED) _targetFormat->addItem(GetFileFormatName(i));

            _targetSampleRate = CreateSettingsComboBox(tr("Ra&te:"), lineOne, lineOneLayout);
            for (uint32 j=0; j<NUM_AUDIO_RATES; j++) _targetSampleRate->addItem(GetSampleRateName(j, false));

            _targetSampleWidth = CreateSettingsComboBox(tr("W&idth:"), lineOne, lineOneLayout);
            for (uint32 k=0; k<NUM_AUDIO_WIDTHS; k++) _targetSampleWidth->addItem(GetSampleWidthName(k, false));

            _conversionQuality = CreateSettingsComboBox(tr("&Quality:"), lineOne, lineOneLayout);
            for (uint32 m=0; m<NUM_AUDIO_CONVERSION_QUALITIES; m++) _conversionQuality->addItem(GetConversionQualityName(m));

            _maxSimultaneous = CreateSettingsComboBox(tr("T&hreads:"), lineOne, lineOneLayout);
            for (uint32 n=0; n<MAX_SIMULTANEOUS_PROCESSES; n++) _maxSimultaneous->addItem(ToQ("%1").arg(n+1));
            connect(_maxSimultaneous, SIGNAL(activated(int)), this, SLOT(MaxSimultaneousChanged()));

            _splitMultiTrackFiles = new QCheckBox(tr("&Split Multi-Track Files"), lineOne);
            lineOneLayout->addWidget(_splitMultiTrackFiles);
         }
         settingsGroupLayout->addWidget(lineOne);

         QWidget * lineTwo = new QWidget(settingsGroup);
         {
            QBoxLayout * lineTwoLayout = NewHBoxLayout(lineTwo, 0, 5);

            _inPlaceConversions = new QCheckBox(tr("Convert &In Place"), lineTwo);
            connect(_inPlaceConversions, SIGNAL(clicked()), this, SLOT(ConvertInPlaceToggled()));
            connect(_inPlaceConversions, SIGNAL(stateChanged(int)), this, SLOT(UpdateButtons()));
            lineTwoLayout->addWidget(_inPlaceConversions);
           
            lineTwoLayout->addSpacing(5);
 
            _destinationStack = new QStackedWidget(lineTwo);  // FogBugz #4388
            {
               QWidget * destWidgets = new QWidget;
               {
                  QBoxLayout * destLayout = NewHBoxLayout(destWidgets, 0, 5);

                  _showDestDialog = AddButton(tr("&Destination Folder:"), SLOT(ShowDestDialog()), destWidgets, destLayout);

                  _outputFolderPath = new QLineEdit(destWidgets);
                  connect(_outputFolderPath, SIGNAL(textChanged(const QString &)), this, SLOT(UpdateDestinationPathStatus()));
                  connect(_outputFolderPath, SIGNAL(editingFinished()), this, SLOT(UpdateDestinationPathStatus()));
                  destLayout->addWidget(_outputFolderPath, 10);

                  _createLCSDiskButton = AddButton(tr("&Create Virtual Drive File"), SLOT(ShowCreateLCSDiskDialog()), destWidgets, destLayout);

                  _confirmOverwrites = new QCheckBox(tr("Confirm &Overwrites"), destWidgets);
                  connect(_confirmOverwrites, SIGNAL(stateChanged(int)), this, SLOT(UpdateConfirmationState()));
                  destLayout->addWidget(_confirmOverwrites);
               }
               _destinationStack->addWidget(destWidgets);

               QLabel * warningLabel = new QLabel(tr("WARNING:  CONVERT IN PLACE MODE ACTIVE!"));
               {
                  QFont f = warningLabel->font();
                  f.setBold(true);
                  warningLabel->setFont(f);
                  warningLabel->setAlignment(AlignCenter);
                  warningLabel->setAutoFillBackground(true);
                  QPalette p = warningLabel->palette();
                  p.setColor(QPalette::Window,   black);
                  p.setColor(QPalette::WindowText, red);
                  warningLabel->setPalette(p);
               }
               _destinationStack->addWidget(warningLabel);
            }
            lineTwoLayout->addWidget(_destinationStack, 10);
         }
         settingsGroupLayout->addWidget(lineTwo);
      }
      mainLayout->addWidget(settingsGroup);

      _processList = new AudioMoveTreeWidget(cw);
      connect(_processList, SIGNAL(FilesDropped(const QStringList &)), this, SLOT(AddFiles(const QStringList &)));
      connect(_processList, SIGNAL(itemSelectionChanged()), this, SLOT(UpdateButtonsAsync()));
      connect(_processList->horizontalScrollBar(), SIGNAL(valueChanged(int)), _processList->viewport(), SLOT(update()));
      mainLayout->addWidget(_processList, 10);

      QFrame * buttonsGroup = new QFrame(cw);
      {
         QBoxLayout * buttonsLayout = NewHBoxLayout(buttonsGroup, 3, 3);

         buttonsLayout->addStretch();

         _pause = new QCheckBox(tr("&Pause Processing"), buttonsGroup);
         connect(_pause, SIGNAL(stateChanged(int)), this, SLOT(TogglePaused()));
         buttonsLayout->addWidget(_pause);

         buttonsLayout->addStretch();
         _showAddFilesDialog   = AddButton(tr("&Add Files..."),   SLOT(ShowAddFilesDialog()),   buttonsGroup, buttonsLayout);

         buttonsLayout->addStretch();
         _showAddFoldersDialog = AddButton(tr("Add &Folder..."), SLOT(ShowAddFoldersDialog()), buttonsGroup, buttonsLayout);

         buttonsLayout->addStretch();
         _removeSelected = AddButton(tr("Remove &Selected Conversions"), SLOT(RemoveSelected()), buttonsGroup, buttonsLayout);
         buttonsLayout->addStretch();
         _removeComplete = AddButton(tr("&Remove Completed/Aborted Conversions"), SLOT(RemoveComplete()), buttonsGroup, buttonsLayout);
         buttonsLayout->addStretch();
      }
      mainLayout->addWidget(buttonsGroup);
   }

   // load initial settings
   {
      String dest  = LocalFromQ(QDir::home().absolutePath());
      String addFilesDir, lcsDiskFile;
      int32 format = AUDIO_FORMAT_SOURCE;
      int32 rate   = AUDIO_RATE_48000;
      int32 width  = AUDIO_WIDTH_FLOAT;
      int32 maxp   = 1;
      int32 cq     = DEFAULT_AUDIO_CONVERSION_QUALITY;
      bool split   = false;
      bool ipc     = false;
      bool confirm = true;

      Message settingsMsg;
      if (LoadMessageFromRegistry("audiomove", settingsMsg) == B_NO_ERROR)
      {
         (void) settingsMsg.FindString("amw_dest",    dest);
         (void) settingsMsg.FindInt32("amw_tformat", &format);
         (void) settingsMsg.FindInt32("amw_trate",   &rate);
         (void) settingsMsg.FindInt32("amw_twidth",  &width);
         (void) settingsMsg.FindInt32("amw_qual",    &cq);
         (void) settingsMsg.FindInt32("amw_maxp",    &maxp);
         (void) settingsMsg.FindBool("amw_split",    &split);
         (void) settingsMsg.FindBool("amw_ipc",      &ipc);
         (void) settingsMsg.FindBool("amw_cow",      &confirm);
         if (settingsMsg.FindString("amw_afd", addFilesDir) == B_NO_ERROR) _addFilesDir = LocalToQ(addFilesDir());
         if (settingsMsg.FindString("amw_lcf", lcsDiskFile) == B_NO_ERROR) _lcsDiskFile = LocalToQ(lcsDiskFile());
         (void) RestoreWindowPositionFromArchive(this, settingsMsg);
      }

      // Restore file-view state
      {
         QHeaderView * h = _processList->header();

         int32 cw;
         for (int32 i=0; settingsMsg.FindInt32("amw_colw", i, &cw) == B_NO_ERROR; i++) if (cw > 0) h->resizeSection(i, cw);

         Hashtable<int32, int32> desiredOrder;
         int32 ci;
         for (int32 i=0; settingsMsg.FindInt32("amw_coli", i, &ci) == B_NO_ERROR; i++) desiredOrder.Put(desiredOrder.GetNumItems(), ci);
         desiredOrder.SortByValue();

         for (HashtableIterator<int32,int32> iter(desiredOrder, HTIT_FLAG_BACKWARDS); iter.HasData(); iter++) h->moveSection(h->visualIndex(iter.GetKey()), 0);

         bool hidden;
         for (int32 i=0; settingsMsg.FindBool("amw_colh", i, &hidden) == B_NO_ERROR; i++) _processList->SetSectionHidden(i, hidden);

         int32 sortColumn = -1;                     (void) settingsMsg.FindInt32("amw_sc", &sortColumn);
         int32 sortOrder  = (int32) AscendingOrder; (void) settingsMsg.FindInt32("amw_so", &sortOrder);
         _processList->sortItems(sortColumn, (SortOrder) sortOrder);
      }

      SetDestination(dest);
      SetTargetFormat(format);
      SetTargetSampleRate(rate);
      SetTargetSampleWidth(width);
      SetConversionQuality(cq);
      SetMaxProcesses(maxp);
      SetSplitMultiTrackFiles(split);
      SetConfirmOverwrites(confirm);
      SetInPlaceConversions(ipc);
   }

   // Allow us to override our settings from the command line 
   {
      String temp;

      if ((args.FindString("quit", temp) == B_NO_ERROR)||(args.FindString("autoquit", temp) == B_NO_ERROR))
      {
         _quitOnIdle = ParseBool(temp, true);
         if (temp.ToLowerCase() == "force") _forceQuit = _quitOnIdle = true;
      }
      if (((args.FindString("pause", temp) == B_NO_ERROR)||(args.FindString("paused", temp) == B_NO_ERROR))&&(temp.HasChars())) _pause->setChecked(ParseBool(temp, true));
      if ((args.FindString("dir", temp) == B_NO_ERROR)||(args.FindString("dest", temp) == B_NO_ERROR)||(args.FindString("destdir", temp) == B_NO_ERROR)) SetDestination(temp);
      if ((args.FindString("format", temp) == B_NO_ERROR)||(args.FindString("destformat", temp) == B_NO_ERROR)) SetTargetFormat(GetFileFormatByName(ToQ(temp()), AUDIO_FORMAT_SOURCE));
      if ((args.FindString("rate", temp) == B_NO_ERROR)||(args.FindString("destrate", temp) == B_NO_ERROR)) SetTargetSampleRate(GetSampleRateByName(ToQ(temp()), AUDIO_RATE_SOURCE));
      if ((args.FindString("width", temp) == B_NO_ERROR)||(args.FindString("destwidth", temp) == B_NO_ERROR)) SetTargetSampleWidth(GetSampleWidthByName(ToQ(temp()), AUDIO_WIDTH_SOURCE));
      if ((args.FindString("threads", temp) == B_NO_ERROR)||(args.FindString("maxthreads", temp) == B_NO_ERROR)) SetMaxProcesses(muscleClamp(atoi(temp()), 1, 9));
      if ((args.FindString("split", temp) == B_NO_ERROR)||(args.FindString("splitmulti", temp) == B_NO_ERROR)||(args.FindString("splitfiles", temp) == B_NO_ERROR)||(args.FindString("splitmultifiles", temp) == B_NO_ERROR)||(args.FindString("splitmultitrackfiles", temp) == B_NO_ERROR)) SetSplitMultiTrackFiles(ParseBool(temp, true));
      if ((args.FindString("inplace", temp) == B_NO_ERROR)||(args.FindString("convertinplace", temp) == B_NO_ERROR)) SetInPlaceConversions(ParseBool(temp, true));
      if ((args.FindString("confirm", temp) == B_NO_ERROR)||(args.FindString("confirmoverwrite", temp) == B_NO_ERROR)||(args.FindString("confirmoverwrites", temp)==B_NO_ERROR)) SetConfirmOverwrites(ParseBool(temp, true));
      if ((args.FindString("qual", temp) == B_NO_ERROR)||(args.FindString("quality", temp) == B_NO_ERROR)||(args.FindString("conversionquality", temp) == B_NO_ERROR)) SetConversionQuality(GetConversionQualityByName(ToQ(temp()), DEFAULT_AUDIO_CONVERSION_QUALITY));

      if (args.HasName("disable_bwf"))
      {
         _isReadBroadcastInfoEnabled = false;
         printf("disable_bwf keyword specified; no broadcast WAV info will be read or written.\n");
      }

      for (uint32 i=0; args.FindString("file", i, temp) == B_NO_ERROR; i++) AddFile(LocalToQ(temp()));
   }

   QTimer::singleShot(1, this, SLOT(UpdateButtons()));  // do this after event loop starts, in case we need to close() ASAP

   connect(&_updateOutputPathTimer, SIGNAL(timeout()), this, SLOT(UpdateDestinationPathStatus()));
   _updateOutputPathTimer.start(5000);   // we'll check every 5 seconds to see if the output path is still valid
}

void AudioMoveWindow :: ConvertInPlaceToggled()
{
   Message dontask;
   if ((GetInPlaceConversions())&&(LoadMessageFromRegistry("cip_dontask", dontask) != B_NO_ERROR))
   {
      AudioMoveMessageBox * amb = new AudioMoveMessageBox(true, this, SLOT(ConvertInPlaceWarningOptionSelected(int)), tr("Convert in Place Warning"), tr("When running in Convert-in-Place mode, AudioMove will overwrite your input files with the converted output files.\n\nOnce a Convert-in-Place operation has completed, it cannot be undone.\n\nAre you sure you want to use Convert-in-Place mode?"), QMessageBox::Warning, QMessageBox::YesToAll, QMessageBox::Yes, QMessageBox::No, this);  // FogBugz #4381
      amb->setWindowModality(WindowModal);
      amb->setDefaultButton(dynamic_cast<QPushButton *>(amb->button(QMessageBox::No)));
      amb->setEscapeButton(amb->button(QMessageBox::No));
      amb->setButtonText(QMessageBox::YesToAll, tr("Yes, and don't show this again"));
      amb->Go(); 
   }
}

void AudioMoveWindow :: ConvertInPlaceWarningOptionSelected(int button)
{
   switch(button)
   {
      case QMessageBox::YesToAll:
         (void) SaveMessageToRegistry("cip_dontask", Message());
      break;

      case QMessageBox::No:
         SetInPlaceConversions(false);
      break;

      default:
         // do nothing
      break;
   }
}

AudioMoveWindow :: ~AudioMoveWindow()
{
   delete _confirmationDialog;

   if (_audioSetupThread)
   {
      _audioSetupThread->ShutdownInternalThread();
      delete _audioSetupThread;
   }

   // Do this now, before everything else goes away, to avoid shutdown-ordering problems
   for (int i=_processList->topLevelItemCount()-1; i>=0; i--) static_cast<AudioMoveItem *>(_processList->topLevelItem(i))->ShutdownInternalThreads();

   Message settingsMsg; (void) SaveWindowPositionToArchive(this, settingsMsg);

   // Save file-view state
   {
      const QHeaderView * h = _processList->header();
      for (int i=0; i<h->count(); i++) 
      {
         settingsMsg.AddInt32("amw_colw", h->sectionSize(i));
         settingsMsg.AddInt32("amw_coli", h->visualIndex(i));
         settingsMsg.AddBool("amw_colh", h->isSectionHidden(i));
      }

      int sc = _processList->sortColumn();
      if (sc >= 0) settingsMsg.AddInt32("amw_sc", sc);

      settingsMsg.AddInt32("amw_so", h->sortIndicatorOrder());
   }

   settingsMsg.AddString("amw_dest",   GetDestination());
   settingsMsg.AddInt32("amw_tformat", GetTargetFormat());
   settingsMsg.AddInt32("amw_trate",   GetTargetSampleRate());
   settingsMsg.AddInt32("amw_twidth",  GetTargetSampleWidth());
   settingsMsg.AddInt32("amw_qual",    GetConversionQuality());
   settingsMsg.AddInt32("amw_maxp",    GetMaxProcesses());
   settingsMsg.AddBool("amw_split",    GetSplitMultiTrackFiles());
   settingsMsg.AddString("amw_afd",    LocalFromQ(_addFilesDir));
   settingsMsg.AddString("amw_lcf",    LocalFromQ(_lcsDiskFile));
   settingsMsg.AddBool("amw_ipc",      GetInPlaceConversions());
   settingsMsg.AddBool("amw_cow",      GetConfirmOverwrites());

   (void) SaveMessageToRegistry("audiomove", settingsMsg);
}

QComboBox * AudioMoveWindow :: CreateSettingsComboBox(const QString & label, QWidget * parent, QBoxLayout * layout)
{
   QComboBox * ret;
   QWidget * subWidget = new QWidget(parent);
   {
      QBoxLayout * subLayout = NewHBoxLayout(subWidget, 0, 3);

      subLayout->addStretch(1);

      QLabel * lab = new QLabel(label, subWidget);
      subLayout->addWidget(lab);

      ret = new QComboBox(subWidget);
      connect(ret, SIGNAL(currentIndexChanged(int)), this, SLOT(ScheduleUpdateComboBoxBackgrounds()));
      lab->setBuddy(ret);
      subLayout->addWidget(ret);
      subLayout->addStretch(1);
   }
   layout->addWidget(subWidget); 
   return ret;
}

void AudioMoveWindow :: ScheduleUpdateComboBoxBackgrounds()
{
   if (_updateComboBoxBackgroundsPending == false)
   {
      _updateComboBoxBackgroundsPending = true;
      QTimer::singleShot(0, this, SLOT(UpdateComboBoxBackgrounds()));
   }
}

static QColor GetColorForErrorLevel(int sev)
{
   QColor c;
   switch(sev)
   {
      case MUSCLE_LOG_NONE:           c = QColor(255,200,255); break;
      case MUSCLE_LOG_CRITICALERROR:  c = QColor(255,100,100); break;
      case MUSCLE_LOG_ERROR:          c = QColor(255,200,200); break;
      case MUSCLE_LOG_WARNING:        c = QColor(255,255,0);   break;
      case MUSCLE_LOG_DEBUG:          c = QColor(200,255,200); break;
      case MUSCLE_LOG_TRACE:          c = QColor(220,255,255); break;
      default:                        c = white;               break;
   }
   return c;
}

static void UpdateComboBoxBackground(QComboBox * b, int errorLevel)
{
   if (errorLevel != MUSCLE_LOG_INFO)
   {
      QPalette p = b->palette();
      p.setColor(b->backgroundRole(), GetColorForErrorLevel(errorLevel));
      b->setAutoFillBackground(true);
      b->setPalette(p);   
   }
   else 
   {
      b->setPalette(QPalette());
      b->setAutoFillBackground(false);
   }
}

bool AudioMoveWindow :: IsTargetSampleRateSupported() const
{
   const uint32 sr = GetTargetSampleRate();
   if (sr == AUDIO_RATE_SOURCE) return true;  // we don't know what it is, so we can't flag it

   switch(GetTargetFormat())
   {
      case AUDIO_FORMAT_OGGOPUS:
         switch(sr)
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

bool AudioMoveWindow :: IsTargetSampleWidthSupported() const
{
   const uint32 sw = GetTargetSampleWidth();
   if (sw == AUDIO_WIDTH_SOURCE) return true;  // we don't know what it is, so we can't flag it

   switch(GetTargetFormat())
   {
      case AUDIO_FORMAT_WAV: case AUDIO_FORMAT_WAV64: case AUDIO_FORMAT_RF64:
         switch(sw)
         {
            case AUDIO_WIDTH_INT8:
               return false;

            default:
               // empty
            break;
         }
      break;
            
      case AUDIO_FORMAT_FLAC: case AUDIO_FORMAT_PAF_LE: case AUDIO_FORMAT_PAF_BE:
         switch(sw)
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

void AudioMoveWindow :: UpdateComboBoxBackgrounds()
{
   _updateComboBoxBackgroundsPending = false;
   UpdateComboBoxBackground(_targetSampleRate,  IsTargetSampleRateSupported()?MUSCLE_LOG_INFO:MUSCLE_LOG_CRITICALERROR);
   UpdateComboBoxBackground(_targetSampleWidth, IsTargetSampleWidthSupported()?MUSCLE_LOG_INFO:MUSCLE_LOG_CRITICALERROR);
}

QAbstractButton * AudioMoveWindow :: AddButton(const QString & label, const char * slot, QWidget * parent, QBoxLayout * layout)
{
   QAbstractButton * ret = new QToolButton(parent);
   ret->setText(label);
   connect(ret, SIGNAL(clicked()), this, slot);
   layout->addWidget(ret);
   return ret;
}

uint32 AudioMoveWindow :: GetNumActiveTransfers(bool selectedOnly, uint32 * optRetNumSel, uint32 * optRetErrs) const
{
   if (optRetNumSel) *optRetNumSel = 0;
   if (optRetErrs)   *optRetErrs   = 0;

   // See we have any non-completed, non-error'd events
   uint32 numActive = 0;
   for (HashtableIterator<uint32, AudioMoveItem *> iter(_moveItems); iter.HasData(); iter++)
   {
      AudioMoveItem * next = iter.GetValue();
      if ((optRetNumSel)&&(next->isSelected())) (*optRetNumSel)++;
      switch(next->GetStatus())
      { 
         case MOVE_STATUS_ERROR:
            if (optRetErrs) (*optRetErrs)++;
         break;

         case MOVE_STATUS_COMPLETE:
            // do nothing
         break;

         default:
            if ((selectedOnly == false)||(next->isSelected())) numActive++;
         break;
      }
   }
   return numActive;
}

void AudioMoveWindow :: closeEvent(QCloseEvent * e)
{
   uint32 numActive = GetNumActiveTransfers(false, NULL, NULL);
   if ((numActive > 0)&&(QMessageBox::warning(this, tr("Incomplete Conversions Warning"), tr("There are %1 conversions still in progress.  Are you sure you want to quit now?").arg(numActive), QMessageBox::Yes, QMessageBox::Cancel|QMessageBox::Escape) == QMessageBox::Cancel))
   {
      e->ignore();
      return;
   }
   QMainWindow::closeEvent(e);
}

void AudioMoveWindow :: SetDestination(const String & d)
{
   _outputFolderPath->setText(LocalToQ(d()));
}

String AudioMoveWindow :: GetDestination() const
{
   return LocalFromQ(_outputFolderPath->text());
}

void AudioMoveWindow :: SetTargetFormat(uint32 format)
{
   _targetFormat->setCurrentIndex(format);
}

uint32 AudioMoveWindow :: GetTargetFormat() const
{
   return _targetFormat->currentIndex();
}

QString AudioMoveWindow :: GetFileDurationName(int64 frames, uint32 samplesPerSecond) const
{
   if (samplesPerSecond > 0)
   {
      //uint64 numSamples = frames%samplesPerSecond;
      uint64 numSeconds = frames/samplesPerSecond;
      uint64 numMinutes = numSeconds/60;  numSeconds = numSeconds%60;
      uint64 numHours   = numMinutes/60;  numMinutes = numMinutes%60;
      char buf[256]; sprintf(buf, UINT64_FORMAT_SPEC"h %02" UINT64_FORMAT_SPEC_NOPERCENT"m %02" UINT64_FORMAT_SPEC_NOPERCENT"s", numHours, numMinutes, numSeconds);
      const char * s = buf;
      while((*s != '\0')&&(muscleInRange(*s, '1', '9') == false)) s++;
      return (*s ? ToQ(s) : ToQ("--"));
   }
   return ToQ("");
}

QString AudioMoveWindow :: GetFileFormatName(uint32 format) const
{
   switch(format)
   {
      case AUDIO_FORMAT_SOURCE:     return tr("Source");
      case AUDIO_FORMAT_WAV:        return ToQ(".WAV");
      case AUDIO_FORMAT_AIFF:       return ToQ(".AIFF");
      case AUDIO_FORMAT_FLAC:       return ToQ(".FLAC");
      case AUDIO_FORMAT_OGGVORBIS:  return ToQ(".OGG");
      case AUDIO_FORMAT_PAF_BE:     return ToQ(".PAF (B.E.)");
      case AUDIO_FORMAT_PAF_LE:     return ToQ(".PAF (L.E.)");
      case AUDIO_FORMAT_WAV64:      return ToQ(".W64");
      case AUDIO_FORMAT_CAF:        return ToQ(".CAF");
      case AUDIO_FORMAT_RF64:       return ToQ(".RF64");
      case AUDIO_FORMAT_OGGOPUS:    return ToQ(".OPUS");
      case AUDIO_FORMAT_NORMALIZED: return ToQ("");
      default:                      return tr("Unknown");
   }
}

uint32 AudioMoveWindow :: GetFileFormatByName(const QString & name, uint32 defaultFormat) const
{
   for (uint32 i=0; i<NUM_AUDIO_FORMATS; i++) if (GetFileFormatName(i).contains(name, CaseInsensitive)) return i;
   return defaultFormat;
}

void AudioMoveWindow :: SetTargetSampleRate(uint32 rate)
{
   _targetSampleRate->setCurrentIndex(rate);
}

uint32 AudioMoveWindow :: GetTargetSampleRate() const
{
   return _targetSampleRate->currentIndex();
}

QString AudioMoveWindow :: GetSampleRateName(uint32 rate, bool inTable) const
{
   uint32 code = GetSampleRateCodeFromValue(rate);
   if (code != NUM_AUDIO_RATES) rate = code;

   switch(rate)
   {
      case AUDIO_RATE_SOURCE: return inTable ? ToQ("") : tr("Source");
      case AUDIO_RATE_192000: return tr("192kHz");
      case AUDIO_RATE_176400: return tr("176.4kHz");
      case AUDIO_RATE_96000:  return tr("96kHz");
      case AUDIO_RATE_88200:  return tr("88.2kHz");
      case AUDIO_RATE_48000:  return tr("48kHz");
      case AUDIO_RATE_44100:  return tr("44.1kHz");
      case AUDIO_RATE_24000:  return tr("24kHz");
      case AUDIO_RATE_22050:  return tr("22.05kHz");
      case AUDIO_RATE_16000:  return tr("16kHz");
      case AUDIO_RATE_12000:  return tr("12kHz");
      case AUDIO_RATE_11025:  return tr("11.025kHz");
      case AUDIO_RATE_8000:   return tr("8kHz");
      default:                return tr("%1KHz").arg((uint32)(rate/1000), (int)0, (char)'g', (int)2, QLatin1Char(' '));
   }
}

uint32 AudioMoveWindow :: GetSampleRateByName(const QString & name, uint32 defaultRate) const
{
   for (uint32 i=0; i<NUM_AUDIO_RATES; i++) if (GetSampleRateName(i, false).contains(name, CaseInsensitive)) return i;
   return defaultRate;
}


void AudioMoveWindow :: SetTargetSampleWidth(uint32 width)
{
   _targetSampleWidth->setCurrentIndex(width);
}

uint32 AudioMoveWindow :: GetTargetSampleWidth() const
{
   return _targetSampleWidth->currentIndex();
}

QString AudioMoveWindow :: GetSampleWidthName(uint32 width, bool inTable) const
{
   switch(width)
   {
      case AUDIO_WIDTH_SOURCE: return inTable ? ToQ("") : tr("Source");
      case AUDIO_WIDTH_FLOAT:  return tr("32-bit Float");  // FogBugz #3668
      case AUDIO_WIDTH_INT32:  return tr("32-bit Fixed");
      case AUDIO_WIDTH_INT24:  return tr("24-bit Fixed");
      case AUDIO_WIDTH_INT16:  return tr("16-bit Fixed");
      case AUDIO_WIDTH_INT8:   return tr("8-bit Fixed");
      case AUDIO_WIDTH_DOUBLE: return tr("64-bit Double");
      default:                 return tr("Unknown");
   }
}

uint32 AudioMoveWindow :: GetSampleWidthByName(const QString & name, uint32 defaultWidth) const
{
   for (uint32 i=0; i<NUM_AUDIO_WIDTHS; i++) if (GetSampleWidthName(i, false).contains(name, CaseInsensitive)) return i;
   return defaultWidth;
}

void AudioMoveWindow :: SetConversionQuality(uint32 qual)
{
   _conversionQuality->setCurrentIndex(qual);
}

uint32 AudioMoveWindow :: GetConversionQuality() const
{
   return _conversionQuality->currentIndex();
}

QString AudioMoveWindow :: GetConversionQualityName(uint32 qual) const
{
   switch(qual)
   {
      case AUDIO_CONVERSION_QUALITY_BEST:   return tr("Best");
      case AUDIO_CONVERSION_QUALITY_BETTER: return tr("Better");
      case AUDIO_CONVERSION_QUALITY_GOOD:   return tr("Good");
      default:                              return tr("Unknown");
   }
}

uint32 AudioMoveWindow :: GetConversionQualityByName(const QString & name, uint32 defaultQual) const
{
   for (uint32 i=0; i<NUM_AUDIO_CONVERSION_QUALITIES; i++) if (GetConversionQualityName(i).contains(name, CaseInsensitive)) return i;
   return defaultQual;
}

QString AudioMoveWindow :: GetStatusName(uint32 status) const
{
   switch(status)
   {
      case MOVE_STATUS_CONFIRMING: return tr("Awaiting Confirmation");
      case MOVE_STATUS_WAITING:    return tr("Waiting");
      case MOVE_STATUS_PROCESSING: return _pause->isChecked() ? tr("Paused") : tr("Processing");
      case MOVE_STATUS_COMPLETE:   return tr("Complete");
      case MOVE_STATUS_ERROR:      return tr("Error");
      default:                     return tr("Unknown");
   }
}

void AudioMoveWindow :: MaxSimultaneousChanged()
{
   // First, go through and set all active processes to the "waiting" state...
   for (HashtableIterator<uint32, AudioMoveItem *> iter(_moveItems); iter.HasData(); iter++) iter.GetValue()->Halt();

   // Then Dequeue as many processes as can now fit
   DequeueTransfers();
   ForceUpdateAll();
}

void AudioMoveWindow :: SetMaxProcesses(uint32 maxp)
{
   _maxSimultaneous->setCurrentIndex(maxp-1);
}

uint32 AudioMoveWindow :: GetMaxProcesses() const
{
   return _maxSimultaneous->currentIndex()+1;
}

void AudioMoveWindow :: SetSplitMultiTrackFiles(bool split)
{
   _splitMultiTrackFiles->setChecked(split);
}

bool AudioMoveWindow :: GetSplitMultiTrackFiles() const
{
   return _splitMultiTrackFiles->isChecked();
}

void AudioMoveWindow :: SetInPlaceConversions(bool ipc)
{
   _inPlaceConversions->setChecked(ipc);
}

bool AudioMoveWindow :: GetInPlaceConversions() const
{
   return _inPlaceConversions->isChecked();
}

void AudioMoveWindow :: SetConfirmOverwrites(bool confirm)
{
   _confirmOverwrites->setChecked(confirm);
}

bool AudioMoveWindow :: GetConfirmOverwrites() const
{
   return _confirmOverwrites->isChecked();
}

void AudioMoveWindow :: ShowAddFilesDialog()
{
   if (_addFiles == NULL) 
   {
      _addFiles = new AudioMoveFileDialog("audiofiles", QString(), QFileDialog::ExistingFiles, this);
      connect(_addFiles, SIGNAL(FilesSelected(const QStringList &)), this, SLOT(DialogAddFiles(const QStringList &)));
   }
   _addFiles->selectFile(LocalToQ(""));
   _addFiles->setDirectory(_addFilesDir);
   _addFiles->RereadDir();
   _addFiles->show();
}

void AudioMoveWindow :: ShowAddFoldersDialog()
{
   if (_addFolders == NULL) 
   {
      _addFolders = new AudioMoveFileDialog("audiofolders", QString(), QFileDialog::DirectoryOnly, this);
      connect(_addFolders, SIGNAL(FilesSelected(const QStringList &)), this, SLOT(DialogAddFolder(const QStringList &)));
   }
   _addFolders->selectFile(LocalToQ(""));
   _addFolders->setDirectory(_addFilesDir);
   _addFolders->RereadDir();
   _addFolders->show();
}

void AudioMoveWindow :: DialogAddFiles(const QStringList & files)
{
   _addFilesDir = _addFiles->directory().absolutePath();
   AddFiles(files);
}

void AudioMoveWindow :: DialogAddFolder(const QStringList & files)
{
   _addFilesDir = _addFolders->directory().absolutePath();
   AddFiles(files);
}

void AudioMoveWindow :: AddFiles(const QStringList & files)
{
   int numFiles = files.size();
   for (int i=0; i<numFiles; i++) AddFile(files[i]);
}

void AudioMoveWindow :: AddFile(const QString & qqfn)
{
   QString qfn(qqfn);
#ifdef WIN32  
   // this cleanup shouldn't be necessary, but it is
   qfn.replace('\\', '/');
#endif

   String fn = LocalFromQ(qfn);
   MessageRef setupMsg = GetMessageFromPool(SETUP_COMMAND_SETUP);
   if ((setupMsg())&&
       (setupMsg()->AddString(SETUP_NAME_SOURCEFILE, LocalFromQ(qfn))           == B_NO_ERROR)&&
       ((GetInPlaceConversions())||(setupMsg()->AddString(SETUP_NAME_DESTDIR, GetDestination()) == B_NO_ERROR))&&
       (setupMsg()->AddInt32(SETUP_NAME_TARGETFORMAT, GetTargetFormat())        == B_NO_ERROR)&&
       (setupMsg()->AddInt32(SETUP_NAME_TARGETRATE, GetTargetSampleRate())      == B_NO_ERROR)&&
       (setupMsg()->AddInt32(SETUP_NAME_TARGETWIDTH, GetTargetSampleWidth())    == B_NO_ERROR)&&
       (setupMsg()->AddInt32(SETUP_NAME_QUALITY, GetConversionQuality())        == B_NO_ERROR)&&
       (setupMsg()->AddBool(SETUP_NAME_SPLITFILES, GetSplitMultiTrackFiles())   == B_NO_ERROR))
   {
      // demand-allocate the setup thread
      if (_audioSetupThread == NULL)
      {
         _audioSetupThread = new AudioSetupThread(this);
         if (_audioSetupThread->StartInternalThread() != B_NO_ERROR)
         {
            LogTime(MUSCLE_LOG_ERROR, "Couldn't start audio setup thread!\n");
            delete _audioSetupThread;
            _audioSetupThread = NULL;
         }
      }
      if ((_audioSetupThread)&&(_audioSetupThread->SendMessageToInternalThread(setupMsg) == B_NO_ERROR)) _numInitializing++;
   }
}


void AudioMoveWindow :: DequeueTransfers()
{
   if (_pause->isChecked() == false)
   {
      uint32 numActive    = 0;
      uint32 maxNumActive = GetMaxProcesses();
      for (HashtableIterator<uint32, AudioMoveItem *> iter(_moveItems); ((numActive<maxNumActive)&&(iter.HasData())); iter++)
      {
         AudioMoveItem * next = iter.GetValue();
         bool update = false;
         switch(next->GetStatus())
         {
            case MOVE_STATUS_WAITING: 
               update = true;
               (void) next->StartInternalThreads();  // only has an effect the first time
            // fall thru!
            case MOVE_STATUS_PROCESSING:
               if (next->SendBuffers(true) == B_NO_ERROR) numActive++;
               if (update) next->Update(true);
            break;
         }
      }
   }
}

void AudioMoveWindow :: ShowCreateLCSDiskDialog()
{
   String dest = GetDestination();
   while((dest.EndsWith("/"))||(dest.EndsWith("\\"))) dest = dest.Substring(0, dest.Length()-1);
   if ((dest.EndsWith("wtrxaudio"))||(QMessageBox::warning(this, tr("Virtual Drive File Creation Warning"), tr("Virtual Drive (.lcsDisk) files are typically created from directories named wtrxaudio,\nso that Wild Tracks relative file paths will be computed correctly.\n\nYou have requested to make a .lcsDisk file by scanning the directory\n\n%1\n\nAre you sure you want to do that?").arg(ToQ(dest())), QMessageBox::Yes, QMessageBox::Cancel|QMessageBox::Escape) == QMessageBox::Yes))
   {
      if (_createLCSDisk == NULL) 
      {
         _createLCSDisk = new AudioMoveFileDialog("diskfiles", LocalToQ("LCS Disk Files (*.lcsDisk)"), QFileDialog::AnyFile, this);
         _createLCSDisk->setAcceptMode(QFileDialog::AcceptSave);
         connect(_createLCSDisk, SIGNAL(FilesSelected(const QStringList &)), this, SLOT(CreateLCSDisk(const QStringList &)));
      }

      QString defName = _lcsDiskFile;
      {
         String d = FromQ(defName);
         if (d.Substring("/").StartsWith("wtrx-") == false)
         {
            int32 lastSlash = d.LastIndexOf('/');

            // We want to strongly encourage file names of the form wtrx-X-scsi-Y.lcsDisk, because
            // those are the only names that the daemons will try to read.  So if the user doesn't
            // have such a name already, we'll replace it with the default file name.
            if (lastSlash >= 0) defName = LocalToQ((d.Substring(0, lastSlash+1) + DEFAULT_LCSDISK_NAME)());
                           else defName = LocalToQ(DEFAULT_LCSDISK_NAME);
         }
      }
      
      _createLCSDisk->selectFile(defName);
      _createLCSDisk->show();
   }
}

void AudioMoveWindow :: ReshowLCSDiskDialog()
{
   if (_createLCSDisk) _createLCSDisk->show();
}

void AudioMoveWindow :: CreateLCSDisk(const QStringList & f)
{
   if (f.size() > 0)
   {
      QString fileName(f[0]);

      const QString extension = LocalToQ(".lcsDisk");
      if (fileName.endsWith(extension) == false) fileName += extension;

      _lcsDiskFile = fileName;
      if (_createLCSDisk) _createLCSDisk->hide();
      if (CreateVDiskFile(GetDestination()(), LocalFromQ(_lcsDiskFile), this) != B_NO_ERROR) QMessageBox::critical(this, tr(".lcsDisk File Save Error"), tr("There was an error saving file\n\n%1\n\nto disk.").arg(_lcsDiskFile));
   }
}

void AudioMoveWindow :: DestinationDirSelected(const QStringList & sl)
{
   if (sl.size() > 0) _outputFolderPath->setText(sl[0]);
}

void AudioMoveWindow :: ShowDestDialog()
{
   if (_chooseDestDir == NULL) 
   {
      _chooseDestDir = new AudioMoveFileDialog("dest", QString(), QFileDialog::DirectoryOnly, this);
      connect(_chooseDestDir, SIGNAL(FilesSelected(const QStringList &)), this, SLOT(DestinationDirSelected(const QStringList &)));
   }
   _chooseDestDir->selectFile(LocalToQ(""));
   _chooseDestDir->setDirectory(_outputFolderPath->text());
   _chooseDestDir->show();
}

void AudioMoveWindow :: TogglePaused()
{
   DequeueTransfers();
   ForceUpdateAll();
}

void AudioMoveWindow :: ForceUpdateAll()
{
   // Force update of all active items, so that "Paused" will become "Processing" or vice versa
   for (HashtableIterator<uint32, AudioMoveItem *> iter(_moveItems); iter.HasData(); iter++) iter.GetValue()->Update(true);
}

void AudioMoveWindow :: DeleteAudioMoveItem(uint32 nextKey, AudioMoveItem * next)
{
   _moveItems.Remove(nextKey);
   (void) _pendingConfirmations.Remove(next);
   delete next;
}

void AudioMoveWindow :: RemoveComplete()
{
   for (HashtableIterator<uint32, AudioMoveItem *> iter(_moveItems); iter.HasData(); iter++)
   {
      AudioMoveItem * next = iter.GetValue();
      switch(next->GetStatus())
      {
         case MOVE_STATUS_ERROR: case MOVE_STATUS_COMPLETE:
            DeleteAudioMoveItem(iter.GetKey(), next);
         break;
      }
   }
   UpdateConfirmationState();
   UpdateButtons();
}

void AudioMoveWindow :: RemoveSelected()
{
   uint32 numSel;
   uint32 numActive = GetNumActiveTransfers(true, &numSel, NULL);
   if ((numActive == 0)||(QMessageBox::warning(this, tr("Incomplete Conversions Warning"), tr("%1 of the %2 selected conversions are still in progress.  Are you sure you want to remove them now?").arg(numActive).arg(numSel), QMessageBox::Yes, QMessageBox::No|QMessageBox::Escape) == QMessageBox::Yes))
   {
      for (HashtableIterator<uint32, AudioMoveItem *> iter(_moveItems); iter.HasData(); iter++) if (iter.GetValue()->isSelected()) DeleteAudioMoveItem(iter.GetKey(), iter.GetValue());
      UpdateConfirmationState();
      UpdateButtons();
      DequeueTransfers();
   }
}

void AudioMoveWindow :: UpdateButtons()
{
   uint32 numSel, numErrs;
   uint32 numActive = GetNumActiveTransfers(false, &numSel, &numErrs);
   _removeSelected->setEnabled(numSel > 0);
   _removeComplete->setEnabled(numActive < _moveItems.GetNumItems());
   _destinationStack->setCurrentIndex(GetInPlaceConversions()?1:0);
   if ((_numInitializing == 0)&&(_quitOnIdle)&&(numActive == 0))
   {
      // do not combine these if statements!
      if (_forceQuit) close();
      else
      {
         if (numErrs == 0) close();
                      else _quitOnIdle = false;
      }
   }
   _targetSampleWidth->setEnabled((GetTargetFormat() != AUDIO_FORMAT_OGGVORBIS)&&(GetTargetFormat() != AUDIO_FORMAT_OGGOPUS));  // Ogg doesn't care about sample widths?
}

void AudioMoveWindow :: UpdateDestinationPathStatus()
{
   String s = FromQ(_outputFolderPath->text());
   if (s.EndsWith("/") == false) s += '/';
   if (EnsureFileFolderExists(s+"dummy", false) == B_NO_ERROR) _outputFolderPath->setPalette(QPalette());
   else 
   {
      QPalette p = _outputFolderPath->palette();
      p.setColor(_outputFolderPath->backgroundRole(), QColor(255,200,200));
      _outputFolderPath->setPalette(p);
   }
}

status_t AudioMoveWindow :: MessageReceivedFromUpstream(const MessageRef & msg)
{
   // We can't handle it here, since we are in an output thread... so we'll
   // post it asynchronously back to the main GUI thread, instead!
   QApplication::postEvent(this, new BufferReturnedEvent(msg, false));
   return B_NO_ERROR;
}

bool AudioMoveWindow :: event(QEvent * evt)
{
   switch(evt->type())
   {
      case BUFFER_RETURNED_EVENT_TYPE:
      {
         const BufferReturnedEvent * bre = dynamic_cast<BufferReturnedEvent *>(evt);
         if (bre) 
         {
            MessageRef msg = bre->GetMessage();
            if (msg())
            {
               if (bre->IsFromSetup())
               {
                  _numInitializing--;

                  MessageRef subMsg;
                  for (int32 i=0; msg()->FindMessage(SETUP_NAME_SETUPRESULT, i, subMsg) == B_NO_ERROR; i++)
                  {
                     RefCountableRef itag, otag;
                     AudioMoveThreadRef inputThread, outputThread;
                     uint32 quality;
                     if ((subMsg()->FindTag(SETUP_NAME_INPUTTHREAD, itag) == B_NO_ERROR)&&(subMsg()->FindTag(SETUP_NAME_OUTPUTTHREAD, otag) == B_NO_ERROR)&&(subMsg()->FindInt32(SETUP_NAME_QUALITY, (int32*)&quality) == B_NO_ERROR)&&(inputThread.SetFromRefCountableRef(itag) == B_NO_ERROR)&&(outputThread.SetFromRefCountableRef(otag) == B_NO_ERROR))
                     {
                        AudioMoveItem * ami = new AudioMoveItem(++_tagCounter, this, _processList, inputThread, AudioMoveThreadRef(new SampleRateThread(GetSampleRateValueFromCode(inputThread()->GetOutputFileSampleRate()), GetSampleRateValueFromCode(outputThread()->GetInputFileSampleRate()), quality, inputThread()->GetFileStreams())), outputThread);
                        if (_moveItems.Put(ami->GetTag(), ami) == B_NO_ERROR)
                        {
                           String errorString; (void) subMsg()->FindString(SETUP_NAME_ERROR, errorString);
                           if (errorString.HasChars()) 
                           {
                              ami->SetStatus(MOVE_STATUS_ERROR);
                              ami->SetStatusString(ToQ(errorString()));
                           }
                           else if ((GetConfirmOverwrites())&&(subMsg()->HasName(SETUP_NAME_FILESTOBEOVERWRITTEN))&&(_pendingConfirmations.Put(ami, true) == B_NO_ERROR)) 
                           {
                              QString filesStr;
                              const char * next;
                              for (int32 i=0; subMsg()->FindString(SETUP_NAME_FILESTOBEOVERWRITTEN, i, &next) == B_NO_ERROR; i++)
                              {
                                 if (filesStr.length() > 0) filesStr += '\n';
                                 filesStr += LocalToQ(next);
                              }
                              ami->SetOverwriteFilesString(filesStr);
                              UpdateConfirmationState();
                           }
                           else ami->SetStatus(MOVE_STATUS_WAITING);

                           ami->Update(true);
                        }
                     }
                  }
                  DequeueTransfers();
               }
               else
               {
                  AudioMoveItem * item;
                  if (_moveItems.Get(msg()->what, item) == B_NO_ERROR)
                  {
                     item->MessageReceivedFromUpstream(msg);
                     if (item->GetStatus() != MOVE_STATUS_PROCESSING) DequeueTransfers();
                  }
               }
            }
         }
      }
      return true;

      default:
         return QMainWindow::event(evt);
   }
}

AudioMoveConfirmationDialog :: AudioMoveConfirmationDialog(AudioMoveWindow * win) : QDialog(win, Dialog|WindowStaysOnTopHint|WindowTitleHint|WindowSystemMenuHint), _win(win)
{
   setWindowTitle(tr("File Overwrite Confirmation"));

   QBoxLayout * vbl = NewVBoxLayout(this, 5);
   vbl->addStretch();

   QLabel * lab = new QLabel(tr("Is it okay for AudioMove to overwrite:"), this);
   lab->setAlignment(AlignCenter);
   vbl->addWidget(lab);
   vbl->addStretch();

   _fileText = new QLabel(this);
   _fileText->setAlignment(AlignCenter);
   QPalette p = _fileText->palette();
   p.setColor(QPalette::Window, white);
   _fileText->setPalette(p);
   _fileText->setAutoFillBackground(true);
   vbl->addWidget(_fileText);
   vbl->addStretch();

   _moreToGo = new QLabel(this);
   _moreToGo->setAlignment(AlignCenter);
   vbl->addWidget(_moreToGo);
   vbl->addStretch();

   QWidget * buttons = new QWidget(this);
   {
      QBoxLayout * buttonsLayout = NewHBoxLayout(buttons, 0, 5);
      CreateButton(tr("Yes"),        SLOT(Yes()),      buttons, buttonsLayout)->setDefault(true);
      CreateButton(tr("Yes to All"), SLOT(YesToAll()), buttons, buttonsLayout);
      CreateButton(tr("No"),         SLOT(No()),       buttons, buttonsLayout);
      CreateButton(tr("No to All"),  SLOT(NoToAll()),  buttons, buttonsLayout);
   }
   vbl->addWidget(buttons);

   connect(this, SIGNAL(accepted()), SLOT(HandleAccepted()));
   connect(this, SIGNAL(rejected()), SLOT(HandleRejected()));
}

QPushButton * AudioMoveConfirmationDialog :: CreateButton(const QString & label, const char * theSlot, QWidget * buttons, QBoxLayout * buttonsLayout)
{
   QPushButton * btn = new QPushButton(label, buttons);
   btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
   connect(btn, SIGNAL(clicked()), this, theSlot);
   buttonsLayout->addWidget(btn);
   return btn;
}

void AudioMoveConfirmationDialog :: UpdateStatus()
{
   const Hashtable<AudioMoveItem *, bool> & t = _win->_pendingConfirmations;
   if (t.HasItems())
   {
      _fileText->setText((*t.GetFirstKey())->GetOverwriteFilesString());
      if (t.GetNumItems() > 1) _moreToGo->setText(QString("(%1 more confirmations to go)").arg(t.GetNumItems()-1));
                          else _moreToGo->setText(ToQ(""));
   }
   else
   {
      // this should never happen, but just in case...
      _moreToGo->setText(ToQ(""));
      _fileText->setText(ToQ(""));
   }
}

void AudioMoveConfirmationDialog :: DoConfirmationResult(bool isYes, bool isToAll)
{
   _win->DoConfirmationResult(isYes, isToAll);
}

void AudioMoveWindow :: FinalizePendingConfirmations(uint32 newState, const QString * optErrStr)
{
   for (HashtableIterator<AudioMoveItem *, bool> iter(_pendingConfirmations); iter.HasData(); iter++) 
   {
      iter.GetKey()->SetStatus(newState);
      if (optErrStr) iter.GetKey()->SetStatusString(*optErrStr);
   }
   _pendingConfirmations.Clear();
   DequeueTransfers();
}

void AudioMoveWindow :: ShowConfirmationDialog()
{
   ShowDialog(_confirmationDialog);
}

void AudioMoveWindow :: UpdateConfirmationState()
{
   if ((_pendingConfirmations.HasItems())&&(GetConfirmOverwrites() == false)) FinalizePendingConfirmations(MOVE_STATUS_WAITING, NULL);
   if (_pendingConfirmations.HasItems()) 
   {
      if (_confirmationDialog == NULL) _confirmationDialog = new AudioMoveConfirmationDialog(this);
      _confirmationDialog->UpdateStatus();
      if (_confirmationDialog->isHidden()) QTimer::singleShot(0, this, SLOT(ShowConfirmationDialog()));  // gotta be async because we might be called during the close event
   }
   else if (_confirmationDialog) _confirmationDialog->hide();

   DequeueTransfers();
}

void AudioMoveWindow :: DoConfirmationResult(bool isYes, bool isToAll)
{
   static const QString errStr = tr("Cancelled");
   
   uint32 newStatus = isYes?MOVE_STATUS_WAITING:MOVE_STATUS_ERROR;

        if (isToAll) FinalizePendingConfirmations(newStatus, isYes?NULL:&errStr);
   else if (_pendingConfirmations.HasItems())
   {
      AudioMoveItem * ami = *(_pendingConfirmations.GetFirstKey());
      ami->SetStatus(newStatus);
      if (!isYes) ami->SetStatusString(errStr);
      (void) _pendingConfirmations.Remove(*_pendingConfirmations.GetFirstKey());
   }

   UpdateConfirmationState();
}

uint32 GetSampleRateValueFromCode(uint32 rateCode)
{
   switch(rateCode)
   {
      case AUDIO_RATE_192000: return 192000;
      case AUDIO_RATE_176400: return 176400;
      case AUDIO_RATE_96000:  return 96000;
      case AUDIO_RATE_88200:  return 88200;
      case AUDIO_RATE_48000:  return 48000;
      case AUDIO_RATE_44100:  return 44100;
      case AUDIO_RATE_24000:  return 24000;
      case AUDIO_RATE_22050:  return 22050;
      case AUDIO_RATE_16000:  return 16000;
      case AUDIO_RATE_12000:  return 12000;
      case AUDIO_RATE_11025:  return 11025;
      case AUDIO_RATE_8000:   return 8000;
      default:                return rateCode;
   }
}

uint32 GetSampleRateCodeFromValue(uint32 value)
{
   switch(value)
   {
      case 192000: return AUDIO_RATE_192000;
      case 176400: return AUDIO_RATE_176400;
      case 96000:  return AUDIO_RATE_96000;
      case 88200:  return AUDIO_RATE_88200;
      case 48000:  return AUDIO_RATE_48000;
      case 44100:  return AUDIO_RATE_44100;
      case 24000:  return AUDIO_RATE_24000;
      case 22050:  return AUDIO_RATE_22050;
      case 16000:  return AUDIO_RATE_16000;
      case 12000:  return AUDIO_RATE_12000;
      case 11025:  return AUDIO_RATE_11025;
      case 8000:   return AUDIO_RATE_8000;
      default:     return NUM_AUDIO_RATES;
   }
}

void AudioMoveWindow :: SetupIcon()
{
static const char *appicon[] = {
/* width height ncolors chars_per_pixel */
"16 16 149 2",
/* colors */
"   c #000000",
" . c #FFD54B",
" X c #C7592F",
" o c #971200",
" O c #FDCB49",
" + c #C9683E",
" @ c #FFC14B",
" # c #C13712",
" $ c #191315",
" % c #FBB747",
" & c #647972",
" * c #A82403",
" = c #59665A",
" - c #956528",
" ; c #F5AC4E",
" : c #412932",
" > c #2C3331",
" , c #FFCC47",
" < c #514E52",
" 1 c #F29E4B",
" 2 c #D04C20",
" 3 c #FBBE43",
" 4 c #8C9885",
" 5 c #5F6A66",
" 6 c #C09D37",
" 7 c #9A6429",
" 8 c #C74724",
" 9 c #DE5634",
" 0 c #CC461F",
" q c #7E6E60",
" w c #D3734A",
" e c #B25E44",
" r c #F5B34A",
" t c #150E10",
" y c #D88752",
" u c #F19C50",
" i c #BE4F32",
" p c #EFC147",
" a c #F6AD4E",
" s c #A27158",
" d c #FFCB46",
" f c #8F999B",
" g c #663527",
" h c #0A0608",
" j c #FCC343",
" k c #FFD349",
" l c #FFD149",
" z c #FECF48",
" x c #E9984E",
" c c #EEBE42",
" v c #160D10",
" b c #B32E0C",
" n c #E9943A",
" m c #6C8178",
" M c #322228",
" N c #443333",
" B c #DA8335",
" V c #EB9349",
" C c #CA451F",
" Z c #C5D1B8",
" A c #F2A153",
" S c #25161B",
" D c #BE3713",
" F c #B06947",
" G c #EA974B",
" H c #D7431B",
" J c #CA662F",
" K c #DC582D",
" L c #412B33",
" P c #DE9249",
" I c #FECE47",
" U c #D48253",
" Y c #F3A04D",
" T c #FDCA46",
" R c #FCC845",
" E c #7C8D7D",
" W c #DB7646",
" Q c #B7C2A3",
" ! c #B3BC9F",
" ~ c #A15C45",
" ^ c #E5BA42",
" / c #5D6A65",
" ( c #F3A050",
" ) c #EA985B",
" _ c #EF9742",
" ` c #B54727",
" ' c #A4B1B1",
" ] c #C8A439",
" [ c #DE4621",
" { c #DE8F48",
" } c #402831",
" | c #E64D22",
".  c #FECD46",
".. c #FDC745",
".X c #FBBE4D",
".o c #515B55",
".O c #983417",
".+ c #FED149",
".@ c #C46542",
".# c #1E1519",
".$ c #E8964E",
".% c #C74824",
".& c #9EAD9D",
".* c #DC934C",
".= c #CB3A14",
".- c #DE8444",
".; c #E36728",
".: c #E67342",
".> c #576257",
"., c #6A6A6D",
".< c #F8B746",
".1 c #827B6A",
".2 c #1F1B20",
".3 c #FFCA46",
".4 c #CC653C",
".5 c None",
".6 c #FABF4B",
".7 c #CEB079",
".8 c #2D2427",
".9 c #1B0E12",
".0 c #FED048",
".q c #FECE48",
".w c #C69B3C",
".e c #38232B",
".r c #F9B74D",
".t c #876C29",
".y c #BF3F1B",
".u c #ED9B55",
".i c #B8C4A8",
".p c #2A1B20",
".a c #EA9D55",
".s c #B23311",
".d c #FBA555",
".f c #C3421B",
".g c #D3733B",
".h c #CB5330",
".j c #8F9D9D",
".k c #FECD47",
".l c #38222A",
".z c #DBB43F",
".x c #B53917",
".c c #CB6933",
".v c #352027",
".b c #7A8A7B",
".n c #B37039",
".m c #FBBF44",
".M c #7F8576",
".N c #E2562C",
".B c None",
/* pixels */
".5.5.5.5.5.5.0 O.6 O.+.0.5.5.5.5",
".5.5.5.5.5 % a Y 1 3 j ,.q z z z",
".i.5.5 Z !.a G u ; I.. c R T.k z",
" Q 4 /.5.7 A P.$.r.  k 6.z d T.5",
".5 E m.5.d (.* x.X T . ^ ].3 l.5",
".5.5 q e.: {.n V r @.w.t p.m.; [",
".5.5 ~ i 2.g.c J B n 7 -.< _ 9.N",
".5.5.5.1 0.f X + U y.-.u ) W K.5",
".5.5.5 & s C D.s `.@ w.4.x.= |.5",
".5.5.5 5.b.M.h H # * o b.% 8  .5",
".5.5.5.5.o >   |.O.y F g N.l  .5",
".5.5.5.5             =.>.2 : L.5",
".5.5.5.5             $.& f M }.5",
".5.5.5.5            .#.j '.8.v.5",
".5.5.5.5             h <., S.e.5",
".5.5.5.5               v.9.p t.5"
};

   setWindowIcon(QPixmap((const char **) appicon));
}

};  // end namespace audiomove
