#!/bin/bash

pushd submodules

if [ "$BUILD_UNIVERSAL_BINARY" != "" ]; then
   echo "Environment variable BUILD_UNIVERSAL_BINARY detected:  Building for both x86_64 and arm64 architectures"
else
   echo "Building native binary only.  On MacOS, you can set environment variable BUILD_UNIVERSAL_BINARY to build a universal binary instead."
fi

# Apple clang 16 and later default to -std=gnu23, in which an empty parameter
# list "f()" declares a function taking NO arguments rather than an unspecified
# number of them.  Several of the vendored libraries still contain K&R-style
# declarations, so pin the C dialect back to C17 for the dependency builds.
EXTRA_CFLAGS="-std=gnu17"

# Build the vendored libraries against the same deployment floor that Qt uses,
# so that every object in the final binary agrees on a minimum OS.  Without
# this, ld warns that each vendored object "was built for newer 'macOS' version
# (26.0) than being linked (14.0)", and the bundle would advertise an
# LSMinimumSystemVersion it cannot actually honour.
export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-14.0}"

function do_configure {
   if [ "$BUILD_UNIVERSAL_BINARY" != "" ]; then
      CFLAGS="$CFLAGS $EXTRA_CFLAGS -arch x86_64 -arch arm64" \
      CXXFLAGS="$CXXFLAGS -arch x86_64 -arch arm64"           \
      LDFLAGS="$LDFLAGS -arch x86_64 -arch arm64" ./configure $@
   else
      CFLAGS="$CFLAGS $EXTRA_CFLAGS" ./configure $@
   fi
}

echo "************************************************************"
echo "* Building libsamplerate..."
echo "************************************************************"
pushd libsamplerate
./autogen.sh
do_configure --enable-shared=no --prefix=$(pwd)/temp_install
make install
popd

echo "************************************************************"
echo "* Building ogg..."
echo "************************************************************"
pushd ogg
./autogen.sh
do_configure --enable-shared=no --prefix=$(pwd)/temp_install
make install
popd

echo "************************************************************"
echo "* Building opus..."
echo "************************************************************"
pushd opus
./autogen.sh
do_configure --enable-shared=no --prefix=$(pwd)/temp_install --with-ogg=$(pwd)/../ogg/temp_install
make install
popd

echo "************************************************************"
echo "* Building vorbis..."
echo "************************************************************"
pushd vorbis
# libvorbis's configure.ac still passes -force_cpusubtype_ALL on Darwin, a
# PowerPC-transition-era flag that Apple's current linker rejects outright
# ("ld: unknown options: -force_cpusubtype_ALL").  That made every Ogg probe in
# configure fail to *link*, so configure wrongly concluded libogg was missing.
# Substitute the C17 pin rather than deleting outright, because the Darwin
# branch overwrites CFLAGS wholesale and would otherwise drop $EXTRA_CFLAGS.
# (-i.bak rather than a bare -i, because BSD sed on MacOS requires an argument
# to -i; the backup is removed again so it does not litter the submodule.)
sed -i.bak "s/-force_cpusubtype_ALL/$EXTRA_CFLAGS/g" configure.ac && rm -f configure.ac.bak
./autogen.sh
do_configure --enable-shared=no --prefix=$(pwd)/temp_install --with-ogg=$(pwd)/../ogg/temp_install
make install
popd

echo "************************************************************"
echo "* Building flac..."
echo "************************************************************"
pushd flac
./autogen.sh
do_configure --enable-shared=no --prefix=$(pwd)/temp_install --with-ogg=$(pwd)/../ogg/temp_install --disable-asm-optimizations
make install
popd

echo "************************************************************"
echo "* Building lame..."
echo "************************************************************"
pushd lame
do_configure --enable-shared=no --prefix=$(pwd)/temp_install
make install
popd

echo "************************************************************"
echo "* Building mpg123..."
echo "************************************************************"
pushd mpg123
autoreconf -iv
do_configure --enable-shared=no --disable-modules --with-cpu=generic_fpu --prefix=$(pwd)/temp_install
make install
popd

echo "************************************************************"
echo "* Building libsndfile..."
echo "************************************************************"
pushd libsndfile
export OGG_CFLAGS='-I'$(pwd)'/../ogg/temp_install/include'
export OGG_LIBS='-L'$(pwd)'/../ogg/temp_install/lib -logg'
export OPUS_CFLAGS='-I'$(pwd)'/../opus/temp_install/include'
export OPUS_LIBS='-L'$(pwd)'/../opus/temp_install/lib -lopus'
export OPUSENC_CFLAGS='-I'$(pwd)'/../opus/temp_install/include'
export OPUSENC_LIBS='-L'$(pwd)'/../opus/temp_install/lib -lopusenc'
export VORBIS_CFLAGS='-I'$(pwd)'/../vorbis/temp_install/include'
export VORBIS_LIBS='-L'$(pwd)'/../vorbis/temp_install/lib -lvorbis'
export VORBISENC_CFLAGS='-I'$(pwd)'/../vorbis/temp_install/include'
export VORBISENC_LIBS='-L'$(pwd)'/../vorbis/temp_install/lib -lvorbisenc'
export FLAC_CFLAGS='-I'$(pwd)'/../flac/temp_install/include'
export FLAC_LIBS='-L'$(pwd)'/../flac/temp_install/lib -lflac'
export CFLAGS='-I'$(pwd)'/../lame/temp_install/include'
export LIBS='-L'$(pwd)'/../lame/temp_install/lib -lmp3lame'
export MPG123_CFLAGS='-I'$(pwd)'/../mpg123/temp_install/include'
export MPG123_LIBS='-L'$(pwd)'/../mpg123/temp_install/lib -lmpg123'
autoreconf -vif
do_configure --enable-shared=no --enable-external-libs --prefix=$(pwd)/temp_install
make install
popd

popd  # Pop out of submodules folder

echo "************************************************************"
echo "* Building AudioMove..."
echo "************************************************************"
pushd audiomove
 qmake
 make
  if test -f ./AudioMove; then
     strip ./AudioMove
  fi
  if test -f ./AudioMove.app/Contents/MacOS/AudioMove; then
     strip ./AudioMove.app/Contents/MacOS/AudioMove
  fi
popd

echo "************************************************************"
echo "* Build script exiting -- assuming no errors, the AudioMove"
echo "* executable should now be in the audiomove sub-folder."
echo "************************************************************"

