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

#ifndef AudioMoveWindow_h
#define AudioMoveWindow_h

#include <QDialog>
#include <QTreeWidget>
#include <QMainWindow>
#include <QTimer>
#include <QCloseEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QTimer>

#include "audiomove/AudioMoveNameSpace.h"
#include "audiomove/AudioMoveThread.h"
#include "util/String.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QAbstractButton;
class QBoxLayout;
class QStackedWidget;

namespace audiomove {

class AudioMoveThread;
class AudioMoveWindow;
class AudioSetupThread;
class AudioMoveFileDialog;
class AudioMovePopupMenu;
class AudioMoveTreeItemDelegate;

enum {
   MOVE_STATUS_CONFIRMING = 0,  // waiting for the user to okay a file overwrite
   MOVE_STATUS_WAITING,         // waiting for CPU time to become available
   MOVE_STATUS_PROCESSING,      // converting audio
   MOVE_STATUS_COMPLETE,        // done converting audio
   MOVE_STATUS_ERROR,           // audio conversion aborted due to error
   NUM_MOVE_STATUSES
};

class AudioMoveItem;

class AudioMoveTreeWidget : public QTreeWidget
{
Q_OBJECT

public:
   AudioMoveTreeWidget(QWidget * parent);

   virtual void dragEnterEvent(QDragEnterEvent * e);
   virtual void dragMoveEvent(QDragMoveEvent * e) {e->acceptProposedAction();}
   virtual void dropEvent(QDropEvent * e);
   virtual void resizeEvent(QResizeEvent * e);

   virtual bool eventFilter(QObject * o, QEvent * e);

   void UpdateRow(AudioMoveItem * ami);
   void SetSectionHidden(int which, bool hidden);

signals:
   void FilesDropped(const QStringList & list);

private slots:
   void DisplayMenuItemChosen(int, int, int menuItemID);

private:
   friend class AudioMoveItem;
   friend class AudioMoveTreeItemDelegate;

   void UpdateDisplayMenu();
   void GetDisplayMenuInfo(bool & canResetOrdering, int & visCount) const;
   float GetProcessingPercentageForRow(int row, QColor & retBackgroundColor) const;
   QString GetMoveColumnName(uint32 col) const;

   AudioMovePopupMenu * _displayMenu;
};

/** Objects of this class represent a transfer -- both the UI object and the implementation. */
class AudioMoveItem : public QTreeWidgetItem, public IMessageAcceptor
{
public:
   AudioMoveItem(uint32 tag, AudioMoveWindow * owner, QTreeWidget * lv, const AudioMoveThreadRef & input, const AudioMoveThreadRef & convert, const AudioMoveThreadRef & output);
   virtual ~AudioMoveItem();

   status_t StartInternalThreads();
   void ShutdownInternalThreads();

   /** Updates all our cells appropriately. */
   void Update(bool updateAllColumns);

   float GetPercentDone() const {return (_totalSamples > 0) ? muscleClamp((float)(((double)(int64)_samplesComplete)/((double)(int64)_totalSamples)), 0.0f, 1.0f) : ((_status == MOVE_STATUS_COMPLETE)?1.0f:0.0f);}
   uint32 GetStatus() const {return _status;}

   uint32 GetTag() const {return _tag;}

   /* If we are currently in processing mode, this will put us back to waiting mode */
   void Halt();

   /** Sends out buffers until are maximum number of buffers are in play */
   status_t SendBuffers(bool allowChangeStatus);

   /** Called by the window object, in the main thread, when a buffer has completed the circuit. */
   virtual status_t MessageReceivedFromUpstream(const MessageRef & msg);

   void SetStatusString(const QString & ss);
   void SetStatus(uint32 s);

   virtual bool operator < (const QTreeWidgetItem & other) const;

   QColor GetColorForStatus(uint32 status) const;

   void SetOverwriteFilesString(const QString & s) {_overwriteFilesString = s;}
   const QString & GetOverwriteFilesString() const {return _overwriteFilesString;}

private:
   enum {
      STAGE_SOURCE = 0,
      STAGE_CONVERSION,
      STAGE_DESTINATION,
      NUM_STAGES
   };

   const uint32 _tag;
   bool _internalThreadsStarted;
   QString _statusString;
   uint32 _status;
   uint64 _samplesLeft;
   uint64 _samplesComplete;
   uint64 _totalSamples;
   uint32 _numActiveBuffers;
   AudioMoveWindow * _owner;
   AudioMoveThreadRef _threads[NUM_STAGES];
   uint64 _lastUpdateTime;
   QString _overwriteFilesString;
   bool _isFirstUpdate;
};

class AudioMoveConfirmationDialog : public QDialog
{
Q_OBJECT

public:
   AudioMoveConfirmationDialog(AudioMoveWindow * win);

   void UpdateStatus();

private slots:
   void Yes()      {DoConfirmationResult(true,   false);}
   void YesToAll() {DoConfirmationResult(true,   true );}
   void No()       {DoConfirmationResult(false,  false);}
   void NoToAll()  {DoConfirmationResult(false, true );}
   void HandleAccepted() {Yes();}
   void HandleRejected() {No();}

private:
   QPushButton * CreateButton(const QString & label, const char * theSlot, QWidget * buttons, QBoxLayout * buttonsLayout);
   void DoConfirmationResult(bool isYes, bool isToAll);

   AudioMoveWindow * _win;
   QLabel * _fileText;
   QLabel * _moreToGo;
};

/** Window class for the AudioMove GUI.  */
class AudioMoveWindow : public QMainWindow, public IMessageAcceptor
{
Q_OBJECT

public:
   /** Standard constructor; all parameters are passed through to the QMainWindow ctor. **/
   AudioMoveWindow(const Message & args, QWidget * parent = NULL, WindowFlags f = WindowFlags());

