cmake_minimum_required(VERSION 2.6)

project (saker C)

option(ENABLE_DEBUG
    "Set to OFF|ON (default is OFF) to control Debug or Release" ON)

option(ENABLE_TESTS
    "Set to OFF|ON (default is OFF) to control build of tests " ON)

set(CMAKE_USE_RELATIVE_PATHS TRUE)


if (NOT CMAKE_BUILD_TYPE)

  if (ENABLE_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
  else ()
    set(CMAKE_BUILD_TYPE Release)  
  endif ()

endif ()
  
if (UNIX)

  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
  set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
  set(CMAKE_C_FLAGS_DEBUG "-O0 -Wall -g")
  set(CMAKE_C_FLAGS_RELEASE "-O2 -Wall -DNDEBUG")
  
elseif (MSVC)

  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  add_definitions("-D_WIN32IOCP")
  add_definitions("-D__x86_64__")
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
  set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
  
else ()

  message(FATAL_ERROR,"must use msvc in windows or gcc in unix")
    
endif ()


if (ENABLE_TESTS)

    include(CTest)
    ENABLE_TESTING()
    message(STATUS "Building with tests ")
  
else ()

    message(STATUS "Building without tests ")
    
endif ()


add_subdirectory(deps)
add_subdirectory(src)
add_subdirectory(src/client)


message(STATUS "CMake ${CMAKE_VERSION} successfully configured ${PROJECT_NAME} using ${CMAKE_GENERATOR} generator")
message(STATUS "Installation target path: ${CMAKE_INSTALL_PREFIX}")
#message(STATUS "C_FLAGS:  =${CMAKE_C_FLAGS}")
#message(STATUS "CXX_FLAGS:=${CMAKE_CXX_FLAGS}")

#add_test(NAME PreScheduler WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/PreScheduler-testrunner -all)

                     
