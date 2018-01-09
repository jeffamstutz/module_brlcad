## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

if (NOT BRLCAD_ROOT)
  set(BRLCAD_ROOT $ENV{BRLCAD_ROOT})
endif()

find_path(BRLCAD_ROOT include/brlcad/bu.h
  DOC "Root of BRLCAD installation"
  HINTS ${BRLCAD_ROOT}
  PATHS
    /opt/brlcad
    /usr/brlcad
)

find_path(BRLCAD_INCLUDE_DIR bu.h PATHS ${BRLCAD_ROOT}/include/brlcad NO_DEFAULT_PATH)
find_library(BRLCAD_BU_LIBRARY bu PATHS ${BRLCAD_ROOT}/lib NO_DEFAULT_PATH)
find_library(BRLCAD_BN_LIBRARY bn PATHS ${BRLCAD_ROOT}/lib NO_DEFAULT_PATH)
find_library(BRLCAD_ON_LIBRARY openNURBS PATHS ${BRLCAD_ROOT}/lib NO_DEFAULT_PATH)
find_library(BRLCAD_RT_LIBRARY rt PATHS ${BRLCAD_ROOT}/lib NO_DEFAULT_PATH)

set(BRLCAD_ERROR_MESSAGE 
"Could not find BRLCAD! Please set BRLCAD_ROOT to to point to your BRLCAD installation")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BRLCAD
  ${BRLCAD_ERROR_MESSAGE}
  BRLCAD_INCLUDE_DIR BRLCAD_BN_LIBRARY BRLCAD_BU_LIBRARY BRLCAD_RT_LIBRARY
)

if (BRLCAD_FOUND)
  set(BRLCAD_INCLUDE_DIRS
    ${BRLCAD_INCLUDE_DIR}/../openNURBS
    ${BRLCAD_INCLUDE_DIR}/..
    ${BRLCAD_INCLUDE_DIR}
  )
  set(BRLCAD_LIBRARIES
    ${BRLCAD_BU_LIBRARY}
    ${BRLCAD_BN_LIBRARY}
    ${BRLCAD_ON_LIBRARY}
    ${BRLCAD_RT_LIBRARY}
  )
endif()

mark_as_advanced(BRLCAD_INCLUDE_DIRS)
mark_as_advanced(BRLCAD_BU_LIBRARY)
mark_as_advanced(BRLCAD_BN_LIBRARY)
mark_as_advanced(BRLCAD_ON_LIBRARY)
mark_as_advanced(BRLCAD_RT_LIBRARY)
