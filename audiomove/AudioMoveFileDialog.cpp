/* This file is Copyright 2002 Level Control Systems.  See the included LICENSE.txt file for details. */

#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeySequence>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QPen>
#include <QTimer>
#include <QKeyEvent>
#include <QPixmap>
#include <QMouseEvent>
#include <QShowEvent>
#include <QTreeWidget>

#include "audiomove/AudioMoveFileDialog.h"
#include "audiomove/AudioMoveMessageBox.h"
#include "audiomove/AudioMovePopupMenu.h"
#include "audiomove/MiscFunctions.h"
#include "system/SystemInfo.h"

namespace audiomove {

static const uint32 MAX_RECENT_FILES = 5;

/* XPM */
static const char * eject_xpm[] = {
"12 9 2 1",
" 	c None",
".	c #707070",
"     ..     ",
"    ....    ",
"   ......   ",
"  ........  ",
" .......... ",
"            ",
"            ",
" .......... ",
" .......... "};

/* XPM */
static const char * eject_sel_xpm[] = {
"12 9 2 1",
" 	c None",
".	c #FFFF90",
"     ..     ",
"    ....    ",
"   ......   ",
"  ........  ",
" .......... ",
"            ",
"            ",
" .......... ",
" .......... "};

enum {
   RECENT_FILE_COLUMN_HOTKEY = 0,
   RECENT_FILE_COLUMN_FILENAME,
   RECENT_FILE_COLUMN_DELETE,
   NUM_RECENT_FILE_COLUMNS,
};

AudioMoveFileDialog :: AudioMoveFileDialog(const String & fileCategory, const QString & filter, QFileDialog::FileMode mode, QWidget * parent) : QFileDialog(parent), _fileCategory(fileCategory.Prepend("dir_")), _lastHotkeySelectTime(0), _prevHotkeySelection(NULL), _directoryChangeCheckPending(false)
{
   QStringList filters;
   if (filter != QString::null) filters.push_back(filter);
   filters.push_back(tr("Any File") + LocalToQ(" (*)"));
   DoInit(mode, filters);
}

AudioMoveFileDialog :: AudioMoveFileDialog(const String & fileCategory, const QStringList & filters, QFileDialog::FileMode mode, QWidget * parent) : QFileDialog(parent), _fileCategory(fileCategory.Prepend("dir_")), _lastHotkeySelectTime(0), _prevHotkeySelection(NULL), _directoryChangeCheckPending(false)
{
   QStringList f;
   for (int i=0; i<filters.size(); i++) f.push_back(filters[i]);
   f.push_back(tr("Any File") + LocalToQ(" (*)"));
   DoInit(mode, f);
}

AudioMoveFileDialog :: ~AudioMoveFileDialog()
{
   // empty
}

void AudioMoveFileDialog :: CheckForDirectoryChangeAsync()
{
   if (_directoryChangeCheckPending == false)
   {
      _directoryChangeCheckPending = true;
      QTimer::singleShot(0, this, SLOT(CheckForDirectoryChange()));
   }
}

class _HackTreeWidget : public QTreeWidget
{
public:
   _HackTreeWidget(AudioMoveFileDialog * owner, QWidget * parent) : QTreeWidget(parent), _owner(owner) {/* empty */}

