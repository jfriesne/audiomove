pushd ..\..\libsamplerate
call make.bat
popd
mkdir release
copy ..\..\libsamplerate\libsamplerate.dll release
copy ..\..\libsndfile\win32\libsndfile-1.dll release
qmake -t vcapp audiomove_svn.pro
devenv Audiomove.sln /build Release
