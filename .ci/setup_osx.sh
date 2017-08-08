#!/usr/bin/env bash

source setup_common.sh


function install {
	brew install $*
}

brew update --quiet
#brew install cln doxygen eigen llvm
brew install cln doxygen llvm

if [[ ${USE} == "g++-4.8" ]]; then
	echo "g++-4.8 is not supported"
	#install gcc-4.8 g++-4.8
	#defCXX gcc-4.8 g++-4.8
elif [[ ${USE} == "g++-5" ]]; then
	install gcc@5
	brew link --overwrite gcc@5
	defCXX gcc-5 g++-5
elif [[ ${USE} == "g++-6" ]]; then
	install gcc@6
	brew link --overwrite gcc@6
	defCXX gcc-6 g++-6
elif [[ ${USE} == "clang++-3.4" ]]; then
	echo "clang++-3.4 is not supported"
	#install clang-3.4
	#defCXX clang-3.4 clang++-3.4
elif [[ ${USE} == "clang++-3.5" ]]; then
	defCXX clang clang++
fi
