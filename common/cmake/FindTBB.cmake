## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
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

IF (WIN32)
  FIND_PATH(TBB_ROOT include/tbb/task_scheduler_init.h
    DOC "Root of TBB installation"
    PATHS ${PROJECT_SOURCE_DIR}/tbb "C:/Program Files (x86)/Intel/Composer XE/tbb"
  )

  IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(TBB_ARCH intel64)
  ELSE()
    SET(TBB_ARCH ia32)
  ENDIF()

  IF (MSVC10)
    SET(TBB_VCVER vc10)
  ELSEIF (MSVC11)
    SET(TBB_VCVER vc11)
  ELSE()
    SET(TBB_VCVER vc12)
  ENDIF()

  SET(TBB_LIBDIR ${TBB_ROOT}/lib/${TBB_ARCH}/${TBB_VCVER})

  FIND_PATH(TBB_INCLUDE_DIR tbb/task_scheduler_init.h PATHS ${TBB_ROOT}/include)
  FIND_LIBRARY(TBB_LIBRARY tbb PATHS ${TBB_LIBDIR} NO_SYSTEM_ENVIRONMENT_PATH)
  FIND_LIBRARY(TBB_LIBRARY_MALLOC tbbmalloc PATHS ${TBB_LIBDIR} NO_SYSTEM_ENVIRONMENT_PATH)

  SET(TBB_DLLDIR ${TBB_ROOT}/../redist/${TBB_ARCH}/tbb/${TBB_VCVER})
  INSTALL(PROGRAMS ${TBB_DLLDIR}/tbb.dll ${TBB_DLLDIR}/tbbmalloc.dll DESTINATION bin COMPONENT redist)
ELSE ()
  SET(TBB_ROOT /opt/intel/composerxe/tbb/ CACHE PATH "Root of TBB installation")

  FIND_PATH(TBB_INCLUDE_DIR tbb/task_scheduler_init.h PATHS ${TBB_ROOT}/include)
  IF (APPLE)
    FIND_LIBRARY(TBB_LIBRARY tbb PATHS ${TBB_ROOT}/lib)
    FIND_LIBRARY(TBB_LIBRARY_MALLOC tbbmalloc PATHS ${TBB_ROOT}/lib)
  ELSE()
    FIND_LIBRARY(TBB_LIBRARY tbb PATHS ${TBB_ROOT}/lib/intel64/gcc4.4)
    FIND_LIBRARY(TBB_LIBRARY_MALLOC tbbmalloc PATHS ${TBB_ROOT}/lib/intel64/gcc4.4)
  ENDIF()

  FIND_PATH(TBB_INCLUDE_DIR_MIC tbb/task_scheduler_init.h NO_DEFAULT_PATH PATHS ${TBB_ROOT}/include)
  FIND_LIBRARY(TBB_LIBRARY_MIC tbb NO_DEFAULT_PATH PATHS ${TBB_ROOT}/lib/mic)
  FIND_LIBRARY(TBB_LIBRARY_MALLOC_MIC tbbmalloc NO_DEFAULT_PATH PATHS ${TBB_ROOT}/lib/mic)

  MARK_AS_ADVANCED(TBB_INCLUDE_DIR_MIC)
  MARK_AS_ADVANCED(TBB_LIBRARY_MIC)
  MARK_AS_ADVANCED(TBB_LIBRARY_MALLOC_MIC)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TBB DEFAULT_MSG TBB_INCLUDE_DIR TBB_LIBRARY TBB_LIBRARY_MALLOC)

IF (TBB_FOUND)
  SET(TBB_INCLUDE_DIRS ${TBB_INCLUDE_DIR})
  SET(TBB_LIBRARIES ${TBB_LIBRARY} ${TBB_LIBRARY_MALLOC})
ENDIF()

IF (TBB_INCLUDE_DIR AND TBB_LIBRARY_MIC AND TBB_LIBRARY_MALLOC_MIC)
  SET(TBB_FOUND_MIC TRUE)
  SET(TBB_INCLUDE_DIRS_MIC ${TBB_INCLUDE_DIR_MIC})
  SET(TBB_LIBRARIES_MIC ${TBB_LIBRARY_MIC} ${TBB_LIBRARY_MALLOC_MIC})
ENDIF()

MARK_AS_ADVANCED(TBB_INCLUDE_DIR)
MARK_AS_ADVANCED(TBB_LIBRARY)
MARK_AS_ADVANCED(TBB_LIBRARY_MALLOC)
