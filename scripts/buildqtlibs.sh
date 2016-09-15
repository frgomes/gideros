#!/bin/bash -x

dir=$(dirname $(test -L "$BASH_SOURCE" && readlink -f "$BASH_SOURCE" || echo "$BASH_SOURCE"))

sudo apt-get install qt5-default qttools5-dev-tools qdbus-qt5 qt5-qmake python-pyqt5 python3-pyqt5 pyqt5-dev-tools -y


PREFIX=/usr/local
GIDEROS_HOME=$HOME/workspace/gideros
QMAKE="$(which qmake) PREFIX=${PREFIX}"

sudo mkdir -p $PREFIX/lib

pushd ${GIDEROS_HOME}/libpystring
$QMAKE libpystring.pro
make clean
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/libgvfs
$QMAKE libgvfs.pro
make clean
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/libgid
$QMAKE libgid_qt5.pro
make clean
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/lua
$QMAKE lua.pro
make clean
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/libgideros
$QMAKE libgideros.pro
make clean
make
sudo make dist
popd




function not_needed {
rm -r -f ${GIDEROS_HOME}/SDK
mkdir -p ${GIDEROS_HOME}/SDK/{include,lib/desktop}
pushd ${GIDEROS_HOME}/SDK
cp ../libgideros/gideros.h include
cp ../libgideros/gplugin.h include
cp ../libgideros/gproxy.h include
cp ../libgideros/greferenced.h include
cp ../libgideros/gexport.h include
cp ../libgvfs/gfile.h include
cp ../lua/src/lua.h include
cp ../lua/src/luaconf.h include
cp ../lua/src/lualib.h include
cp ../lua/src/lauxlib.h include
cp ../libgid/include/gglobal.h include
cp ../libgvfs/gpath.h include
cp ../libgid/include/glog.h include
cp ../libgid/include/gapplication.h include
cp ../libgid/include/gevent.h include

cp ../libgvfs/libgvfs.1.dylib lib/desktop
cp ../libgideros/libgideros.1.dylib lib/desktop
cp ../lua/liblua.1.dylib lib/desktop
cp ../libgid/libgid.1.dylib lib/desktop

ln -s libgvfs.1.dylib libgvfs.dylib
ln -s libgideros.1.dylib libgideros.dylib
ln -s liblua.1.dylib liblua.dylib
ln -s libgid.1.dylib libgid.dylib
popd
}
