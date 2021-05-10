#!/bin/csh -f
#
# git $Id$
# svn $Id$
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# Copyright (c) 2002-2021 The ROMS/TOMS Group                           :::
#::::::::::::::::::::::::::::::::::::::::::::::::::::: Hernan G. Arango :::
#                                                                       :::
# WPS Compiling CSH Script                                              :::
#                                                                       :::
# Script to compile WPS where source code files are kept separate       :::
# from the application configuration and build objects.                 :::
#                                                                       :::
# Q: How/why does this script work?                                     :::
#                                                                       :::
# A:                                                                    :::
#                                                                       :::
# Usage:                                                                :::
#                                                                       :::
#    ./build_wps.sh [options]                                           :::
#                                                                       :::
# Options:                                                              :::
#                                                                       :::
#    -j [N]      Compile in parallel using N CPUs                       :::
#                  omit argument for all available CPUs                 :::
#                                                                       :::
#    -move       Move compiled objects to build directory               :::
#                                                                       :::
#    -noclean    Do not run clean -a script                             :::
#                                                                       :::
#    -noconfig   Do not run configure compilation script                :::
#                                                                       :::
#                                                                       :::
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

setenv which_MPI openmpi                        # default, overwriten below

# Initialize.

set separator = `perl -e "print ':' x 100;"`

set parallel = 0
set clean = 1
set config = 1
set move = 0

setenv CPLFLAG ''
setenv MY_CPP_FLAGS ''

