/* This file is Copyright 2002 Level Control Systems.  See the included LICENSE.txt file for details. */

#ifndef AudioMoveMessageBox_h
#define AudioMoveMessageBox_h

#include <QMessageBox>

#include "audiomove/AudioMoveNameSpace.h"
#include "message/Message.h"

namespace audiomove {

/** This implementation of QMessageBox is designed to simplify the
  * conversion of modal code (i.e. code that calls modal QMessageBox functions) 
  * into a safer, non-modal code style.
  */
class AudioMoveMessageBox : public QMessageBox
{
Q_OBJECT

public:
   /** Constructor.
     * @param autoDelete If true, we will delete ourself when we become hidden.  Otherwise,
     *                   it will be up to the calling code to delete us.
     * @param target The QObject to connect our MessageBoxResult signal to.  If NULL, no connection will be made.
     *               This slot should match the (int button, const MessageRef & ms, int objectType, int objectID) argument pattern.
     * @param slotName The slot name to connect our MessageBoxResult signal to.  If NULL, no connection will be made.
     * @param parent Passed to the QMessageBox constructor.
     */
   AudioMoveMessageBox(bool autoDelete, QObject * target, const char * slotName, QWidget * parent);

   /** Extended constructor, for convenience.
     * @param autoDelete If true, we will delete ourself when we become hidden.  Otherwise,
     *                   it will be up to the calling code to delete us.
     * @param target The QObject to connect our MessageBoxResult signal to.  If NULL, no connection will be made.
     *               This slot should match the (int button, const MessageRef & ms, int objectType, int objectID) argument pattern.
     * @param slotName The slot name to connect our MessageBoxResult signal to.  If NULL, no connection will be made.
     * @param caption Message box caption.  Passed on to the QMessageBox constructor.
     * @param text Message box text.  Passed on to the QMessageBox constructor.
     * @param icon Message box icon.  Passed on to the QMessageBox constructor.
     * @param button0 Type code for the first button.  Passed on to the QMessageBox constructor.
     * @param button1 Type code for the second button.  Passed on to the QMessageBox constructor.
     * @param button2 Type code for the third button.  Passed on to the QMessageBox constructor.
     * @param parent Passed to the QMessageBox constructor.
     * @param flags window flags to pass to the QMessageBox constructor.
     */
   AudioMoveMessageBox(bool autoDelete, QObject * target, const char * slotName, const QString & caption, const QString & text, Icon icon, int button0, int button1, int button2, QWidget * parent, WindowFlags flags = Dialog|MSWindowsFixedSizeDialogHint);

   /** Destructor. */
   virtual ~AudioMoveMessageBox();

   /** Causes the message box to become visible.  This method always returns immediately.
     * @param objectType This type value will be passed back in the signal we generate.
     * @param objectID This object ID value will be passed back in the signal we generate.
     * @param messageState this MessageRef will be passed back in the signal we generate.
     */
   void Go(int objectType=0, int objectID=0, const MessageRef & messageState = MessageRef());

   /** Convenience method:  Creates a messagebox and shows it to the user.  This method always returns immediately.
     * @param caption Text to show at the top of the message box
     * @param text Text to show inside the message box
     * @param icon Icon to show in the Message box
     * @param parent Parent widget of this message box
     */
   static void ShowMessage(const QString & caption, const QString & text, Icon icon, QWidget * parent);

signals:
   /** This signal is sent when the user chooses an item or dismisses the message box.
     * If (autoDelete) is true, then this AudioMoveMessageBox will delete itself after emitting the signal.
     * @param buttonID The ID of the button that was clicked, or QMessageBox::NoButton if no button was clicked.
     * @param msg The MessageRef that was passed in to ShowMessageBox().
     * @param objectType The object type value, as was passed in to Go()
     * @param objectID The object ID value, as was passed in to Go()
     */
   void OptionChosen(int buttonID, const muscle::MessageRef & msg, int objectType, int objectID);
 
protected:
   virtual void done(int r);

private slots:
   void HandleDoneAux(int r);

private:
   void EnableHackFixForFogBugz15263();

   bool _autoDelete;
   bool _signalEmitted;
   int _objectType;
   int _objectID;
   MessageRef _msg;
};

};  // end namespace audiomove

#endif
