#!/bin/bash -x

dir=$(dirname $(test -L "$BASH_SOURCE" && readlink -f "$BASH_SOURCE" || echo "$BASH_SOURCE"))

sudo apt-get install qt5-default qttools5-dev-tools qdbus-qt5 qt5-qmake python-pyqt5 python3-pyqt5 pyqt5-dev-tools -y


PREFIX=/usr/local
GIDEROS_HOME=$HOME/workspace/gideros
QMAKE="$(which qmake) PREFIX=${PREFIX}"


pushd ${GIDEROS_HOME}/texturepacker
$QMAKE texturepacker.pro
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/fontcreator
$QMAKE fontcreator.pro
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/ui
$QMAKE ui.pro
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/player
$QMAKE player_qt5.pro
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/gdrdeamon
$QMAKE gdrdeamon.pro
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/gdrbridge
$QMAKE gdrbridge.pro
make
sudo make dist
popd

pushd ${GIDEROS_HOME}/gdrexport
$QMAKE gdrexport.pro
make
sudo make dist
popd
