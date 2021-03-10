AudioMove v1.22 3/9/2021
by Jeremy Friesner (jaf@meyersound.com)
Copyright 2004-2021 Meyer Sound Laboratories
Licensed under the GNU Public License
See file COPYING.TXT for details.


This directory contains AudioMove, Meyer Sound's handy batch-mode file-conversion utility.  You can use this utility to quickly copy files from one location to another, and optionally convert their format as part of the copy.


What can AudioMove do?
----------------------

AudioMove can change the file's format from (any format that libsndfile can read) to WAV, AIFF, FLAC, OGG, and several other formats.

AudioMove can change the file's sampling rate from (any rate that libsndfile can read) to a number of output sampling rates, ranging from 8kHz to 192kHz.

AudioMove can change the file's sample width from (any width that libsndfile can read) to 8, 16, 24, or 32 bit, floating point, or double-precision floating point format.

AudioMove can optionally convert stereo or multitrack files into multiple single-track (mono) files.


How do I install AudioMove?
---------------------------

Just drag the AudioMove folder to wherever you want to keep it.

To run AudioMove, just double-click its icon.


How is AudioMove licensed?
--------------------------

AudioMove is licensed under the GNU Public License.  See the file COPYING for details.  (Note that the code in the muscle subdirectory is also available under the BSD license)

AudioMove uses the following third party libraries under the terms of the GPL:

libnsdfile    - Source code to libsndfile can be downloaded at http://www.mega-nerd.com/libsndfile/#Download
libsamplerate - Source code to libsamplerate can be downloaded at http://www.mega-nerd.com/SRC/download.html
libogg        - Source code to libogg can be downloaded at http://www.xiph.org/downloads/
libvorbis     - Source code to libvorbis can be downloaded at http://www.xiph.org/downloads/
libFLAC       - Source code to libFLAC can be downloaded at http://flac.sourceforge.net/download.html
AKRip.dll     - (Win32 build only) Source code to AKRip can be downloaded at http://akrip.sourceforge.net/

Note that known-good source code archives of all of the libraries mentioned above are included in the AudioMove source code archive, so you don't have to download them all yourself if you odn't want to.


How do I use AudioMove? 
-----------------------

Follow these steps:

1) Launch AudioMove by double clicking its icon

2) Choose your desired output settings from the combo boxes at the top of the window.  You may choose the output files' format (WAV, AIFF, FLAC, OGG, etc, or "Same as Source File"), their sampling rate, and the quality of the sample-rate conversion ("Good", "Better", or "Best").  Higher quality sample rate conversion takes longer, but the result sounds slightly better.

3) Click the "Destination Folder" button and choose the folder that you want AudioMove to write its output files to.  Or if you prefer to do in-place conversions (where the output file replaces the input file) click the "Convert Files In-Place" checkbox.  (Warning:  be careful with in-place conversions, since your original input files will be replaced by the output files and won't be recoverable later if you change your mind!)

4) Click "Add Files" to choose the file(s) you want to convert via a file requester, or drag-and-drop the files or folders you want to convert into the white area of the window.  (Folders you drag in will have their contents converted)

5) Wait until all the transfer-stats rows have turned green.  At that point, the file conversion process is complete and your output files are ready for use in the destination folder you specified above.


Can I use AudioMove from the command line, or call it from a script?
--------------------------------------------------------------------

Absolutely!  AudioMove will let you run it from the command line and specify arguments for what it should do when it runs.  This is useful e.g. for running AudioMove from scripts or from other programs.  To run it this way, open a shell, cd to the AudioMove directory, and type

./AudioMove (args)

or, on a Mac,

./AudioMove.app/Contents/MacOS/AudioMove (args)

But wait -- don't literally type (args)... instead, replace (args) with some or all of the keyword=value pairs specified below.  Note that many of the keywords have several synonyms that are equivalent, for convenience.  For any keywords that are not specified, the default setting that was active the last time AudioMove was quit will be used instead.

