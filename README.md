# Flow123d #

Flow123d is a simulator of underground water flow and transport processes in fractured
porous media. Novelty of this software is support of computations on complex
meshes consisting of simplicial elements of different dimensions. Therefore
we can combine continuum models and discrete fracture network models.
For more information see the project pages:
[flow123d.github.com](http://flow123d.github.com).
  
[![CI Build Status](http://ci2.nti.tul.cz/buildStatus/icon?job=Flow123d-linux-release-multijob)](http://ci2.nti.tul.cz/job/Flow123d-linux-release-multijob/)

## License ##

The source code of Flow123d is licensed under GPL3 license. For details look
at files doc/LICENSE and doc/GPL3.

## Build Flow123d ##

### Windows OS prerequisities ###

If you are running Windows, you have to install [Cygwin](https://www.cygwin.com/)  for emulation of
POSIX unix environment (both 32-bit and 64-bit version should work). Then all what follows has to be done in the directories under
cygwin, e.g "C:\cygwin\home\user\" and within the cygwin shell. In addition to the packages mentioned for the Linux you will need:

* openssh (for MPICH)
* some editor that can save Unix like text files (eg. notepad+ or sublime) 


### Linux OS prerequisities ###
Requested packages are: 

* gcc, g++                        C/C++ compiler in version 4.7 and higher (we use C++11)
* gfortran                        Fortran compiler for compilation of BLAS library for PETSc.
* python, perl                    Scripting languages 
* make, cmake (>2.8.8), patch       Building tools

Recommended packages for development: 
* libboost                        General purpose C++ library 

Namely you need development version of the Boost sub-libraries: "Program Options", "Serialize", "Regex", and "Filesystem".
In Debian/Ubuntu distributions, these are in separate packages:

* libboost-program-options-dev 
* libboost-serialize-dev
* libboost-regex-dev
* libboost-filesystem-dev 

Flow123d downloads and installs Boost during configuration if it is not found in the system, but it may
take long. Optionally you may need

* doxygen, graphviz     for source generated documentation and its dependency diagrams 
* texlive-latex         or other Latex package to build reference manual, we use also some extra packages:
                        on RedHat type distributions you may need texlive-cooltooltips, on Debian/Ubuntu 
			texlive-latex-extra
* imagemagick		tool is used to generate some graphics for the reference manual	

Flow123d depends also on PETSc library. It can be installed automatically during configuration,
but for good parallel performance it has to be configured manually see appropriate section later on.


### Compile Flow123 ###

Copy file `config.cmake.template` to `config.cmake`:

    > cp config.cmake.template config.cmake

Edit file `config.cmake`, set `PETSC_DIR` and `PETSC_ARCH` variables if you have your own installation of PETSc.
For further options see comments in the template file.

Then run the compilation by:

    > make all

This runs configuration and then the build process. If the configuration
fails and you need to correct your `config.cmake` or other system setting
you have to cleanup all files generated by unsuccessful cmake configuration by:

    > make clean-all

Try this every time if your build doesn't work and you don't know why.

** Parallel builds ** 

Build can take quite long time when running on single processor. Use make's parameter '-j N' to 
use N parallel processes. Developers can set appropriate alias in '.bashrc', e.g. :
    
    export NUMCPUS=`grep -c '^processor' /proc/cpuinfo`
    alias pmake='time nice make -j$NUMCPUS --load-average=$NUMCPUS'




### Manual PETSc installation (optional) ###

Flow versions 1.8.x depends on the PETSC library 3.4.x. It is installed automatically 
if you do not set an existing installation in `config.cmake` file. However the manual installation 
is necessary if
- you want to switch between more configurations (debugging, release, ...) using `PETSC_ARCH` variable.
- you want to achieve best performance configuring with system-wide MPI and/or BLAS and LAPACK libraries.


1.  download PETSc 3.4.x from:
    http://www.mcs.anl.gov/petsc/petsc-as/documentation/installation.html

2.  unpack to any working directory 
    and go to it, eg. :
 
        > cd /home/jb/local/petsc

3.  Set variables:

        > export PETSC_DIR=`pwd`

    For development you will need at least debugging build of the
    library. Set the name of configuration, eg. :

        > export PETSC_ARCH=linux-gcc-dbg

4. Run the configuration script, for example with following options:

        > ./config/configure.py --with-debugging=1 --CFLAGS-O=-g --FFLAGS-O=-g \
          --download-mpich=yes --download-metis=yes --download-parmetis --download-f-blas-lapack=1

    This also force cofigurator to install BLAS, Lapack, MPICH, and ParMetis so it takes
    a while (it could take about 15 min). If everything is OK, you obtain table with
    used compilers and libraries. 
    
5.  Finally compile PETSC with this configuration:

        > make all

    To test the compilation run:

        > make test

    To obtain PETSC configuration for the production version you can use e.g.

    ```
    > export PETSC_ARCH=linux-gcc-dbg
    > ./config/configure.py --with-debugging=0 --CFLAGS-O=-O3 --FFLAGS-O=-O3 \
       --download-mpich=yes --download-metis=yes --download-f-blas-lapack=1
    > make all
    > make test
    ```

### Notes: ###
* You can have several PETSC configuration using different `PETSC_ARCH` value.
* For some reasons if you let PETSc to download and install its own MPICH it
  overrides your optimization flags for compiler. Workaround is to edit
  file `${PETSC_DIR}/${PETSC_ARCH}/conf/petscvariables` and modify variables
  `<complier>_FLAGS_O` back to the values you wish.
* PETSc configuration should use system wide MPI implementation for efficient parallel computations.
* You have to compile PETSc at least with Metis support.

### Support for other libraries ##
PETSc supports lot of other software packages. Use `configure.py --help` for the full list. Here we
highlight the packages we are familiar with.

* **MKL** is implementation of BLAS and LAPACK libraries provided by Intel for their processors. 
  Usually gives the best performance. Natively supported by PETSc.

* **ATLAS library** PETSC use BLAS and few LAPACK functions for its local vector and matrix
  operations. The speed of BLAS and LAPACK have dramatic impact on the overall
  performance. There is a sophisticated implementation of BLAS called ATLAS.
  ATLAS performs extensive set of performance tests on your hardware then make
  an optimized implementation of  BLAS code for you. According to our
  measurements the Flow123d is about two times faster with ATLAS compared to
  usual --download-f-blas-lapack (on x86 architecture and usin GCC).
   
  In order to use ATLAS, download it from ... and follow their instructions.
  The key point is that you have to turn off the CPU throttling. To this end
  install 'cpufreq-set' or `cpu-freq-selector` and use it to set your processor
  to maximal performance:
  
    cpufreq-set -c 0 -g performance
    cpufreq-set -c 1 -g performance

   ... this way I have set performance mode for both cores of my Core2Duo.

   Then you need not to specify any special options, just run default configuration and make. 
   
   Unfortunately, there is one experimental preconditioner in PETSC (PCASA) which use a QR decomposition Lapack function, that is not
   part of ATLAS. Although it is possible to combine ATLAS with full LAPACK from Netlib, we rather provide an empty QR decomposition function
   as a part of Flow123d sources.
   See. HAVE_ATTLAS_ONLY_LAPACK in ./makefile.in

* PETSC provides interface to many useful packages. You can install them 
  adding further configure options:

  --download-superlu=yes         # parallel direct solver
  --download-hypre=yes           # Boomer algebraic multigrid preconditioner, many preconditioners
  --download-spools=yes          # parallel direc solver
  --download-blacs=ifneeded      # needed by MUMPS
  --download-scalapack=ifneeded  # needed by MUMPS
  --download-mumps=yes           # parallel direct solver
  --download-umfpack=yes         # MATLAB solver

  For further information about use of these packages see:

  http://www.mcs.anl.gov/petsc/petsc-2/documentation/linearsolvertable.html

  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/PC/PCFactorSetMatSolverPackage.html#PCFactorSetMatSolverPackage
  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/Mat/MAT_SOLVER_SPOOLES.html#MAT_SOLVER_SPOOLES
  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/Mat/MAT_SOLVER_MUMPS.html#MAT_SOLVER_MUMPS
  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/Mat/MAT_SOLVER_SUPERLU_DIST.html
  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-current/docs/manualpages/Mat/MAT_SOLVER_UMFPACK.html

  http://www.mcs.anl.gov/petsc/petsc-as/snapshots/petsc-dev/docs/manualpages/PC/PCHYPRE.html

## Build the reference manual ##

The reference manual can be built by

    > make ref-doc

which calls LaTeX commands to make the final pdf file `doc/reference_manual/flow123d_doc.pdf`.
The LaTeX source files depends on several packages that are needed for succesful build. In the list below 
we provide a hint to find these packages (and names of distribution packages if you are using TeXLive under Ubuntu):

* hyperref
* colortbl
* amsmath
* natbib
* graphicx
* array (in CTAN package tools)
* tabularx (in CTAN package tools)
* multicol (in CTAN package tools)
* longtable (in CTAN package tools)
* pdflscape (in several distributions include in package bundle 'oberdiek')
 
which can be found in Ubuntu in 'texlive-latex-base' package

* caption
* subcaption
* fancyvrb
* rotating

which can be found in Ubuntu in 'texlive-latex-recommended' package

* amssymb (in CTAN package amsfonts)

which can be found in Ubuntu in 'texlive-base' package

* etoolbox

which can be found in Ubuntu in 'texlive-latex-extra' package, but is also enclosed in Flow123d reference manual source directory

All these packages can be also obtained from [CTAN-Comprehensive TeX Archive Network](http://ctan.org/).


## Troubleshooting ##

* **Petsc Detection Problem:**

  CMake try to detect type of your PETSc installation and then test it
  by compiling and running simple program. However, this can fail if the 
  program has to be started under 'mpiexec'. In such a case, please, set:

    > set (PETSC_EXECUTABLE_RUNS YES)

  in your makefile.in.cmake file, and perform: `make clean-all; make all`

  For further information about program usage see reference manual `doc/flow_doc`.


* ** Shared libraries ** By default PETSC will create dynamically linked libraries, which can be shared be more applications. But on some systems 
  (in particular we have problems under Windows) this doesn't work, so one is forced to turn off dynamic linking by:

    --with-shared=0

* ** No MPI ** If you want only serial version of PETSc (and Flow123d)
  add --with-mpi=0 to the configure command line.


* ** Windows line ends**  If you use a shell script for PETSC configuration under cygwin,
  always check if you use UNIX line ends. It can be specified in the notepad
  of Windows 7.

* ** older Windows ** For some older Windows versions it may be necessary to set in configuration of PETSC
    --with-timer=nt 

* ** Cygwin remap problem ** You may get strange errors during configuration of PETSc, like 

    C:\cygwin\bin\python.exe: *** unable to remap C:\cygwin\bin\cygssl.dll to same address as parent(0xDF0000) != 0xE00000

  or other errors usually related to DLL conflicts.
  (see http://www.tishler.net/jason/software/rebase/rebase-2.4.2.README)
  To fix DLL libraries you should perform:

 -# shutdown all Cygwin processes and services
 -# start `ash` (do not use bash or rxvt)
 -# execute `/usr/bin/rebaseall` (in the ash window)

  Possible problem with 'rebase':
    /usr/lib/cygicudata.dll: skipped because nonexist
    .
    .
    .
    FixImage (/usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_sjlj-1.dll) failed with last error = 13

   Solution (ATTENTION, depends on Cygwin version): 
    add following to line 110 in the rebase script:
    -e '/\/sys-root\/mingw\/bin/d'