   virtual void mousePressEvent(QMouseEvent * e)
   {
      QModelIndex mi = indexAt(e->pos());
      if (mi.column() == RECENT_FILE_COLUMN_DELETE)  // FogBugz #4712, Ellen's note #2
      {
         _owner->DeleteItem(mi.row());
         e->ignore();
      }
      else QTreeWidget::mousePressEvent(e);
   }

private:
   AudioMoveFileDialog * _owner;
};

void AudioMoveFileDialog :: DoInit(QFileDialog::FileMode mode, const QStringList & filters)
{
#if (QT_VERSION >= 0x040500)
   setOption(DontUseNativeDialog);  // FogBugz #5274 
#endif

   // FogBugz #4373:  Watch combo boxes for changes to the directory (Qt 4.3 STILL doesn't have the support we need :^P )
   {
      QList<QComboBox*> combos = qFindChildren<QComboBox*>(this);
      for (int i=0; i<combos.size(); ++i)
      {
         QComboBox * box = combos[i];
         connect(box, SIGNAL(editTextChanged(const QString &)), this, SLOT(CheckForDirectoryChangeAsync()));
         connect(box, SIGNAL(currentIndexChanged(int)), this, SLOT(CheckForDirectoryChangeAsync()));

         QLineEdit * le = box->lineEdit();
         if (le)
         {
            connect(le, SIGNAL(returnPressed()),      this, SLOT(CheckForDirectoryChangeAsync()));
            connect(le, SIGNAL(textChanged(QString)), this, SLOT(CheckForDirectoryChangeAsync()));
         }
      }
   }
   connect(this, SIGNAL(currentChanged(const QString &)), SLOT(CheckForDirectoryChangeAsync()));

   // FogBugz #4472:  We need to update our hot-selects whenever the user changes the file name
   {
      QList<QLineEdit*> ledits = qFindChildren<QLineEdit*>(this);
      for (int i=ledits.size()-1; i>=0; i--) connect(ledits[i], SIGNAL(textEdited(QString)), this, SLOT(CheckForDirectoryChangeAsync()));  // FogBugz #4472
   }

   setViewMode(Detail);  // FogBugz #4293

   // I'll assume that if the user is allowed to specify a non-existing file,
   // then we must be doing a save operation
   if (mode == QFileDialog::AnyFile) setAcceptMode(QFileDialog::AcceptSave);

   // We'll check for file-overwrites ourself, thank you very much
   // That way we can give better info, and we don't risk getting stuck in a modal requester
   setConfirmOverwrite(false);

   setFilters(filters);

   setFileMode(mode);
   connect(this, SIGNAL(filesSelected(const QStringList &)), SLOT(CheckForOverwrite(const QStringList &)));
   connect(this, SIGNAL(SelectedFilesChanged()), this, SLOT(HandleSelectedFilesChanged()));
   connect(this, SIGNAL(CurrentDirectoryChanged(const QDir &)), this, SLOT(UpdateHotButtons()));

   _extras = new QWidget(this);
   _extras->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
   {
      QBoxLayout * extrasLayout = NewVBoxLayout(_extras, 0, 5);

      QWidget * hotButtons = new QWidget(_extras);
      { 
         QBoxLayout * hbl = NewHBoxLayout(hotButtons, 0, 5);
         for (uint32 i=0; i<NUM_HOT_BUTTONS; i++)
         {
            AudioMoveFileDialogHotButton * b = new AudioMoveFileDialogHotButton(hotButtons);
            connect(b, SIGNAL(clicked()), this, SLOT(HotButtonToggled()));
            connect(b, SIGNAL(RightButtonClicked()), this, SLOT(HotButtonRightClicked()));
            _hotButtons[i] = b;
            hbl->addWidget(b);
         }
      }
      extrasLayout->addWidget(hotButtons);

      _recentFilesTreeWidget = new _HackTreeWidget(this, _extras);
      _recentFilesTreeWidget->setUniformRowHeights(true);
      _recentFilesTreeWidget->setSortingEnabled(false);
      _recentFilesTreeWidget->setColumnCount(NUM_RECENT_FILE_COLUMNS);

      QStringList sl;
      sl.push_back(tr("Hotkey"));
      sl.push_back(tr("Recent Files"));
      sl.push_back(ToQ(""));
      _recentFilesTreeWidget->setHeaderLabels(sl);

      QHeaderView * h = _recentFilesTreeWidget->header();
      h->setMovable(false);
      h->setClickable(false);
      h->setResizeMode(RECENT_FILE_COLUMN_HOTKEY,   QHeaderView::ResizeToContents);
      h->setResizeMode(RECENT_FILE_COLUMN_FILENAME, QHeaderView::Stretch);
      h->setResizeMode(RECENT_FILE_COLUMN_DELETE,   QHeaderView::ResizeToContents);
      h->setStretchLastSection(false);

      QTreeWidgetItem * temp = new QTreeWidgetItem(_recentFilesTreeWidget);  // necessary for sizeHintForRow(0) to give us a valid result
      int recentHeight = h->sizeHint().height()+(_recentFilesTreeWidget->sizeHintForRow(0)*MAX_RECENT_FILES+5);
      _recentFilesTreeWidget->setRootIsDecorated(false);
      _recentFilesTreeWidget->setFixedHeight(recentHeight);
      delete temp;

      connect(_recentFilesTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(RecentFileSelectionChangedAsync()));
      connect(_recentFilesTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(RecentFileActivated(QTreeWidgetItem *, int)));
      extrasLayout->addWidget(_recentFilesTreeWidget);
   }

   AddBottomWidget(_extras);

   Message tempMsg;
   if (LoadMessageFromRegistry(_fileCategory(), tempMsg) == B_NO_ERROR) (void) RestoreWindowPositionFromArchive(this, tempMsg, false);

   resize(sizeHint());
   RereadDir();
}

void AudioMoveFileDialog :: AddBottomWidget(QWidget * w, Alignment alignment)
{
   QGridLayout * gl = dynamic_cast<QGridLayout*>(layout());
   if (gl) 
   {
      int r = gl->rowCount();
      gl->addWidget(w, r, 0, 1, gl->columnCount(), alignment);
      gl->setRowStretch(r, 0);
   }
}

void AudioMoveFileDialog :: AddRightWidget(QWidget * w, Alignment alignment)
{
   QGridLayout * gl = dynamic_cast<QGridLayout *>(layout());
   if (gl) 
   {
      int c = gl->columnCount();
      gl->addWidget(w, 0, c, gl->rowCount(), 1, alignment);
      gl->setColumnStretch(c, 0);
   }
}

QSize AudioMoveFileDialog :: sizeHint() const
{
   QSize ret = QFileDialog::sizeHint();
   return QSize(ret.width()+200, ret.height()+_extras->sizeHint().height());
}

void AudioMoveFileDialog :: showEvent(QShowEvent * e)
{
   QFileDialog::showEvent(e);
   qApp->installEventFilter(this);
   CheckForDirectoryChange();
}

void AudioMoveFileDialog :: hideEvent(QHideEvent * e)
{
   qApp->removeEventFilter(this);
   QFileDialog::hideEvent(e);
}

void AudioMoveFileDialog::RereadDir()
{
   LoadRecentListFromRegistry();
   LoadHotButtonsFromRegistry();

   QString allPaths;
   Message temp;
   if (LoadMessageFromRegistry(_fileCategory(), temp) == B_NO_ERROR)
   {
      bool setDir = false;

      String nextPath;
      for (int32 i=0; temp.FindString("path", i, nextPath) == B_NO_ERROR; i++)
      {
         if (setDir == false)
         { 
            QFileInfo fi(LocalToQ(nextPath()));
            if (fi.exists())
            { 
               setDirectory(fi.absoluteDir());
               setDir = true;
            }
         }

         QString fn = LocalToQ(nextPath.Substring("/")());
         if (allPaths.length() == 0) allPaths = fn;
         else
         {
            if (allPaths[0] != '\"') allPaths = ToQ("\"") + allPaths + ToQ("\"");
            allPaths = allPaths + ToQ(" \"") + fn + "\"";
         }
      }
   }
   if (allPaths.length() > 0) SetSelection(allPaths);
                         else setDirectory(QDir::home());
}

void AudioMoveFileDialog :: ClearFileSelections()
{
   QTreeView * tv = findChild<QTreeView *>(ToQ("treeView"));
   if (tv) tv->clearSelection();
}

void AudioMoveFileDialog :: SaveRecentListToRegistry() const
{
   Message temp;
   for (uint32 i=0; i<_recentFilesList.GetNumItems(); i++) 
   {
      MessageRef subMsg = GetMessageFromPool();
      const Queue<String> & q = _recentFilesList[i];
      if ((subMsg())&&(q.GetNumItems() > 0))
      {
         for (uint32 j=0; j<q.GetNumItems(); j++) (void) subMsg()->AddString("recent", q[j]);
         (void) temp.AddMessage("recent", subMsg);
      }
   }
   (void) SaveMessageToRegistry((_fileCategory+"_recent")(), temp);
}

void AudioMoveFileDialog :: LoadRecentListFromRegistry()
{
   _recentFilesList.Clear();
   Message temp;
   if (LoadMessageFromRegistry((_fileCategory+"_recent")(), temp) == B_NO_ERROR)
   {
      // In the current format, each entry has its own sub-Message
      MessageRef subMsg;
      for (int32 i=0; ((_recentFilesList.GetNumItems() < MAX_RECENT_FILES)&&(temp.FindMessage("recent", i, subMsg) == B_NO_ERROR)); i++)
      {
         if (_recentFilesList.AddTail() == B_NO_ERROR)
         {
            Queue<String> & q = _recentFilesList.Tail();
            String s;
            for (int32 j=0; subMsg()->FindString("recent", j, s) == B_NO_ERROR; j++) (void) q.AddTail(s);
         }
      }
   }
   UpdateRecentFilesTreeWidget();
}

static QString GetExecutableName()
{
   if (qApp->argc() > 0)
   {
      String exeName(qApp->argv()[0]);
#ifdef __APPLE__
      exeName = exeName.Substring(0, ".app");
#endif
      return ToQ(exeName.Substring("/")());
   }
   else return qApp->translate("AudioMoveFileDialog", "Program");
}

status_t AudioMoveFileDialog :: GetHotButtonDefaults(QString & retLab, String & retLoc, uint32 which) const
{
   switch(which)
   {
      case 0:  retLab = tr("Home");       return GetSystemPath(SYSTEM_PATH_USERHOME, retLoc);
      case 1:  retLab = tr("Desktop");    return GetSystemPath(SYSTEM_PATH_DESKTOP,  retLoc);

#ifdef __APPLE__
      case 2:  retLab = tr("Documents");  return GetSystemPath(SYSTEM_PATH_DOCUMENTS, retLoc);
      case 3:  retLab = tr("Volumes");    retLoc = "/Volumes/"; return B_NO_ERROR;
      case 4:  
      {
         retLab = GetExecutableName();
         if (GetSystemPath(SYSTEM_PATH_EXECUTABLE, retLoc) == B_NO_ERROR)
         {
            // Get rid of the "AudioMove.app" silliness
            int32 appIdx = retLoc.LastIndexOf(".app");
            while((appIdx >= 0)&&(retLoc[appIdx] != '/')) appIdx--; 
            retLoc = retLoc.Substring(0, appIdx);
            return B_NO_ERROR; 
         }
         return B_ERROR;
      }
      case 5:  retLab = tr("Root");       return GetSystemPath(SYSTEM_PATH_ROOT,       retLoc);
#else
# ifdef WIN32
      case 2:  retLab = GetExecutableName(); return GetSystemPath(SYSTEM_PATH_EXECUTABLE, retLoc);
      case 3:  retLab = tr("Root");          return GetSystemPath(SYSTEM_PATH_ROOT,       retLoc);
# else
      case 2:  retLab = tr("Documents");     return GetSystemPath(SYSTEM_PATH_DOCUMENTS,  retLoc);
      case 3:  retLab = GetExecutableName(); return GetSystemPath(SYSTEM_PATH_EXECUTABLE, retLoc);
      case 4:  retLab = tr("Root");          return GetSystemPath(SYSTEM_PATH_ROOT,       retLoc);
# endif
#endif

      default: return B_ERROR;
   }
}

void AudioMoveFileDialog :: SaveHotButtonsToRegistry() const
{
   Message temp;
   for (uint32 i=0; i<NUM_HOT_BUTTONS; i++) 
   {
      temp.AddString("hotlab", FromQ(_hotButtons[i]->text()));
      temp.AddString("hotloc", _hotButtonDirs[i]);
   }
   (void) SaveMessageToRegistry((_fileCategory+"_hot")(), temp);
}

void AudioMoveFileDialog :: LoadHotButtonsFromRegistry()
{
   Message temp;
   (void) LoadMessageFromRegistry((_fileCategory+"_hot")(), temp);
   for (uint32 i=0; i<NUM_HOT_BUTTONS; i++)
   {
      String lab, loc;
      if ((temp.FindString("hotlab", i, lab) == B_NO_ERROR)&&(temp.FindString("hotloc", i, loc) == B_NO_ERROR)) UpdateHotButton(i, ToQ(lab()), loc);
                                                                                                           else ResetHotButton(i);
   }
   UpdateHotButtons();
}

void AudioMoveFileDialog :: ResetHotButton(uint32 which)
{
   QString qLab;
   String loc;
   if (GetHotButtonDefaults(qLab, loc, which) != B_NO_ERROR)
   {
      qLab = tr("Unassigned");
      loc  = "";
   }
   UpdateHotButton(which, qLab, loc);
}

void AudioMoveFileDialog :: SaveSettingsToRegistry(const QStringList & qsl)
{
   Queue<String> sl;
   for (int32 i=0; i<qsl.count(); i++) 
   {
      String s = LocalFromQ(qsl[i]);
      if (s.EndsWith(_autoSuffix) == false) s += _autoSuffix;
      sl.AddTail(s);
   }
   sl.Sort();  // keep a well-defined order for easier comparisons

   // If (sl) isn't already in our list of recent files, add it to the end.
   // We take care to not let the list grow longer than MAX_RECENT_FILES, though.
   if (_recentFilesList.IndexOf(sl) < 0)
   {
      while(_recentFilesList.GetNumItems() >= MAX_RECENT_FILES) _recentFilesList.RemoveHead();
      _recentFilesList.AddTail(sl);
   }

   UpdateRecentFilesTreeWidget();
   SaveRecentListToRegistry();
   SaveHotButtonsToRegistry();

   Message temp;
   (void) SaveWindowPositionToArchive(this, temp);
   for (uint32 i=0; i<sl.GetNumItems(); i++) temp.AddString("path", sl[i]);
   (void) SaveMessageToRegistry(_fileCategory(), temp);
}

void AudioMoveFileDialog :: reject()
{
   QFileDialog::reject();
   emit Cancelled(); 
}

const Queue<String> & AudioMoveFileDialog :: GetFilePathsForItem(const QTreeWidgetItem * item) const 
{
   return GetFilePathsForIndex(GetIndexForItem(item));
}

const Queue<String> & AudioMoveFileDialog :: GetFilePathsForIndex(uint32 index) const
{
   static const Queue<String> emptyQ;
   return (index < _recentFilesList.GetNumItems()) ? _recentFilesList[index] : emptyQ;
}

int32 AudioMoveFileDialog :: GetIndexForItem(const QTreeWidgetItem * item) const
{
   return _recentFilesTreeWidget->indexOfTopLevelItem(const_cast<QTreeWidgetItem *>(item));
}

void AudioMoveFileDialog :: RecentFileSelectionChangedAsync()
{
   if ((_recentFileDisableCount.IsInBatch() == false)&&(_recentFileSelectionChangedPending == false))
   {
      _recentFileSelectionChangedPending = true;
      QTimer::singleShot(0, this, SLOT(RecentFileSelectionChanged()));
   }
}

void AudioMoveFileDialog :: RecentFileSelectionChanged()
{
   _recentFileSelectionChangedPending = false;
   {
      Queue<String> sl;
      {
         Hashtable<String, bool> temp;
         for (int i=_recentFilesTreeWidget->topLevelItemCount()-1; i>=0; i--)
         {
            QTreeWidgetItem * twi = _recentFilesTreeWidget->topLevelItem(i);
            if (_recentFilesTreeWidget->isItemSelected(twi))
            {
               const Queue<String> & subList = GetFilePathsForItem(twi);
               for (int32 j=subList.GetNumItems()-1; j>=0; j--) temp.Put(subList[j], true); 
            }
         }
         temp.SortByKey();
         const String * next;
         for (HashtableIterator<String, bool> iter(temp); iter.HasData(); iter++) sl.AddTail(iter.GetKey());
      }

      NestCountGuard ncg(_fileHighlightedDisableCount);
      {
         ClearFileSelections();
         if (sl.GetNumItems() >= 1)
         {
            String dir = sl[0];
            int32 lastSlash = dir.LastIndexOf('/');
            if (lastSlash >= 0) dir = dir.Substring(0, lastSlash);

            String files;
            for (uint32 i=0; i<sl.GetNumItems(); i++)
            {
               String fileName = sl[i].Substring("/");
               if (files.HasChars()) files += ' ';
               if (sl.GetNumItems() > 1) fileName = fileName.Prepend("\"").Append("\"");
               files += fileName;
            }
            if (dir.HasChars())
            {
               QDir d(LocalToQ(dir()));
               if (d.exists())
               {
                  setDirectory(d);
                  SetSelection(LocalToQ(files()));
               }
            }
         }
         else ClearFileSelections();
      }
   }
   UpdateHotButtons();
}

Queue<String> AudioMoveFileDialog :: GetSelectedFilesList() const
{
   Queue<String> ret;
   QStringList qsl = selectedFiles();
   for (int32 i=0; i<qsl.count(); i++) ret.AddTail(LocalFromQ(qsl[i]));
   return ret;
}

void AudioMoveFileDialog :: CheckForDirectoryChange()
{
   _directoryChangeCheckPending = false;

   QDir curDir = directory();  // sometimes this is an expensive call!  (FogBugz #4373)
   if (curDir != _lastDirectory)
   {  
      _lastDirectory = curDir;
      emit CurrentDirectoryChanged(_lastDirectory);
   }
   
   QStringList sl = selectedFiles();
   if (sl != _lastSelectedFiles)
   {  
      _lastSelectedFiles = sl;
      emit SelectedFilesChanged();
   }

   HandleSelectedFilesChanged();
}

bool AudioMoveFileDialog :: eventFilter(QObject * watched, QEvent * e)
{
   switch(e->type())
   {
      case QEvent::KeyPress:
         if (isActiveWindow())
         {
            QKeyEvent * ke = (QKeyEvent *)e;
            int key = (Key) ke->key();
            if ((ke->modifiers() & ControlModifier)&&(muscleInRange(key, (int)Key_1, muscleClamp((int)(Key_1+MAX_RECENT_FILES), (int)Key_1, (int)Key_9))))
            {
               int which = key - Key_1;
               QTreeWidgetItem * item = _recentFilesTreeWidget->topLevelItem(which);
               if (item) 
               {
                  _recentFilesTreeWidget->clearSelection();
                  item->setSelected(true);
                  _recentFilesTreeWidget->setFocus();
                  if ((_recentFilesTreeWidget->isItemSelected(item))&&(_prevHotkeySelection == item)&&(_lastHotkeySelectTime+250000 >= GetRunTime64())) 
                  {
                     _lastHotkeySelectTime = 0;
                     _prevHotkeySelection = NULL;
                     CheckForOverwrite(FilePathsToQ(GetFilePathsForItem(item)));
                  }
                  _lastHotkeySelectTime = GetRunTime64();
                  _prevHotkeySelection = item;
               }
               ke->accept();
               return true;
            }
            else if ((key == Key_Enter)||(key == Key_Return))
            {
               QTreeWidgetItem * rfi = GetSelectedRecentFileItem();
               if (rfi)
               {
                  CheckForOverwrite(FilePathsToQ(GetFilePathsForItem(rfi)));
                  ke->accept();
                  return true;
               }
            }
         }
      break;

      default:
         // do nothing -- here to shut compiler up
      break;
   }
   return QFileDialog::eventFilter(watched, e);
}

QTreeWidgetItem * AudioMoveFileDialog :: GetSelectedRecentFileItem() const
{
   for (int i=_recentFilesTreeWidget->topLevelItemCount()-1; i>=0; i--)
   {
      QTreeWidgetItem * item = _recentFilesTreeWidget->topLevelItem(i);
      if (_recentFilesTreeWidget->isItemSelected(item)) return item;
   }
   return NULL;
}

void AudioMoveFileDialog :: CheckForOverwrite(const QStringList & sl)
{
   if (acceptMode() == QFileDialog::AcceptSave)
   {
      // Gotta see if the user is okay with overwriting these files!
      QString alreadyExists;
      QString cantWrite;
      int overwriteCount = 0;
      for (int i=0; i<sl.size(); i++)
      {
         const QString & fname = sl[i];
         QFileInfo f(fname);
         if (f.exists()) 
         {
            overwriteCount++;
            alreadyExists += ToQ("\n") + fname;
            if (f.isWritable() == false) cantWrite += ToQ("\n")+fname;
         }
      }
      if (cantWrite.length() > 0)
      {
         ShowDialog(this);  // re-show ourself, in case we got hidden
         AudioMoveMessageBox::ShowMessage(tr("Can't write to file"), tr("Unable to write to the following file(s):\n\n%1").arg(cantWrite), QMessageBox::Critical, this);
         return;
      }
      if (alreadyExists.length() > 0)
      {
         MessageRef files = GetMessageFromPool();
         for (int i=0; i<sl.size(); i++) files()->AddString("files", LocalFromQ(sl[i]));
         
         QString textStr;
         if (overwriteCount == 1) textStr = tr("The file\n%1\n\nalready exists.\n\nAre you sure you want to replace it?").arg(alreadyExists);
                             else textStr = tr("The following files already exist:\n%1\n\nAre you sure you want to replace them?").arg(alreadyExists);

         ShowDialog(this);  // re-show ourself, in case we got hidden
         (new AudioMoveMessageBox(true, this, SLOT(OverwriteCheckResults(int, const muscle::MessageRef &)), tr("File Overwrite Check"), textStr, QMessageBox::Question, QMessageBox::Ok|QMessageBox::Default, QMessageBox::Cancel|QMessageBox::Escape, QMessageBox::NoButton, this))->Go(0,0,files);
         return;
      }
   }
   ReturnFileResults(sl);
}

void AudioMoveFileDialog :: ReturnFileResults(const QStringList & sl)
{
   SaveSettingsToRegistry(sl);
   emit FilesSelected(sl);  // okay to go!
   done(0);
}

// Called when the user responds to the "are you sure you want to replace blah?" requester
void AudioMoveFileDialog :: OverwriteCheckResults(int button, const MessageRef & files)
{
   if ((button == QMessageBox::Ok)&&(files()))
   {
      QStringList sl;
      const char * next;
      for (int32 i=0; files()->FindString("files", i, &next) == B_NO_ERROR; i++) sl.push_back(LocalToQ(next));
      ReturnFileResults(sl);
   }
}


QStringList AudioMoveFileDialog :: FilePathsToQ(const Queue<String> & fp) const
{
   QStringList ret;
   for (uint32 i=0; i<fp.GetNumItems(); i++) ret.push_back(LocalToQ(fp[i]()));
   return ret;
}

void AudioMoveFileDialog :: RecentFileActivated(QTreeWidgetItem * item, int column)
{
   if (column != RECENT_FILE_COLUMN_DELETE) CheckForOverwrite(FilePathsToQ(GetFilePathsForItem(item)));
}

/** Specialized QTreeWidgetItem that can draw a red X in the delete column */
class RecentFileTreeWidgetItem : public QTreeWidgetItem
{
public:
   RecentFileTreeWidgetItem(QTreeWidget * lv, const QString & displayStr, const QKeySequence & ks, bool isReadable) : QTreeWidgetItem(lv), _isReadable(isReadable) 
   {
      static QIcon _ejectIcon(QPixmap((const char **) eject_xpm));
      static bool _firstTime = true;
      if (_firstTime)
      {
         _firstTime = false;
         _ejectIcon.addPixmap(QPixmap((const char **) eject_sel_xpm), QIcon::Selected, QIcon::Off);
      }
      setText(RECENT_FILE_COLUMN_FILENAME, displayStr);
      setText(RECENT_FILE_COLUMN_HOTKEY, (QString)ks);
      setIcon(RECENT_FILE_COLUMN_DELETE, _ejectIcon);
   }

private:
   bool _isReadable;
};

void AudioMoveFileDialog :: UpdateRecentFilesTreeWidget()
{
   _recentFilesTreeWidget->clear();
   for (uint32 i=0; i<_recentFilesList.GetNumItems(); i++) 
   {
      QString displayStr;
      const Queue<String> & q = _recentFilesList[i];
      bool isReadable = (q.GetNumItems()>0);
      for (int32 j=q.GetNumItems()-1; j>=0; j--)
      {
         if (QFileInfo(LocalToQ(q[j]())).isReadable() == false) isReadable = false;
         if (displayStr.length() > 0) displayStr += LocalToQ(", ");
         displayStr += LocalToQ(q[j].Substring("/")());
      }

      (void) new RecentFileTreeWidgetItem(_recentFilesTreeWidget, displayStr, QKeySequence((Key_1+i)|(CTRL)), isReadable);
   }
}


void AudioMoveFileDialog :: DeleteItem(int32 idx)
{
   if (_recentFilesList.RemoveItemAt(idx) == B_NO_ERROR)
   delete _recentFilesTreeWidget->topLevelItem(idx);
   SaveRecentListToRegistry();

   // FogBugz #4765
   for (int i=_recentFilesTreeWidget->topLevelItemCount()-1; i>=0; i--)
   {
      QTreeWidgetItem * twi = _recentFilesTreeWidget->topLevelItem(i);
      twi->setText(RECENT_FILE_COLUMN_HOTKEY, QKeySequence((Key_1+i)|(CTRL)).toString());
   }
}

void AudioMoveFileDialog :: HandleSelectedFilesChanged()
{
   if (_fileHighlightedDisableCount.IsInBatch() == false)
   {
      NestCountGuard ncd(_recentFileDisableCount);

      Queue<String> sl;
      for(int i=_lastSelectedFiles.size()-1; i>=0; i--) sl.AddHead(FromQ(_lastSelectedFiles[i]));
      sl.Sort();  // keep a well-defined order for easier comparisons

      // Make sure only the nth child is selected
      _recentFilesTreeWidget->clearSelection();
      int32 idx = _recentFilesList.IndexOf(sl);
      if (idx >= 0)
      {
         QTreeWidgetItem * item = _recentFilesTreeWidget->topLevelItem(idx);
         if (item) item->setSelected(true);
      }
   }
}

void AudioMoveFileDialog :: UpdateHotButtons()
{
   String dirStr = LocalFromQ(_lastDirectory.absolutePath());
   if (dirStr.EndsWith("/") == false) dirStr += '/';
   for (uint32 i=0; i<NUM_HOT_BUTTONS; i++) _hotButtons[i]->setChecked((_lastDirectory.isRelative() == false)&&(_hotButtonDirs[i] == dirStr));
}

void AudioMoveFileDialog :: HotButtonToggled()
{
   QPushButton * btn = const_cast<QPushButton *>(static_cast<const QPushButton *>(sender()));
   if (btn->isChecked())
   {
      for (uint32 i=0; i<NUM_HOT_BUTTONS; i++)
      {
         if (btn == _hotButtons[i])
         {
            if (_hotButtonDirs[i].HasChars())
            {
               QDir d(LocalToQ(_hotButtonDirs[i]()));
               if ((d.exists())&&(d.isReadable())) setDirectory(d);
                                              else AudioMoveMessageBox::ShowMessage(tr("Unknown Directory"), tr("The Directory\n\n%1\n\ndoes not exist, or is not readable.\n\nTo reassign this button to the current directory,\nright-click on the button.").arg(d.absolutePath()), QMessageBox::Critical, this);
            }
            else AudioMoveMessageBox::ShowMessage(tr("Unassigned Button"), tr("This button does not currently have a directory\nassociated with it.\n\nTo assign this button to the current directory,\nright-click on the button."), QMessageBox::Information, this);

            break;
         }
      }
   }
   UpdateHotButtons();  // handle the un-toggles too
}

enum {
   HOTBUTTON_POPUP_ASSIGN = 0,
   HOTBUTTON_POPUP_RESET,
   HOTBUTTON_POPUP_UNASSIGN
};

void AudioMoveFileDialog :: HotButtonRightClicked()
{
   QPushButton * btn = const_cast<QPushButton *>(static_cast<const QPushButton *>(sender()));
   for (uint32 i=0; i<NUM_HOT_BUTTONS; i++)
   {
      if (btn == _hotButtons[i])
      {
         QString defaultLab;
         String defaultLoc;
         if (GetHotButtonDefaults(defaultLab, defaultLoc, i) != B_NO_ERROR) defaultLab = tr("Unassigned");

         AudioMovePopupMenu * pop = new AudioMovePopupMenu(true, this, SLOT(PopupMenuResult(int, int, int)), btn);
         pop->InsertCommandItem((_hotButtonDirs[i].HasChars()?tr("Reassign Button to "):tr("Assign Button to "))+GetDirName(_lastDirectory, tr("(Unknown Folder)")), HOTBUTTON_POPUP_ASSIGN, ((_lastDirectory.exists()))&&(_lastDirectory.isRelative() == false));
         pop->addSeparator();
         pop->InsertCommandItem(tr("Reset Button to %1").arg(defaultLab), HOTBUTTON_POPUP_RESET, (_hotButtonDirs[i] != defaultLoc));
         pop->InsertCommandItem(tr("Unassign Button"), HOTBUTTON_POPUP_UNASSIGN, _hotButtonDirs[i].HasChars());

         pop->Go(0,i);
         break;
      }
   }
}

QString AudioMoveFileDialog :: GetDirName(const QDir & dir, const QString & defVal) const
{
   if (dir.exists())
   {
      QString ret = dir.dirName();
      return (ret.length()>0) ? ret : ToQ("Root");
   }
   else return defVal;
}

void AudioMoveFileDialog :: PopupMenuResult(int, int whichButton, int menuID)
{
   if ((menuID >= 0)&&(muscleInRange(whichButton, 0, NUM_HOT_BUTTONS-1)))
   {
      if ((menuID == HOTBUTTON_POPUP_ASSIGN)&&(_lastDirectory.exists()))
      {
         String label = LocalFromQ(GetDirName(_lastDirectory, ToQ("")));
         int maxWidth = (_extras->minimumSizeHint().width()/NUM_HOT_BUTTONS)-16;  // 16 for button edges, spacing, etc
         QString qlabel = LocalToQ(label());
         QFontMetrics fm = _hotButtons[whichButton]->fontMetrics();
#ifdef __linux__
         QString ellipses = ToQ("...");   // SUSE's ellipse unicode character is screwed up, so avoid it for now!
#else
         QString ellipses = QChar(0x26, 0x20);
#endif
         int labWidth = fm.width(qlabel);
         if (labWidth > maxWidth)
         {
            while((qlabel.length()>0)&&(fm.width(qlabel+ellipses) > maxWidth)) qlabel = qlabel.left(qlabel.length()-1);
            qlabel += ellipses;
         }
         UpdateHotButton(whichButton, qlabel, FromQ(_lastDirectory.absolutePath()));
      }
      else if (menuID == HOTBUTTON_POPUP_RESET) ResetHotButton(whichButton);
      else UpdateHotButton(whichButton, tr("Unassigned"), "");

      UpdateHotButtons();
      SaveHotButtonsToRegistry();
   }
}

void AudioMoveFileDialog :: UpdateHotButton(uint32 whichButton, const QString & label, const String & filePath)
{
   QPushButton * btn = _hotButtons[whichButton];
   btn->setText(label);
   if (filePath.IsEmpty()) 
   {
      QPalette p = btn->palette();
      p.setColor(btn->foregroundRole(), lightGray);
      btn->setPalette(p);
   }
   else btn->setPalette(QPalette());

   _hotButtonDirs[whichButton] = filePath;
   if (_hotButtonDirs[whichButton].EndsWith("/") == false) _hotButtonDirs[whichButton] += '/';
}

void AudioMoveFileDialogHotButton :: mousePressEvent(QMouseEvent * e)
{
   if (IsRightClickEvent(e))
   {
      e->accept();
      emit RightButtonClicked();
   }
   else QPushButton::mousePressEvent(e);
}

void AudioMoveFileDialog :: SetSelection(const QString & filePath)
{
   selectFile(filePath); 
   UpdateHotButtons();

   // Make sure the treeview is scrolled so that the current selection is visible, if possible
   QTreeView * tv = findChild<QTreeView *>(ToQ("treeView"));
   if (tv)
   {
      QModelIndexList mil = tv->selectionModel()->selection().indexes();
      if (mil.size() > 0) tv->scrollTo(mil[0], QAbstractItemView::PositionAtCenter);
   }
}

};  // end namespace audiomove