Keywords   | Description
-----------+----------------------------------------------------------------------
dir        | Specify the directory that AudioMove should write
dest       | its output audio files into.  This directory should already exist.
destdir    |   e.g. dir=/Users/jeremy/MyConvertedAudioFiles
           |
file       | Specify an input audio file to read.  This keyword may be specified 
           | multiple times if you wish to convert multiple files.  Also, any
           | command line arguments that you specify that don't have an equal sign
           | in them are assumed to be file names.  (Note that if you want to use
           | wild card expansions, you must omit the file= prefix)
           | 
           |    e.g. file=/Users/jeremy/src/File1.aiff file=File2.aiff
           |    or:  *.aiff *.wav congo.0.wav
           |
format     | Specify the output file format.  Possible values are AIFF,
destformat | WAV, FLAC, OGG, PAF, W64, CAF, RF64, OPUS, or Source.  e.g. destformat=AIFF
           |
rate       | Specify the output files' sampling rate.  Possible values are
destrate   | 8, 11, 12, 16, 22, 24, 44, 48, 88, 96, 176, 192, or Source.  e.g. destrate=44
           |
width      | Specify the output files' sample width.  Possible values are
destwidth  | 8, 16, 32, float, or Source.  e.g. destwidth=Source
           |
qual       | Specify the sample rate converter's quality setting.  Possible values
quality    | are good, better, or best.   e.g. qual=best
conversionquality
           |
pause      | If specified, AudioMove will start up with in paused mode.
paused     | In this mode, conversions will not start until you uncheck the
           | "Paused" checkbox.   e.g. pause=yes
           |
threads    | Specifies how many simultaneous processing threads AudioMove may
maxthreads | launch.  Available values range from 1 to 9.   e.g. threads=5
           |
split      | If specified, stereo and multitrack input files will be converted 
splitfiles | into multiple mono files.
           |
inplace    | If specified, destdir will be ignored and all file conversions will
convertinplace       be done "in-place" (replacing the input files on completion)
           |
