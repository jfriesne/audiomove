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

#include <QApplication>

#include "audiomove/AudioMoveWindow.h"
#include "system/SetupSystem.h"
#include "util/MiscUtilityFunctions.h"
#include "util/String.h"

using namespace audiomove;

extern bool _isReadBroadcastInfoEnabled;

int main(int argc, char ** argv)
{
   CompleteSetupSystem system;
   QApplication app(argc, argv);

   if (strstr(argv[0], "disable_bwf") != NULL)
   {
      printf("Program name contains disable_bwf, so no broadcast WAV info will be read or written.\n");
      _isReadBroadcastInfoEnabled = false;
   }

   String sTitle = String(argv[0]);
#ifdef __APPLE__
   sTitle = sTitle.Substring(0, ".app/");  // we want the user-visible name, not the internal name!
#endif
#ifdef __WIN32__
   sTitle = sTitle.Substring("\\").Substring(0, ".exe");
#else
   sTitle = sTitle.Substring("/");
#endif

   // Any arguments without equals signs are assumed to be file names!
   Message args;
   for (int i=1; i<argc; i++)
   {
#ifdef WIN32
      if (strcmp(argv[i], "console") == 0)
      {
         AllocConsole();
         freopen("conout$", "w", stdout);
         freopen("conout$", "w", stderr);
      }
      else
#endif
      if (strcmp(argv[i], "disable_bwf") == 0)  // except this one
      {
         args.AddString("disable_bwf", "");
         argv[i] = NULL;
      }
      else if (strchr(argv[i], '=') == NULL)
      {
         args.AddString("file", argv[i]);
         argv[i] = NULL;
      }
   }

   (void) ParseArgs(argc, argv, args);
   AudioMoveWindow win(args);
#if defined(SVN_VERSION_STRING)
	win.setWindowTitle(ToQ((sTitle + " v" + VERSION_STRING + " (Build #" + SVN_VERSION_STRING + ')')()));
#else
   win.setWindowTitle(ToQ((sTitle + " v" + VERSION_STRING)()));
#endif
   win.show();
   return app.exec();
}
