/* This file is Copyright 2002 Level Control Systems.  See the included LICENSE.txt file for details. */

#include "audiomove/AudioMoveMessageBox.h"
#include "audiomove/MiscFunctions.h"

namespace audiomove {

AudioMoveMessageBox :: AudioMoveMessageBox(bool autoDelete, QObject * target, const char * slotName, QWidget * parent) : QMessageBox(parent), _autoDelete(autoDelete), _signalEmitted(true)
{
   if ((target)&&(slotName)) connect(this, SIGNAL(OptionChosen(int, const muscle::MessageRef &, int, int)), target, slotName);
}

AudioMoveMessageBox :: AudioMoveMessageBox(bool autoDelete, QObject * target, const char * slotName, const QString & caption, const QString & text, Icon icon, int button0, int button1, int button2, QWidget * parent, WindowFlags flags) : QMessageBox(caption, text, icon, button0, button1, button2, parent, flags), _autoDelete(autoDelete), _signalEmitted(true)
{
   if ((target)&&(slotName)) connect(this, SIGNAL(OptionChosen(int, const muscle::MessageRef &, int, int)), target, slotName);
}

AudioMoveMessageBox :: ~AudioMoveMessageBox()
{
   // empty
}

void AudioMoveMessageBox :: Go(int objectType, int objectID, const MessageRef & messageState)
{
   _objectType = objectType;  
   _objectID   = objectID;
   _msg        = messageState;
   _signalEmitted = false;
   ShowDialog(this);
}

void AudioMoveMessageBox :: done(int r)
{
   emit OptionChosen(r, _msg, _objectType, _objectID);
   QMessageBox::done(r);
   if (_autoDelete) deleteLater();
}

void AudioMoveMessageBox :: ShowMessage(const QString & caption, const QString & text, Icon icon, QWidget * parent)
{
   (new AudioMoveMessageBox(true, NULL, NULL, caption, text, icon, QMessageBox::Ok|QMessageBox::Default, NoButton, NoButton, parent))->Go();
}

};  // end namespace audiomove