quit       | Specifies that AudioMove should automatically quit when it is done
autoquit   | processing audio.  Possible values are "no" (i.e. don't quit when
           | finished, "yes" (which will cause AudioMove to quit when it is finished,
           | except if there are errors), or "force" (which will cause AudioMove
           | to quit when finished even if there are errors.  e.g. quit=yes
           |
disable_bwf| If specified, then AudioMove won't read or write BWF chunks in WAV
           | files.  (This might be useful if you are using AudioMove in conjunction
           | with old software that doesn't handle BWF chunks well)

Here is an example invocation of AudioMove from the command line under MacOS/X:

./AudioMove.app/Contents/MacOS/AudioMove /Users/jeremyfriesner/wtrxaudio/congo.0.wav /Users/jeremyfriesner/wtrxaudio/*.aiff dest=/Users/jeremyfriesner format=AIFF rate=48 width=float quit=yes


What does the "Create Virtual Drive File" button do?
====================================================

It iterates recursively over all the audio files in the current Destination Folder, extracting metadata (currently, the file name, file length, and number of tracks in the file) from each audio file.  The metadata is saved into a compressed ".lcsDisk" file that can then later be used as a dummy substitute for the actual files, when working in an environment where the actual files are not available.

NOTE:  To make use of the .lcsDisk file, you will need to have software that knows how to read it.  As of this writing, the only programs that knows how to read .lcsDisk files are Meyer Sound's CueStation and VirtualLX programs -- so if you aren't a Meyer Sound customer, this feature probably won't be very useful to you.

 
History
-------

v1.22 3/9/2021   - Migrated the codebase to Github ( https://github.com/jfriesne/audiomove )
                   for easier development
                 - Updated muscle submodule to v8.0
                 - Updated error-handling the modern-muscle style
                 - Updated audio-handling submodules to the latest versions
                 - Updated the code to compile under either Qt 4.x or Qt 5.x
                 - Added .W64, .CAF, .RF64, and .OPUS as output-format options
                 - Added 8kHz, 12kHz, 16kHz, and 24kHz as output-rate options
                 - "Rate" combo box now turns red if set to a known-not-to-work setting
                   for the current Output Format (e.g. 96kHz for Ogg/Opus)
                 - "Width" combo box now turns red if set to a known-not-to-work setting
                   for the current Output Format (e.g. 32-bit Fixed for FLAC)
                 * 8-bit output-width now works as expected for all supported output formats.

v1.21 5/19/2016  - Upgraded the included libraries to more recent versions
                 - Added support for little-endian and big-endian PAF output format
                 - Added a "disable_bwf" command line argument, in case you need to
                   disable the reading and writing of Broadcast WAV format information
                   for some reason.  This flag can also be enabled by renaming the
                   AudioMove executable to include the substring disable_bwf in its name.

v1.20 3/5/2009   - Added support for reading and writing FLAC and OGG file types.
                 - The Win32 build of AudioMove can now read .cda files directly
                   from audio CD, thanks to Andy Key's AKRip CD-ripping library.
                   (used under the terms of the GPL)
                 - Added an "Add Folder..." button that make it a bit more
                   intuitive to set up recursive batch conversions.
                 - Upgraded the included muscle distribution to v4.50.
                 - Upgraded the included libsamplerate distribution to v0.1.7.
                 - Upgraded the included libsndfile distribution to v1.0.19.
                 - Added included distributions for libogg, libvorbis, and libFLAC.
                 - Added a build-audiomove.sh script to make building AudioMove
                   more automatic under MacOS/X and Linux.  (The Windows build doesn't
                   have a build script, so you still have to assemble everything
                   manually.  It's probably best just to use the Windows AudioMove.exe
                   binary instead, and leave that pain to me) 
                 o Update AudioMove code to compile against (and require) Qt4.5.x.
                 * Fixed a bug where convert-in-place mode would try to replace
                   unusual filename extensions with their more usual ones, and
                   in the process create a renamed copy of the input file instead
                   of overwriting the input file (e.g. converting "file.aif" in-place
                   would instead output to "file.aiff")

v1.15 6/6/2007   - Added a "Split Stereo Files" checkbox.  When checked,
                   any stereo or multitrack input files will be converted
                   into multiple single-channel output files.
                 - Added keyboard-shortcuts to all the GUI controls.
                 - You can now right-click on the file view's column headers
                   bar to bring up a popup-menu that shows or hides columns.
                 - There is now a "Convert in Place" checkbox that you can click
                   if you want all of your conversions to happen in-place (i.e.
                   the output file replaces the input file)
                 - AudioMove now supports reading and writing 64-bit
                   "double" floating point sample widths.
                 o Ported the code to compile with Qt 4.x instead of Qt 3.x.
                 o Made the color scheme a bit brighter and prettier.
                 * The file requester now handles multiple files in
                   a recent-files line properly.
                 * Fixed a bug that could cause the conversion process
                   to stall if you deleted a job that was in progress.
                 * The file view's column ordering and sort order is now
                   saved and restored correctly across sessions.
                 * File names with foreign characters are now handled
                   correctly.
                 * AudioMove now preserves the EBU broadcast chunk (BEXT)
                   of broadcast WAV files when processing them back into 
                   WAV format.
                 * AudioMove now does the right thing when the input file
                   and the output file are the same file (before it could
                   end up munging the file in that case) 
                 * Fixed a bug that caused conversions of audio file
                   conversions to fail when the input file had a channel
                   count that was not a power of two.

v1.14 6/2/2006   - Added a recently-selected-files list and customizable
                   directory-shortcut buttons to the AudioMove file dialogs.
                 - Updated the included libsndfile distribution to v1.0.16.
                 - Renamed the items in the "Width" combo box to be more
                   explicit about the sample formats they describe.
                 - Updated the included muscle distribution to v3.21.
                  
v1.13 4/28/2006  - Updated the included libsndfile distribution 
                   to v1.0.16pre2.
                 * AudioMove 1.12 was outputting WAV files in WAV/RIFX
                   (big endian) format.  This was problematic because
                   most existing software doesn't know how to handle WAV/RIFX
                   format yet.  So in this version WAV files are output in
                   normal WAV/RIFF (little endian) format once again.

v1.12 3/25/2006  - The background color of the items being processed now
                   doubles as a progress bar, giving a visual indication
                   of how far along the file-conversion process is.
                 - Updated the included libsndfile install to version 1.15.
                 - Updated the included muscle install to version 3.20.
                 - Updated the included libsamplerate install to version 0.1.2.
                 * AudioMove now detects clipping in the output file and
                   renormalizes the output file (if necessary) to avoid it.  
                   (see the first question in the libsamplerate FAQ for 
                   details:  http://www.mega-nerd.com/SRC/faq.html#Q001 )
                 * Fixed a bug that would cause AudioMove to crash if you
                   quit AudioMove while file-conversions were ongoing.

v1.11 3/15/2006  - The "Destination" widget now turns pink whenever the
                   destination path is not a valid directory.  The widget
                   is updated whenever its contents change, and also every 5 
                   seconds (in case the file system changes).
                 * If the folder listed in the "Destination" widget does not
                   exist, all transfers will immediately fail.  In previous 
                   versions, the destination folder would be automatically created
                   in this case, but that turned out to be problematic because
                   if the destination referred to a mount-point which was not
                   currently mounted, a local directory would be created at the
                   mount-point's previous location.

v1.10 7/5/2005   - Added 16x16 window icon bitmap
                 - Updated the included libsndfile archive to 1.0.12pre7, 
                   primarily to add support for reading Sound Designer II format
                 - Updated the included muscle archive to v3.00.
                 - Added a "Create .lcsDisk" button that causes AudioMove to
                   scan the output directory and create a .lcsDisk file that
                   represents that directory.  This feature is useful only for 
                   people who also use LCS's VirtualLX and Wild Tracks products.
                 o Removed support for obsolete LCS audio format.
                 o Changed the default "Simultaneous Threads" value to 1.

v1.09 1/6/2005   - Added more sampling rate options to the Output Rate combo box,
                   including 192kHz, 176.4kHz, 96kHz, 88.2kHz, and 11.025kHz.
                 o Updated the included muscle folder to v2.62.
                 * Improved GUI presentation of sampling rate values.

v1.08 11/16/2004 * Tweaked the filename handling logic so that Japanese and
                   other non-ASCII filenames are handled properly under Windows.

v1.07 10/20/2004 - Arguments without equals signs are now interpreted as input
                   files.  This allows you to use wildcarding to specify input
                   files (e.g. "./AudioMove *.wav")
                 * Reorganized the GUI layout so that the controls appear in
                   the order you are most likely to want to use them.

v1.06  9/01/2004 - Installed the official AudioMove icon.
                 * Fixed a bug in the recursive drag-and-drop routine -- the
                   created folder hierarchy wasn't rooted correctly.

v1.05  8/17/2004 o Conversion Quality options are now "good", "better", or "best".
                 - Directories can now be dragged in to the AudioMove window for
                   recursive batch conversion of all subfolders and files.
                 - Default output directory is now the user's home directory.

v1.04  8/02/2004 o Changed the .pro file to statically link all libraries.
                 o Released AudioMove under GPL license.

v1.03  7/30/2004 - Added "Quality" combo box to the GUI, for choosing conversion quality.
                 o Integrated libsamplerate sample rate converter code.
                 * Made the GUI somewhat more space efficient.

v1.02  7/28/2004 - Added command-line argument parsing (see above for details)
                 * Fixed the bug that was causing silence gaps in the output of 
                   sample-rate-converted stereo files.

v1.01  7/19/2004 - Initial setup of files is now done in a separate thread, so that 
                   the GUI doesn't freeze up after dragging in a lot of files.
                 - Only the active processes keep file handles open now.
                 - Other minor GUI tweaks and enhancements.

v1.00  7/14/2004 - Initial Release
