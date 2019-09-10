proc=$(uname -p)
 
if [ ${proc}="i386" ]
then
	# Build libsamplerate
	cd ../../libsamplerate
	./configure LDFLAGS="-arch ppc" CFLAGS="-arch ppc" CPPFLAGS="-arch ppc"
	make  #This step shows an error but libsamplerate.a is build
	cp src/.libs/libsamplerate.a libsamplerate_ppc.a 
	make clean
	./configure
	make
	cp src/.libs/libsamplerate.a libsamplerate_x86.a
	lipo -create libsamplerate_x86.a libsamplerate_ppc.a -output src/.libs/libsamplerate.a
	ranlib src/.libs/libsamplerate.a

	# Build libsndfile
	cd ../libsndfile
	./configure LDFLAGS="-arch ppc" CFLAGS="-arch ppc" CPPFLAGS="-arch ppc"
	make
	cp src/.libs/libsndfile.a libsndfile_ppc.a
	make clean
	./configure
	make
	cp src/.libs/libsndfile.a libsndfile_x86.a
	lipo -create libsndfile_x86.a libsndfile_ppc.a -output src/.libs/libsndfile.a
	ranlib src/.libs/libsndfile.a

	cd ../qtnet/AudioMove
	qmake "CONFIG += x86 ppc" Audiomove_svn.pro
	make

else
	echo This script only works on an Intel Mac.
fi
