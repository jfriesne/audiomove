#!/bin/sh

# A function to set the timestamps of all files matching the arguments back
# to a fixed time in the future.  (Just doing a plain 'touch' without a fixed
# time specified meant that the files would occasionally end up with different
# timestamps, which would cause a build failure because automake is missing
# from the build environment)
touch_without_race_condition ()
{
   touch temp_dummy_file.txt       # touch one file just to give us a single timestamp
   touch -rtemp_dummy_file.txt $@  # then set all specified files to that timestamp
}

echo "************************************************************"
echo "* Building libsamplerate..."
echo "************************************************************"
pushd libsamplerate
touch_without_race_condition config* aclocal* Makefile*
./configure --enable-shared=no --prefix=`pwd`/temp_install
make install
popd

echo "************************************************************"
echo "* Building libogg..."
echo "************************************************************"
pushd libogg
./configure --enable-shared=no --prefix=`pwd`/temp_install
touch_without_race_condition config* aclocal* Makefile*
make install
popd

echo "************************************************************"
echo "* Building libvorbis..."
echo "************************************************************"
pushd libvorbis
./autogen.sh --enable-shared=no --prefix=`pwd`/temp_install --with-ogg=`pwd`/../libogg/temp_install
touch_without_race_condition config* aclocal* Makefile*
make install
popd

echo "************************************************************"
echo "* Building libFLAC..."
echo "************************************************************"
pushd flac
./configure --enable-shared=no --prefix=`pwd`/temp_install --with-ogg=`pwd`/../libogg/temp_install --disable-asm-optimizations
touch_without_race_condition config* aclocal* Makefile*
make install
popd

echo "************************************************************"
echo "* Building libsndfile..."
echo "************************************************************"
pushd libsndfile
export OGG_CFLAGS='-I'`pwd`'/../libogg/temp_install/include'
export OGG_LIBS='-L'`pwd`'/../libogg/temp_install/lib -logg'
export VORBIS_CFLAGS='-I'`pwd`'/../libvorbis/temp_install/include'
export VORBIS_LIBS='-L'`pwd`'/../libvorbis/temp_install/lib -lvorbis'
export VORBISENC_CFLAGS='-I'`pwd`'/../libvorbis/temp_install/include'
export VORBISENC_LIBS='-L'`pwd`'/../libvorbis/temp_install/lib -lvorbisenc'
export FLAC_CFLAGS='-I'`pwd`'/../flac/temp_install/include'
export FLAC_LIBS='-L'`pwd`'/../flac/temp_install/lib -lflac'
touch_without_race_condition config* aclocal* Makefile*
./configure --enable-shared=no --enable-external-libs --prefix=`pwd`/temp_install
make install
popd

echo "************************************************************"
echo "* Building AudioMove..."
echo "************************************************************"
pushd audiomove
 qmake
 make
 strip ./AudioMove
 mv ./AudioMove ..
 strip ./AudioMove.app/Contents/MacOS/AudioMove
 mv ./AudioMove.app ..
popd

echo "************************************************************"
echo "* Build script exiting -- assuming no errors, the AudioMove"
echo "* executable should now be in this folder."
echo "************************************************************"

