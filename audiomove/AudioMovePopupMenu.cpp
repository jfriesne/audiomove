/* This file is Copyright 2002 Level Control Systems.  See the included LICENSE.txt file for details. */

#include <QCursor>
#include <QTimer>
#include <QMenu>

#include "audiomove/AudioMovePopupMenu.h"
#include "audiomove/MiscFunctions.h"

namespace audiomove {

AudioMovePopupMenu :: AudioMovePopupMenu(bool autoDelete, QObject * target, const char * slotName, QWidget * parent) : QMenu(parent), _goCalled(false), _autoDelete(autoDelete), _signalEmissionPending(false), _submenusAdded(false), _singleShotPending(false)
{
   if ((target)&&(slotName)) connect(this, SIGNAL(PopupMenuResult(int,int,int,QAction *)), target, slotName);
   connect(this, SIGNAL(triggered(QAction *)), this, SLOT(ActionTriggered(QAction *)));
}

AudioMovePopupMenu :: ~AudioMovePopupMenu()
{
   // empty
}

void AudioMovePopupMenu :: Go(int objectType, int objectID, int idAtMouse)
{
   _goCalled   = true;
   _objectType = objectType;
   _objectID   = objectID;
   if ((_submenusAdded == false)&&(_idToAction.IsEmpty())) EmitSignal();
   else
   {
      QAction * a = NULL; (void) _idToAction.Get(idAtMouse);
      popup(QCursor::pos(), a);
   }
}

void AudioMovePopupMenu :: hideEvent(QHideEvent * e)
{
   QMenu::hideEvent(e);
   if (_signalEmissionPending) EmitSignal();
   CheckForAutoDelete();
}

void AudioMovePopupMenu :: CheckForAutoDeleteAux()
{
   _singleShotPending = false;
   CheckForAutoDelete();
}

void AudioMovePopupMenu :: CheckForAutoDelete()
{
   if ((_autoDelete)&&(isHidden()))
   {
      if (isTearOffMenuVisible())
      {
         // FogBugz #5312:  Since there's no way to get a signal or event when the
         // tear-off menu is closed, we've got to poll instead!   Yuck!!!!
         // Once TrollTech implements TrollTask #236340 we can remove this hackery...
         if (_singleShotPending == false)
         {
            _singleShotPending = true;
            QTimer::singleShot(100, this, SLOT(CheckForAutoDeleteAux()));
         }
     }
      else
      {
        _autoDelete = false;  // make doubly sure we never call deleteLater() more than once!
        deleteLater();
      }
   }
}

void AudioMovePopupMenu :: EmitSignal()
{
   _signalEmissionPending = false;
   int id = -1;
   if (_actionToEmit) (void) _actionToID.Get(_actionToEmit, id);

   // If Go() wasn't called on us personally, then we'll go up
   // the ownership hierarchy and find the most recent ancestor
   // that it was called on.  That way submenus inherit these
   // parameters from their parent menus.
   int objectID   = -1;
   int objectType = -1;
   {
      QObject * pm = this;
      while(pm != NULL)
      {
         AudioMovePopupMenu * ppm = dynamic_cast<AudioMovePopupMenu *>(pm);
         if ((ppm)&&(ppm->_goCalled))
         {
            objectType = ppm->_objectType;
            objectID   = ppm->_objectID;
            break;
         }
         pm = pm->parent();
      }
   }

   emit PopupMenuResult(objectType, objectID, id, _actionToEmit);
}

void AudioMovePopupMenu :: ActionTriggered(QAction * act)
{
   int id;
   if (_actionToID.Get(act, id).IsOK())
   {
      _actionToEmit = act;
      if (isHidden()) EmitSignal();
                 else _signalEmissionPending = true;
   }

   hide();
   CheckForAutoDelete();
}

QAction * AudioMovePopupMenu :: InsertItem(const QString & text, int id, bool isEnabled, bool isCheckable, bool isChecked)
{
   QAction * act = new QAction(text, this);
   act->setEnabled(isEnabled);
   act->setCheckable(isCheckable);
   act->setChecked(isChecked);

   (void) _actionToID.Put(act, id);
   if (_idToAction.ContainsKey(id) == false) (void) _idToAction.Put(id, act);
   addAction(act);
   return act;
}

QAction * AudioMovePopupMenu :: InsertSubmenu(const QString & subMenuName, QMenu * subMenu)
{
   if (subMenu->parent() != this) LogTime(MUSCLE_LOG_ERROR, "InsertSubmenu:  subMenu %p has wrong parent %p (should be %p)\n", subMenu, subMenu->parent(), this);
   subMenu->setTitle(subMenuName);
   _submenusAdded = true;
   return addMenu(subMenu);
}

QAction * AudioMovePopupMenu :: InsertSeparator()
{
   // Nothing special, for now...
   return addSeparator();
}

void AudioMovePopupMenu :: Clear()
{
   _actionToID.Clear();
   _idToAction.Clear();
   _submenusAdded = false;
   clear();
}

void AudioMovePopupMenu :: SetItemChecked(int id, bool isChecked)
{
   QAction * a = _idToAction[id];
   if (a) a->setChecked(isChecked);
}

bool AudioMovePopupMenu :: IsItemChecked(int id) const
{
   const QAction * a = _idToAction[id];
   return ((a)&&(a->isChecked()));
}

void AudioMovePopupMenu :: SetItemEnabled(int id, bool isEnabled)
{
   QAction * a = _idToAction[id];
   if (a) a->setEnabled(isEnabled);
}

bool AudioMovePopupMenu :: IsItemEnabled(int id) const
{
   const QAction * a = _idToAction[id];
   return ((a)&&(a->isEnabled()));
}

};  // end namespace audiomove
