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

#include "audiomove/AudioMoveThread.h"

namespace audiomove {

AudioMoveThread :: AudioMoveThread() : _downstreamAcceptor(NULL)
{
   // empty
}

AudioMoveThread :: ~AudioMoveThread()
{
   // empty
}

status_t AudioMoveThread :: MessageReceivedFromUpstream(const MessageRef & msg)
{
   return SendMessageToInternalThread(msg);
}

status_t AudioMoveThread :: MessageReceivedFromOwner(const MessageRef & msg, uint32)
{
   if (msg())
   {
      if (msg()->HasName(AUDIOMOVE_NAME_OPENFILE))
      {
         if (OpenFile().IsError())
         {
            char buf[128]; muscleSprintf(buf, "Couldn't open %s file", IsOutput() ? "output" : "input");
            msg()->AddString(AUDIOMOVE_NAME_STATUS, buf);
         }
      }
      else
      {
         bool isLastBuffer;
         if (msg()->FindBool(AUDIOMOVE_NAME_ISLAST, &isLastBuffer).IsError()) isLastBuffer = false;

         ByteBufferRef bufRef;
         if (msg()->FindFlat(AUDIOMOVE_NAME_BUF, bufRef).IsOK())
         {
            QString errStr;
            ByteBufferRef newRef = ProcessBuffer(bufRef, errStr, isLastBuffer);
            if (newRef() == NULL)
            {
               (void) msg()->RemoveName(AUDIOMOVE_NAME_BUF);
               if (errStr.length() > 0) (void) msg()->ReplaceString(true, AUDIOMOVE_NAME_STATUS, FromQ(errStr));
            }
            else if (newRef() != bufRef()) (void) msg()->ReplaceFlat(true, AUDIOMOVE_NAME_BUF, newRef);
         }
         else if (isLastBuffer) CloseFile(CLOSE_FLAG_FINAL);
      }
      if (_downstreamAcceptor) _downstreamAcceptor->MessageReceivedFromUpstream(msg);
      return B_NO_ERROR;
   }
   else return B_ERROR;
}

};  // end namespace audiomove
