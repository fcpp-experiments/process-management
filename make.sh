#!/bin/bash

git submodule init
git submodule update
mkdir -p plot
cp fcpp/src/extras/plotter/plot.asy plot/
if [ "$1" == "plots" ]; then
    fcpp/src/make.sh run -O -DNOTREE batch
    cat plot/batch.asy | sed 's|plot.ROWS = 1|plot.ROWS = 5|g' > "plot/sphere batch.asy"
    fcpp/src/make.sh run -O -DNOSPHERE batch
    cat plot/batch.asy | sed 's|plot.ROWS = 1|plot.ROWS = 5|g' > "plot/tree batch.asy"
    fcpp/src/make.sh run -O -DNOSPHERE -DBLOOM batch
    cat plot/batch.asy | sed 's|plot.ROWS = 1|plot.ROWS = 5|g' > "plot/bloom batch.asy"
    rm plot/batch.{asy,pdf}
    cd plot
    asy -mask {sphere,tree,bloom}" batch.asy" -f pdf
    cd ..
elif [ "$1" == "window" ]; then
    fcpp/src/make.sh gui run -O -DNOTREE -DGRAPHICS graphic
    cat plot/graphic.asy | sed 's|plot.ROWS = 1|plot.ROWS = 5|g' > "plot/sphere graphic.asy"
    fcpp/src/make.sh gui run -O -DNOSPHERE -DGRAPHICS graphic
    cat plot/graphic.asy | sed 's|plot.ROWS = 1|plot.ROWS = 5|g' > "plot/tree graphic.asy"
    fcpp/src/make.sh gui run -O -DNOSPHERE -DBLOOM -DGRAPHICS graphic
    cat plot/graphic.asy | sed 's|plot.ROWS = 1|plot.ROWS = 5|g' > "plot/bloom graphic.asy"
    rm plot/graphic.{asy,pdf}
    cd plot
    asy -mask {sphere,tree,bloom}" graphic.asy" -f pdf
    cd ..
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
