/* This file is Copyright 2002 Level Control Systems.  See the included LICENSE.txt file for details. */

#ifndef AudioMoveFileDialog_h
#define AudioMoveFileDialog_h

#include <QFileDialog>
#include <QPushButton>
#include <QShowEvent>
#include <QEvent>

#include "audiomove/AudioMoveNameSpace.h"
#include "message/Message.h"
#include "util/NestCount.h"

class QComboBox;
class QTreeWidget;
class QTreeWidgetItem;

namespace audiomove {

class _HackTreeWidget;

/** This class contains LCS-specific GUI enhancements in it.  
  * AudioMove code should always use this class instead of the regular QFileDialog class.
  */
class AudioMoveFileDialog : public QFileDialog
{
Q_OBJECT

public:
   /**
    *  @param fileCategory An arbitrary string identifying the type of file this dialog is used for.
    *                      This string is used when choosing a default folder to open up in.
    *  @param filter Filter string to use.  This string is passed to QFileDialog::setFilterList().
    *  @param mode Dialog mode to use.  This value is passed to QFileDialog::setMode().
    *  @param parent Parent widget to use.  This value is passed to the QFileDialog constructor.
    */
   AudioMoveFileDialog(const String & fileCategory, const QString & filter, QFileDialog::FileMode mode, QWidget * parent);

   /**
    *  @param fileCategory An arbitrary string identifying the type of file this dialog is used for.
    *                      This string is used when choosing a default folder to open up in.
    *  @param filter A list of Filter strings to use.  This list is passed to QFileDialog::setFilterList().
    *  @param mode Dialog mode to use.  This value is passed to QFileDialog::setMode().
    *  @param parent Parent widget to use.  This value is passed to the QFileDialog constructor.
    */
   AudioMoveFileDialog(const String & fileCategory, const QStringList & filters, QFileDialog::FileMode mode, QWidget * parent);

   virtual ~AudioMoveFileDialog();

   virtual void showEvent(QShowEvent * e);
   virtual void hideEvent(QHideEvent * e);

   virtual bool eventFilter(QObject * watched, QEvent * e);

   /** Set the suffix to make sure is at the end of all file names before adding them to the convenience list. */
   void SetAutoSuffix(const String & s) {_autoSuffix = s;}

   /** Adds the specified widget to the bottom of the dialog.
     * @param w The widget to add.  Should be a child of (this).
     */
   void AddBottomWidget(QWidget * w, Alignment alignment = 0);

   /** Adds the specified widget to the right-hand side of the dialog.
     * @param w The widget to add.  Should be a child of (this).
     */
   void AddRightWidget(QWidget * w, Alignment alignment = 0);

   /** Sets the specified file (or file path) to be our sole current selection. */
   void SetSelection(const QString & filePath);

   virtual QSize sizeHint() const;

public slots:
   /** Similar to QFileDialog::rereadDir(), except we also re-scan the registry
     * and make sure we are in the folder specified therein.
     */
   void RereadDir();

signals:
   /** Emitted if the user cancels the dialog without selecting anything */
   void Cancelled();

   /** Emitted whenever the currently shown directory changes */
   void CurrentDirectoryChanged(const QDir & dir);

   /** Emitted whenever the set of selected files changes */
   void SelectedFilesChanged();

   /** This signal is emitted when files have been selected and the file-overwrite
     * confirmation process has been completed (if applicable).
     * connect this signal instead of filesSelected() to your code's slots when
     * when using this class.
     */
   void FilesSelected(const QStringList & sl);

protected:
   virtual void reject();

private slots:
   void SaveSettingsToRegistry(const QStringList &);
   void RecentFileActivated(QTreeWidgetItem * item, int col);
   void RecentFileSelectionChangedAsync();
   void RecentFileSelectionChanged();
   void HotButtonToggled();
   void HotButtonRightClicked();
   void PopupMenuResult(int, int whichButton, int menuID);
   void UpdateHotButtons();
   void HandleSelectedFilesChanged();
   void OverwriteCheckResults(int button, const muscle::MessageRef & files);
   void CheckForOverwrite(const QStringList & sl);
   void CheckForDirectoryChange();
   void CheckForDirectoryChangeAsync();

private:
   friend class _HackTreeWidget;

   void ClearFileSelections();
   void DoInit(QFileDialog::FileMode mode, const QStringList & filters);
   QTreeWidgetItem * GetSelectedRecentFileItem() const;
   Queue<String> GetSelectedFilesList() const;
   const Queue<String> & GetFilePathsForItem(const QTreeWidgetItem * item) const;
   const Queue<String> & GetFilePathsForIndex(uint32 index) const;
   int32 GetIndexForItem(const QTreeWidgetItem * item) const;
   void SaveRecentListToRegistry() const;
   void LoadRecentListFromRegistry();
   void SaveHotButtonsToRegistry() const;
   void LoadHotButtonsFromRegistry();
   status_t GetHotButtonDefaults(QString & retLab, String & retLoc, uint32 which) const;
   void ResetHotButton(uint32 which);
   QString GetDirName(const QDir & dir, const QString & defVal) const;
   void UpdateHotButton(uint32 whichButton, const QString & label, const String & filePath);
   QStringList FilePathsToQ(const Queue<String> & fp) const;
   void ReturnFileResults(const QStringList & sl);
   void DeleteItem(int32 idx);
   void UpdateRecentFilesTreeWidget();

   String _fileCategory;

   enum {NUM_HOT_BUTTONS = 5};
   QPushButton * _hotButtons[NUM_HOT_BUTTONS];
   String _hotButtonDirs[NUM_HOT_BUTTONS];

   QDir _lastDirectory;
   QStringList _lastSelectedFiles;
   QWidget * _extras;
   QTreeWidget * _recentFilesTreeWidget;
   Queue<Queue<String> > _recentFilesList;
   NestCount _recentFileDisableCount;
   NestCount _fileHighlightedDisableCount;
   String _autoSuffix;
   uint64 _lastHotkeySelectTime;
   void * _prevHotkeySelection;  // warning:  not guaranteed to be a valid pointer!
   bool _directoryChangeCheckPending;
   bool _recentFileSelectionChangedPending;
};

class AudioMoveFileDialogHotButton : public QPushButton
{
Q_OBJECT

public:
   AudioMoveFileDialogHotButton(QWidget * parent) : QPushButton(parent) {setCheckable(true);}

protected:
   virtual void mousePressEvent(QMouseEvent * e);

signals:
   void RightButtonClicked();
};

};  // end namespace audiomove

#endif
