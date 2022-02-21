#!/bin/bash

git submodule init
git submodule update
mkdir -p plot
cp fcpp/src/extras/plotter/plot.asy plot/
if [ "$1" == "plots" ]; then
    fcpp/src/make.sh run -O -DNOTREE batch
    mv plot/batch.asy "plot/sphere batch.asy"
    mv plot/batch.pdf "plot/sphere batch.pdf"
    fcpp/src/make.sh run -O -DNOSPHERE batch
    mv plot/batch.asy "plot/tree batch.asy"
    mv plot/batch.pdf "plot/tree batch.pdf"
elif [ "$1" == "window" ]; then
    fcpp/src/make.sh gui run -O -DNOTREE -DGRAPHICS graphic
    mv plot/graphic.asy "plot/sphere graphic.asy"
    mv plot/graphic.pdf "plot/sphere graphic.pdf"
    fcpp/src/make.sh gui run -O -DNOSPHERE -DGRAPHICS graphic
    mv plot/graphic.asy "plot/tree graphic.asy"
    mv plot/graphic.pdf "plot/tree graphic.pdf"
else
    if [ "$1" == "" ]; then
        echo -e "\033[4msimplified usage:\033[0m"
        echo -e "    \033[1m./make.sh plots\033[0m                  produces plots through non-interactive batch runs"
        echo -e "    \033[1m./make.sh window\033[0m                 opens interactive windows for a spherical and tree scenario"
        echo
        echo -e "the number of batch runs can be tweaked through constant \033[1mruns\033[0m in \033[1mbatch.cpp\033[0m"
        echo
    fi
    fcpp/src/make.sh "$@"
fi
