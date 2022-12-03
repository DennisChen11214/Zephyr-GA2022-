# Install script for directory: C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/ODE")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/Debug/ode_singled.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/Release/ode_single.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/MinSizeRel/ode_single.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/RelWithDebInfo/ode_single.lib")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xruntimex" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/Debug/ode_singled.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/Release/ode_single.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/MinSizeRel/ode_single.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/RelWithDebInfo/ode_single.dll")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdebugx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/Debug/ode_singled.pdb")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/RelWithDebInfo/ode_single.pdb")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ode" TYPE FILE FILES
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/collision.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/collision_space.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/collision_trimesh.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/common.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/compatibility.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/contact.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/cooperative.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/error.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/export-dif.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/mass.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/matrix.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/matrix_coop.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/memory.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/misc.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/objects.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/ode.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/odeconfig.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/odecpp.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/odecpp_collision.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/odeinit.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/odemath.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/odemath_legacy.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/rotation.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/threading.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/threading_impl.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/include/ode/timer.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/include/ode/precision.h"
    "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/include/ode/version.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/ode.pc")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/ode-config")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/ode-config.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/ode-config-version.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2/ode-export.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2/ode-export.cmake"
         "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/CMakeFiles/Export/lib/cmake/ode-0.16.2/ode-export.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2/ode-export-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2/ode-export.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/CMakeFiles/Export/lib/cmake/ode-0.16.2/ode-export.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/CMakeFiles/Export/lib/cmake/ode-0.16.2/ode-export-debug.cmake")
  endif()
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/CMakeFiles/Export/lib/cmake/ode-0.16.2/ode-export-minsizerel.cmake")
  endif()
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/CMakeFiles/Export/lib/cmake/ode-0.16.2/ode-export-relwithdebinfo.cmake")
  endif()
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ode-0.16.2" TYPE FILE FILES "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/CMakeFiles/Export/lib/cmake/ode-0.16.2/ode-export-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "C:/Users/denni/Documents/GitHub/Zephyr-ga2022-/src/ode/build/vs2019single/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
