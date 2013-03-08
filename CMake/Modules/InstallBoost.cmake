#InstallBoost.cmake
#
# Created on: Jul 20, 2012
#     Author: jb
#

if (NOT EXTERNAL_BOOST_DIR)
    set(EXTERNAL_BOOST_DIR "${PROJECT_BINARY_DIR}/boost_build")
endif()    
    
# Run

# A temporary CMakeLists.txt
set (cmakelists_fname "${EXTERNAL_BOOST_DIR}/CMakeLists.txt")
file (WRITE "${cmakelists_fname}"
"
  ## This file was autogenerated by InstallBoost.cmake
  cmake_minimum_required(VERSION 2.8)
  include(ExternalProject)
  ExternalProject_Add(Boost
    DOWNLOAD_DIR ${EXTERNAL_BOOST_DIR} 
    URL \"http://bacula.nti.tul.cz/~jan.brezina/flow123d_libraries/boost_1_50_0.tar.gz\"
    SOURCE_DIR ${EXTERNAL_BOOST_DIR}/src
    BINARY_DIR ${EXTERNAL_BOOST_DIR}/src
    CONFIGURE_COMMAND ${EXTERNAL_BOOST_DIR}/src/bootstrap.sh --prefix=${EXTERNAL_BOOST_DIR} --with-libraries=program_options,serialization
    BUILD_COMMAND ${EXTERNAL_BOOST_DIR}/src/b2 install
    INSTALL_COMMAND \"\"
  )
")

# run cmake
execute_process(COMMAND ${CMAKE_COMMAND} ${EXTERNAL_BOOST_DIR} 
    WORKING_DIRECTORY ${EXTERNAL_BOOST_DIR})

find_program (MAKE_EXECUTABLE NAMES make gmake)
# run make
execute_process(COMMAND ${MAKE_EXECUTABLE} Boost
    WORKING_DIRECTORY ${EXTERNAL_BOOST_DIR})    


file (REMOVE ${cmakelists_fname})

message(STATUS "BOOST build done")