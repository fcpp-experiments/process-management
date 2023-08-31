# FCPP Process Management

Project for developing and demonstrating process management techniques.

All commands below are assumed to be issued from the cloned git repository folder.
For any issues with reproducing the experiments, please contact [Giorgio Audrito](mailto:giorgio.audrito@unito.it).

## References

- FCPP main website: [https://fcpp.github.io](https://fcpp.github.io).
- FCPP documentation: [http://fcpp-doc.surge.sh](http://fcpp-doc.surge.sh).
- FCPP presentation paper: [http://giorgio.audrito.info/static/fcpp.pdf](http://giorgio.audrito.info/static/fcpp.pdf).
- FCPP sources: [https://github.com/fcpp/fcpp](https://github.com/fcpp/fcpp).


## Setup

The next sections contain the setup instructions based on the CMake build system for the various supported OSs and virtual containers. Jump to the section dedicated to your system of choice and ignore the others.

### Windows

Pre-requisites:
- [MSYS2](https://www.msys2.org)
- [Asymptote](http://asymptote.sourceforge.io) (for building the plots)
- [Doxygen](http://www.doxygen.nl) (for building the documentation)

At this point, run "MSYS2 MinGW x64" from the start menu; a terminal will appear. Run the following commands:
```
pacman -Syu
```
After updating packages, the terminal will close. Open it again, and then type:
```
pacman -Sy --noconfirm --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-make git
```
The build system should now be available from the "MSYS2 MinGW x64" terminal.

### Linux

Pre-requisites:
- Xorg-dev package (X11)
- G++ 9 (or higher)
- CMake 3.18 (or higher)
- Asymptote (for building the plots)
- Doxygen (for building the documentation)

To install these packages in Ubuntu, type the following command:
```
sudo apt-get install xorg-dev g++ cmake asymptote doxygen
```
In Fedora, the `xorg-dev` package is not available. Instead, install the packages:
```
libX11-devel libXinerama-devel.x86_6 libXcursor-devel.x86_64 libXi-devel.x86_64 libXrandr-devel.x86_64 mesa-libGL-devel.x86_64
```

### MacOS

Pre-requisites:
- Xcode Command Line Tools
- CMake 3.18 (or higher)
- Asymptote (for building the plots)
- Doxygen (for building the documentation)

To install them, assuming you have the [brew](https://brew.sh) package manager, type the following commands:
```
xcode-select --install
brew install cmake asymptote doxygen
```

### Graphical User Interface

Executing a graphical simulation will open a window displaying the simulation scenario, initially still: you can start running the simulation by pressing `P` (current simulated time is displayed in the bottom-left corner). While the simulation is running, network statistics may be periodically printed in the console, and be possibly aggregated in form of an Asymptote plot at simulation end. You can interact with the simulation through the following keys:

- `Esc` to end the simulation
- `P` to stop/resume
- `O`/`I` to speed-up/slow-down simulated time
- `L` to show/hide connection links between nodes
- `G` to show/hide the grid on the reference plane and node pins
- `M` enables/disables the marker for selecting nodes
- `left-click` on a selected node to open a window with node details
- `C` resets the camera to the starting position
- `Q`,`W`,`E`,`A`,`S`,`D` to move the simulation area along orthogonal axes
- `right-click`+`mouse drag` to rotate the camera
- `mouse scroll` for zooming in and out
- `left-shift` added to the camera commands above for precision control
- any other key will show/hide a legenda displaying this list

Hovering on a node will also display its UID in the top-left corner.

## Simulations

### Graphic 

```./make.sh window```

Runs a single test simulation with GUI for each of the following scenarios:
- spherical topology
- tree topology
- tree topology exploiting Bloom filters

PDF plots will be produced in the `plot/` directory, and the textual output will be saved in the `output/` directory.

#### Graphical description

- **TODO:** describe graphical notation

#### Parameters (cf. plots)

- **dens** density of the network as avg number of neighbours
- **hops** network diameter
- **speed** maximum speed of devices as a percentage of the communication speed
- **tvar** variance of the round durations, as a percentage of the avg

#### Metrics (cf. plots)

- `dcount` (delivery count): % of messages that arrived to destination 
- `aproc` (average processes): average number of process instances (i.e., for a single process, the average number of devices running it)
- `asiz` (average size) 
- `mmsiz` (max message size)
- `adel` (average delay)

See also the namespace `tag` in file `lib/generals.hpp` (where, e.g., struct `max_msg_size` turns into extracted metric `mmsize`).

### Case Study

```./make.sh gui run -O -DGRAPHIC [-DBLOOM] case_study```

The essence of the Case Study (target ```case_study```) consists of the following scenario, based on a network of nodes:

- when idle, a node _n_ may decide to broadcast a discovery message for a service _S_
- each node _m_ offering service _S_ replies with an _offer_ (each node in the network implements exactly one service taken from a set {S1, ..., Sk})
- if node _n_ does not receive an offer for service _S_ within a given interval, its wait times out and it returns to an idle state
- the _first_ offer received by _n_ is accepted and an accept message is sent to its sender _m_; the other offers are _ignored_
- upon the acceptance of its offer, node _m_ starts sending a _file_ (sequence) of _N_ data messages to node _n_, sending _one message per round_ (currently _N=1_)
- after a given interval, the nodes whose offers are ignored time out and return to an idle state
- after sending the last (only) message, _m_ returns to an idle state
- after recognizing that it has received the last message, _n_ returns to an idle state

Overall, the above steps are summarized by the following state machine:

![nodes-automa](https://github.com/fcpp-experiments/process-management/assets/1214215/65f1cbb6-2db8-42bf-b968-8de679e87d60)

#### Graphical description

- **TODO:** describe graphical notation

#### Configuration

The case study can be _configured_ through many settings:

- basic settings in ```case_study.cpp```. These settings are the ones that are varied in the _systematic tests_.
  - **dens** density of the network as avg number of neighbours
  - **hops** network diameter
  - **speed** maximum speed of devices as a percentage of the communication speed
  - **tvar** variance of the round durations, as a percentage of the avg
- further settings in  ```simulation_setup.hpp```. These settings are shared with the targets ```graphic``` and ```batch``` of the _systematic tests_.
  - **period** avg duration of a round
  - **comm** communication radius
  - **end** end of simulated time
  - **timeout_coeff** coefficient to be multiplied to **hops** to get the timeout value
  - **max_svc_id** number _k_ of different services {S1,...,Sk}
  - **max_file_size** maximum size (in messages) of the file sent by service to client (currently ***ignored*** since file size is fixed to _1_)

#### Restrictions

The following are current restrictions to the scenario that may be lifted in future versions.

- at any round, each device is in exactly one of the poassible states _IDLE_, _DISCO_, _OFFER_, _SERVED_, _SERVING_ of the above state machine. This means that a device that is, e.g., *OFFER*-ing its service cannot at the same being _SERVING_ some previous request, etc.
- only _one discovery message_ is actually generated during the _whole use case execution_. The request is created by the device with id _(N-1)_ (where _N_ is the number of devices) at time (round) _T=11_ (this delay has the purpose to let the tree topology computation stabilize)
- the offer accepted is always the first one received by a device in the _DISCO_ state; in case two or more offers are received at the same time by a device in _DISCO_ state, one of them is arbitrarily chosen to be accepted
- the file sent by a service after its offer has been accepted has a length of exactly _1 message_

