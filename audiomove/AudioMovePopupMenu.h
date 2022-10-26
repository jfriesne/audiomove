/* This file is Copyright 2002 Level Control Systems.  See the included LICENSE.txt file for details. */

#ifndef AudioMovePopupMenu_h
#define AudioMovePopupMenu_h

#include <QAction>
#include <QPointer>
#include <QMenu>

#include "audiomove/AudioMoveNameSpace.h"
#include "util/Hashtable.h"

class QAction;

namespace audiomove {

/** This implementation of QPopupMenu is designed to simplify the
  * conversion of modal code (i.e. code that calls exec()) into a safer,
  * non-modal code style.
  */
class AudioMovePopupMenu : public QMenu
{
Q_OBJECT

public:
   /** Constructor.
     * @param autoDelete If true, we will delete ourself when we become hidden.  Otherwise,
     *                   it will be up to the calling code to delete us.
     * @param target The QObject to connect our PopupMenuResult signal to.  If NULL, no connection will be made.
     * @param slotName The slot name to connect our PopupMenuResult signal to.  If NULL, no connection will be made.
     * @param parent Passed to the QPopupMenu constructor.
     */
   AudioMovePopupMenu(bool autoDelete, QObject * target, const char * slotName, QWidget * parent);

   /** Destructor. */
   virtual ~AudioMovePopupMenu();

   /** Causes the popup menu to become visible.
     * @param objectType This type value will be passed back in the signal we generate.
     * @param objectID This object ID value will be passed back in the signal we generate.
     * @param indexAtPoint If specified, the menu will pop up so that the menu item with the specified ID
     *                     will appear under the cursor by default.  Leave unspecified to invoke default behaviour.
     */
   void Go(int objectType, int objectID, int indexAtPoint=-1);

   /** Insert a command entry (i.e. without a checkbox) into the popup menu.
     * @param text Text to display for this entry
     * @param id Numeric ID for this entry
     * @param isEnabled If false, this entry will be disabled.  Defaults to true.
     * @param isChecked If true, this entry will be checked and have a checkbox.
     * @param isCheckable If true, this entry will have a checkbox.  Defaults to false.
     * @returns the resulting QAction object, in case you want to mess with it further.
     */
   QAction * InsertCommandItem(const QString & text, int id, bool isEnabled = true) {return InsertItem(text, id, isEnabled, false, false);}

   /** Insert a checkbox entry (i.e. with a checkbox) into the popup menu.
     * @param isChecked If true, this entry will be checked.
     * @param text Text to display for this entry
     * @param id Numeric ID for this entry
     * @param isEnabled If false, this entry will be disabled.  Defaults to true.
     * @returns the resulting QAction object, in case you want to mess with it further.
     */
   QAction * InsertCheckboxItem(bool isChecked, const QString & text, int id, bool isEnabled = true) {return InsertItem(text, id, isEnabled, true, isChecked);}

   /** Inserts the specified submenu as a child of this menu, and gives it the specified name. */
   QAction * InsertSubmenu(const QString & subMenuName, QMenu * subMenu);

   /** Inserts a separator bar into the popup menu, and returns the corresponding QAction object. */
   QAction * InsertSeparator();

   /** Resets this menu to its default/empty state */
   void Clear();

   virtual void hideEvent(QHideEvent * e);

   /** Sets the given item to be checked or unchecked.
     * @param item ID, as was previously passed to (InsertItem)
     * @param isChecked true iff you want the item to be checked, or false otherwise.
     */
   void SetItemChecked(int id, bool isChecked);

   /** Returns true iff the item with the given ID is currently checked.
     * @param item ID, as was previously passed to (InsertItem)
     */
   bool IsItemChecked(int id) const;

   /** Sets the given item to be enabled or disabled.
     * @param item ID, as was previously passed to (InsertItem)
     * @param isChecked true iff you want the item to be enabled, or false otherwise.
     */
   void SetItemEnabled(int id, bool isChecked);

   /** Returns true iff the item with the given ID is currently enabled.
     * @param item ID, as was previously passed to (InsertItem)
     */
   bool IsItemEnabled(int id) const;

signals:
   /** This signal is sent when the user chooses an item, or when the popup menu is hidden.
     * If (autoDelete) is true, then this AudioMovePopupMenu will delete itself after emitting the signal.
     * @param objectType The object type value, as was passed in to Go()
     * @param objectID The object ID value, as was passed in to Go()
     * @param menuItemID The ID of the menu item that was chosen, or -1 if no menu item was chosen.
     * @param action the QAction object representing the menu item that was fired.  Will be NULL if the menu was cancelled.
     */
   void PopupMenuResult(int objectType, int objectID, int menuID, QAction * action);

private slots:
   void ActionTriggered(QAction *);
   void EmitSignal();
   void CheckForAutoDeleteAux();

private:
   void CheckForAutoDelete();
   QAction * InsertItem(const QString & text, int id, bool isEnabled, bool isCheckable, bool isChecked);

   Hashtable<QAction *, int> _actionToID;
   Hashtable<int, QAction *> _idToAction;
   bool _goCalled;
   bool _autoDelete;
   bool _signalEmissionPending;
   bool _submenusAdded;
   bool _singleShotPending;
   int _objectType;
   int _objectID;
   QPointer<QAction> _actionToEmit;  // only valid when _signalEmissionPending is true
};

};  // end namespace audiomove

#endif