while ( ($#argv) > 0 )
  switch ($1)
    case "-j"
      shift
      set parallel = 1
      if (`echo $1 | grep '^[0-9]\+$'` != "" ) then
        set NCPUS = $1
        shift
      else
        set NCPUS = 2
      endif
    breaksw

    case "-noclean"
      shift
      set clean = 0
    breaksw

    case "-noconfig"
      shift
      set config = 0
    breaksw

    case "-move"
      shift
      set move = 1
    breaksw

    case "-*":
      echo ""
      echo "$0 : Unknown option [ $1 ]"
      echo ""
      echo "Available Options:"
      echo ""
      echo "-j [N]        Compile in parallel using N CPUs"
      echo ""
      echo "-move         Move compiled objects to build directory"
      echo ""
      echo "-noclean      Do not run clean script"
      echo ""
      echo "-noconfig     Do not run configure compilation script"
      echo ""
     exit 1
    breaksw

  endsw
end

#--------------------------------------------------------------------------
# Set a local environmental variable to define the root path to the
# directories where ROMS and WPS source files are located.  The ROMS
# source directory is needed for replacing several WRF files for
# ESMF/NUOPC Coupling (see below).
#--------------------------------------------------------------------------

 setenv ROMS_SRC_DIR         ${HOME}/ocean/repository/coupling

 setenv WPS_ROOT_DIR         ${HOME}/ocean/repository/WPS
 setenv WPS_SRC_DIR          ${WPS_ROOT_DIR}

#--------------------------------------------------------------------------
# Set a local environmental variable to define the path of the working
# application directory where all this project's files are kept.
#--------------------------------------------------------------------------

 setenv MY_PROJECT_DIR       ${PWD}
 setenv MY_PROJECT_DATA      `dirname ${PWD}`/Data

#--------------------------------------------------------------------------
# COAMPS configuration CPP options.
#--------------------------------------------------------------------------

# Sometimes it is desirable to activate one or more CPP options to
# configure a particular application. If it is the case, specify each
# option here using the -D syntax. Notice also that you need to use
# shell's quoting syntax to enclose the definition. Both single or
# double quotes work. For example,
#
#setenv MY_CPP_FLAGS         "${MY_CPP_FLAGS} -DHDF5"
#
# can be used to read input data from HDF5 file instead of flat files.
# Notice that you can have as many definitions as you want by appending
# values.

#setenv MY_CPP_FLAGS         "${MY_CPP_FLAGS} -DHDF5"

#--------------------------------------------------------------------------
# Set Fortran compiler and MPI library to use.
#--------------------------------------------------------------------------

# Set path of the directory containing makefile configuration (*.mk) files.
# The user has the option to specify a customized version of these files
# in a different directory than the one distributed with the source code,
# ${COAMPS_ROOT_DIR}/compilers. If this is the case, the you need to keep
# these configurations files up-to-date.

 setenv USE_MPI              on          # distributed-memory parallelism
 setenv USE_MPIF90           on          # compile with mpif90 script
#setenv which_MPI            mpich       # compile with MPICH library
#setenv which_MPI            mpich2      # compile with MPICH2 library
#setenv which_MPI            mvapich2    # compile with MVAPICH2 library
 setenv which_MPI            openmpi     # compile with OpenMPI library

 setenv FORT                 ifort
#setenv FORT                 gfortran
#setenv FORT                 pgi

 setenv USE_REAL_DOUBLE      on          # use real double precision (-r8)
#setenv USE_DEBUG            on          # use Fortran debugging flags
 setenv USE_HDF5             on          # compile with HDF5 library
 setenv USE_NETCDF           on          # compile with NetCDF
 setenv USE_NETCDF4          on          # compile with NetCDF-4 library
                                         # (Must also set USE_NETCDF)

#--------------------------------------------------------------------------
# Use my specified library paths. It is not needed but it is added for
# for checking in the future.
#--------------------------------------------------------------------------

 setenv USE_MY_LIBS no           # use system default library paths
#setenv USE_MY_LIBS yes          # use my customized library paths

 set MY_PATHS = ${ROMS_SRC_DIR}/Compilers/my_build_paths.sh
#set MY_PATHS = ${HOME}/Compilers/ROMS/my_build_paths.sh

if ($USE_MY_LIBS == 'yes') then
  source ${MY_PATHS} ${MY_PATHS}
endif

#--------------------------------------------------------------------------
# WPS build and executable directory.
#--------------------------------------------------------------------------

# Put the *.a, .f and .f90 files in a project specific Build directory to
# avoid conflict with other projects.

if ($?USE_DEBUG) then
  setenv  WPS_BUILD_DIR   ${MY_PROJECT_DIR}/Build_wpsG
else
  setenv  WPS_BUILD_DIR   ${MY_PROJECT_DIR}/Build_wps
endif

# Put WPS executables in the following directory.

setenv  WPS_BIN_DIR       ${WPS_BUILD_DIR}/Bin

# Go to the users source directory to compile. The options set above will
# pick up the application-specific code from the appropriate place.

 cd ${WPS_ROOT_DIR}

#--------------------------------------------------------------------------
# Configure. It creates configure.wps script used for compilation.
#
#   -d    build with debugging information and no optimization
#   -D    build with -d AND floating traps, traceback, uninitialized variables
#   -r8   build with 8 byte reals
#
# During configuration the WPS/arch/Config.pl perl script is executed and
# we need to interactively select the combination of compiler and parallel
# comunications option. For example, for Darwin operating system we get:
#
# Please select from among the following Darwin ARCH options:
#
#  1. (serial)   2. (smpar)   3. (dmpar)   4. (dm+sm)   PGI (pgf90/pgcc)
#  5. (serial)   6. (smpar)   7. (dmpar)   8. (dm+sm)   INTEL (ifort/icc)
#  9. (serial)  10. (smpar)  11. (dmpar)  12. (dm+sm)   INTEL (ifort/clang)
# 13. (serial)               14. (dmpar)                GNU (g95/gcc)
# 15. (serial)  16. (smpar)  17. (dmpar)  18. (dm+sm)   GNU (gfortran/gcc)
# 19. (serial)  20. (smpar)  21. (dmpar)  22. (dm+sm)   GNU (gfortran/clang)
# 23. (serial)               24. (dmpar)                IBM (xlf90_r/cc)
# 25. (serial)  26. (smpar)  27. (dmpar)  28. (dm+sm)   PGI (pgf90/pgcc): -f90=pgf90
# 29. (serial)  30. (smpar)  31. (dmpar)  32. (dm+sm)   INTEL (ifort/icc): Open MPI
# 33. (serial)  34. (smpar)  35. (dmpar)  36. (dm+sm)   INTEL/GNU (ifort/gcc): Open MPI
# 37. (serial)  38. (smpar)  39. (dmpar)  40. (dm+sm)   GNU (gfortran/gcc): Open MPI
#
# Enter selection [1-40] : ??
#
# For coupling with ESMF/NUOPC, we need select an option from the (dmpar)
# column for distributed-memory configuration.
#--------------------------------------------------------------------------

# Clean source code and remove build directory.

if ( $clean == 1 ) then
  echo ""
  echo "${separator}"
  echo "Cleaning WPS source code:  ${WPS_ROOT_DIR}/clean -a"
  echo "${separator}"
  echo ""
  ${WPS_ROOT_DIR}/clean -a            # clean source code
  /bin/rm -rf ${WPS_BUILD_DIR}        # remove existing build directories
endif

if ($?USE_DEBUG) then
   set DEBUG_FLAG = -d
#  set DEBUG_FLAG = -D
endif

setenv CONFIG_FLAGS ''

if ( $config == 1 ) then
  if ($?USE_DEBUG && $?USE_REAL_DOUBLE) then
    setenv CONFIG_FLAGS   "${DEBUG_FLAG} -r8"
  else if ($?USE_DEBUG) then
    setenv CONFIG_FLAGS   ${DEBUG_FLAG}
  else if ($?USE_REAL_DOUBLE) then
    setenv CONFIG_FLAGS   -r8
  endif

  echo ""
  echo "${separator}"
  echo "Configuring WPS code:  ${WPS_ROOT_DIR}/configure ${CONFIG_FLAGS}"
  echo "${separator}"
  echo ""

  ${WPS_ROOT_DIR}/configure ${CONFIG_FLAGS}

endif

#--------------------------------------------------------------------------
# WPS Compile script:
#
# Usage:
#
#     compile [-j n] wrf   compile wrf in run dir (NOTE: no real.exe,
#                          ndown.exe, or ideal.exe generated)
#
#   or choose a test case (see README_test_cases for details) :
#
#     compile [-j n] em_b_wave
#     compile [-j n] em_convrad
#     compile [-j n] em_esmf_exp
#     compile [-j n] em_fire
#     compile [-j n] em_grav2d_x
#     compile [-j n] em_heldsuarez
#     compile [-j n] em_hill2d_x
#     compile [-j n] em_les
#     compile [-j n] em_quarter_ss
#     compile [-j n] em_real
#     compile [-j n] em_scm_xy
#     compile [-j n] em_seabreeze2d_x
#     compile [-j n] em_squall2d_x
#     compile [-j n] em_squall2d_y
#     compile [-j n] em_tropical_cyclone
#     compile [-j n] nmm_real
#     compile [-j n] nmm_tropical_cyclone
#
#     compile -j n         parallel make using n tasks if supported
#                          (default 2)
#
#     compile -h           help message
#--------------------------------------------------------------------------

# Compile (the binary will go to BINDIR set above).

if ( $parallel == 1 ) then
  setenv J "-j ${NCPUS}"
else
  setenv J "-j 2"
endif

echo ""
echo "${separator}"
echo "Compiling WPS using  ${MY_PROJECT_DIR}/${0}:"
echo ""
echo "   ${WPS_ROOT_DIR}/compile"
echo "        J = ${J},          number of compiling CPUs"
echo "${separator}"
echo ""

${WPS_ROOT_DIR}/compile

#--------------------------------------------------------------------------
# Move WPS objects and executables.
#--------------------------------------------------------------------------

if ( $move == 1 ) then

  echo ""
  echo "${separator}"
  echo "Moving WPS objects to Build directory  ${WPS_BUILD_DIR}:"
  echo "${separator}"
  echo ""

  if ( ! -d ${WPS_BUILD_DIR} ) then
    /bin/mkdir -pv ${WPS_BUILD_DIR}
    /bin/mkdir -pv ${WPS_BUILD_DIR}/Bin
    echo ""
  endif

  /bin/cp -pfv configure.wps ${WPS_BUILD_DIR}

  echo "WPS_ROOT_DIR = ${WPS_ROOT_DIR}"
  find ${WPS_ROOT_DIR} -type f -name "*.exe" -exec /bin/mv -fv {} ${WPS_BUILD_DIR}/Bin \;

endif