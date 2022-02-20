#!/bin/bash

git submodule init
git submodule update
mkdir -p plot
cp fcpp/src/extras/plotter/plot.asy plot/
if [ "$1" == "plots" ]; then
    fcpp/src/make.sh gui run -O -DNOTREE batch
    mv plot/batch.asy "plot/sphere batch.asy"
    mv plot/batch.pdf "plot/sphere batch.pdf"
    fcpp/src/make.sh gui run -O -DNOSPHERE batch
    mv plot/batch.asy "plot/tree batch.asy"
    mv plot/batch.pdf "plot/tree batch.pdf"
else
    fcpp/src/make.sh "$@"
fi