   /** Standard destructor */
   virtual ~AudioMoveWindow();

   /** Overridden so we can ask the user if he is sure he wants to quit while transfers are running. */
   virtual void closeEvent(QCloseEvent * e);

   /** Called whenever some data has finished processing.  NOTE:  called from various threads!! */
   virtual status_t MessageReceivedFromUpstream(const MessageRef & msg);

   /** Overridden to handle incoming BufferReturnedEvents */
   virtual bool event(QEvent * evt);

private slots:
   void ShowAddFilesDialog();
   void ShowAddFoldersDialog();
   void ShowDestDialog();
   void TogglePaused();
   void RemoveSelected();
   void RemoveComplete();
   void AddFiles(const QStringList & files);
   void AddFile(const QString & file);
   void DialogAddFiles(const QStringList & files);
   void DialogAddFolder(const QStringList & files);
   void UpdateButtons();
   void UpdateButtonsAsync() {QTimer::singleShot(0, this, SLOT(UpdateButtons()));}
   void MaxSimultaneousChanged();
   void UpdateDestinationPathStatus();
   void DestinationDirSelected(const QStringList &);
   void ConvertInPlaceToggled();
   void ConvertInPlaceWarningOptionSelected(int button);
   void UpdateConfirmationState();
   void ShowConfirmationDialog();
   void ScheduleUpdateComboBoxBackgrounds();
   void UpdateComboBoxBackgrounds();

private:
   friend class AudioMoveItem;
   friend class AudioMoveConfirmationDialog;

   QComboBox * CreateSettingsComboBox(const QString & label, QWidget * parent, QBoxLayout * layout);
   void RemoveItems(bool selOnly);

   void SetDestination(const String & d);
   String GetDestination() const;

   void SetTargetFormat(uint32 format);
   uint32 GetTargetFormat() const;

   QString GetFileFormatName(uint32 format) const;
   uint32 GetFileFormatByName(const QString & name, uint32 defaultFormat) const;
   const char * GetAudioFormatExtensions(uint32 format) const;
   String ReplaceExtension(const String & ss, const String & ext);

   void SetTargetSampleRate(uint32 rate);
   uint32 GetTargetSampleRate() const;
   QString GetSampleRateName(uint32 rate, bool inTable) const;
   uint32 GetSampleRateByName(const QString & name, uint32 defaultRate) const;

   void SetTargetSampleWidth(uint32 width);
   uint32 GetTargetSampleWidth() const;
   QString GetSampleWidthName(uint32 width, bool inTable) const;
   uint32 GetSampleWidthByName(const QString & name, uint32 defaultWidth) const;

   void SetConversionQuality(uint32 qual);
   uint32 GetConversionQuality() const;
   QString GetConversionQualityName(uint32 width) const;
   uint32 GetConversionQualityByName(const QString & name, uint32 defaultQuality) const;

   void SetMaxProcesses(uint32 maxp);
   uint32 GetMaxProcesses() const;

   void SetSplitMultiTrackFiles(bool split);
   bool GetSplitMultiTrackFiles() const;

   void SetConfirmOverwrites(bool confirm);
   bool GetConfirmOverwrites() const;

   void SetInPlaceConversions(bool ipc);
   bool GetInPlaceConversions() const;

   QString GetFileDurationName(int64 frames, uint32 samplesPerSecond) const;
   QString GetStatusName(uint32 status) const;

   QAbstractButton * AddButton(const QString & label, const char * slot, QWidget * parent, QBoxLayout * layout);

   void DequeueTransfers();
   void ForceUpdateAll();

   uint32 GetNumActiveTransfers(bool selectedOnly, uint32 * optRetNumSel, uint32 * optRetNumErrs) const;
   void SetupIcon();

   void DoConfirmationResult(bool isYes, bool isToAll);
   void FinalizePendingConfirmations(uint32 newState, const QString * optErrStr);
   void DeleteAudioMoveItem(uint32 nextKey, AudioMoveItem * next);

   bool IsTargetSampleRateSupported() const;
   bool IsTargetSampleWidthSupported() const;

   QTimer _updateOutputPathTimer;
   AudioMoveTreeWidget * _processList;
   QStackedWidget * _destinationStack;
   QAbstractButton * _showAddFilesDialog;
   QAbstractButton * _showAddFoldersDialog;
   QAbstractButton * _showDestDialog;
   QLineEdit   * _outputFolderPath;
   QComboBox   * _targetFormat;
   QComboBox   * _targetSampleRate;
   QComboBox   * _targetSampleWidth;
   QComboBox   * _conversionQuality;
   QComboBox   * _maxSimultaneous;
   QCheckBox   * _splitMultiTrackFiles;
   QCheckBox   * _inPlaceConversions;
   QCheckBox   * _confirmOverwrites;

   QCheckBox   * _pause;
   QAbstractButton * _removeSelected;
   QAbstractButton * _removeComplete;

   AudioMoveFileDialog * _chooseDestDir;
   AudioMoveFileDialog * _addFiles;
   AudioMoveFileDialog * _addFolders;
   QString _addFilesDir;
   bool _quitOnIdle;
   bool _forceQuit;
   uint32 _numInitializing;

   uint32 _tagCounter;
   Hashtable<uint32, AudioMoveItem *> _moveItems;

   Hashtable<AudioMoveItem *, bool> _pendingConfirmations;
   AudioMoveConfirmationDialog * _confirmationDialog;

   AudioSetupThread * _audioSetupThread;
   bool _updateComboBoxBackgroundsPending;
};

};  // end namespace audiomove

#endif
