# Regional Ocean Modeling System (ROMS) - Bottom Tracer Version

**ROMS** s a numerical primitive equation ocean model, that uses terrain-following vertical and orthogonal-curvilinear horizontal coordinates. The ROMS model offers a variety of different configurations and adjustments as well as coupling with other models. Input files such as the grid configuration and boundary forcings can be supplied by common netCDF format files.

This code documents a modified version of the development branch of the [original source code](https://github.com/myroms/roms). It enables the simulation of a passive tracer which is inserted at the bottom of the model domain representing the input of a hydrothermal venting system. This version is used in the thesis [Plume Dispersal in the Arctic Ocean](https://doi.org/10.26092/elib/4441) but the modfications themselves where provided by G. Xu and originally used in [Xu G and German CR (2023)](10.3389/fmars.2023.1213470).

# Description
This repository is composed of two branches: _develop_ is a fork of the corresponding branch of the original source code. It is in the version where the modified version was last used and worked. _bottom-tracer_ is a branch of _develop_ including the modifications for the passive bottom tracer. In principle merging new commits from the original code into _bottom-tracer_ should be done, but it is not guaranteed that this will work without further adjustments.

The code in _bottom-tracer_ is modified at only one place: In the file [mod_ncparam.F](/ROMS/Modules/mod_ncparam.F) an additional section is inserted which adds the new type of variable, a passive tracer for bottom flux. For that see commit [3cd6757](https://github.com/jmetteUni/roms/commit/3cd6757725959a16f0d87d84e295ca5487dcc7fc).

This repository contains only the source code for the model, the application used to run the model in the thesis mentioned above is documented in separate [repository](https://github.com/jmetteUni/aurora-0-he). There you can find also a How-To and more information. For general information on the ROMS model check the official [wiki](https://www.myroms.org/wiki/Documentation_Portal) and the [forum](https://www.myroms.org/forum/).

