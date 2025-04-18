/* This file is Copyright 2002 Level Control Systems.  See the included LICENSE.txt file for details. */

#include "audiomove/AudioMoveMessageBox.h"
#include "audiomove/MiscFunctions.h"

namespace audiomove {

AudioMoveMessageBox :: AudioMoveMessageBox(bool autoDelete, QObject * target, const char * slotName, QWidget * parent) : QMessageBox(parent), _autoDelete(autoDelete), _signalEmitted(true), _objectType(0), _objectID(0)
{
   EnableHackFixForFogBugz15263();
   if ((target)&&(slotName)) connect(this, SIGNAL(OptionChosen(QMessageBox::ButtonRole, const muscle::MessageRef &, int, int)), target, slotName);
}

AudioMoveMessageBox :: AudioMoveMessageBox(bool autoDelete, QObject * target, const char * slotName, const QString & caption, const QString & text, Icon icon, StandardButtons standardButtons, QWidget * parent, WindowFlags flags) : QMessageBox(icon, caption, text, standardButtons, parent, flags), _autoDelete(autoDelete), _signalEmitted(true), _objectType(0), _objectID(0)
{
   EnableHackFixForFogBugz15263();
   if ((target)&&(slotName)) connect(this, SIGNAL(OptionChosen(QMessageBox::ButtonRole, const muscle::MessageRef &, int, int)), target, slotName);
}

AudioMoveMessageBox :: ~AudioMoveMessageBox()
{
   // empty
}

void AudioMoveMessageBox :: EnableHackFixForFogBugz15263()
{
#if QT_VERSION >= 0x050c02  // FogBugz #15263
   connect(this, SIGNAL(finished(int)), this, SLOT(HandleDoneAux()));
#endif
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
#if QT_VERSION < 0x050c02  // FogBugz #15263
   HandleDoneAux();
#endif
   QMessageBox::done(r);
}

void AudioMoveMessageBox :: HandleDoneAux()
{
   QAbstractButton * b = clickedButton();

   const QMessageBox::ButtonRole r = b ? buttonRole(b) : QMessageBox::InvalidRole;
   emit OptionChosen(r, _msg, _objectType, _objectID);

   if (_autoDelete) deleteLater();
}

void AudioMoveMessageBox :: ShowMessage(const QString & caption, const QString & text, Icon icon, QWidget * parent)
{
   (new AudioMoveMessageBox(true, NULL, NULL, caption, text, icon, StandardButtons(QMessageBox::Ok), parent))->Go();
}

};  // end namespace audiomove
