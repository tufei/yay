cmake_minimum_required(VERSION 3.17)
project(yay)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

###############################################################################
## file globbing ##############################################################
###############################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables 
# `sources` and `data`.
#file(GLOB_RECURSE sources      src/main/*.cpp src/main/*.h)
#file(GLOB_RECURSE sources_test src/test/*.cpp)
#file(GLOB_RECURSE data resources/*)
# You can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically.
set(sources yay.c getopt.h slre.h slre.c)

if(MSVC)
  add_compile_options(/W3)
  add_compile_options(/wd4081)
  add_compile_options(/wd4115)
  add_compile_options(/wd4255)
  add_compile_options(/wd4267)
  add_compile_options(/wd4554)
  add_compile_options(/wd4668)
  add_compile_options(/wd4710)
  add_compile_options(/wd4820)
  add_compile_options(/wd4996)
  add_compile_options(/wd5045)
endif()

###############################################################################
## target definitions #########################################################
###############################################################################

# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
#add_executable(example ${sources} ${data})
add_executable(yay ${sources})

# Just for example add some compiler flags.
#target_compile_options(example PUBLIC -std=c++1y -Wall -Wfloat-conversion)
target_compile_options(yay PUBLIC -Wall)

# This allows to include files relative to the root of the src directory with a <> pair
#target_include_directories(example PUBLIC src/main)

# This copies all resource files in the build directory.
# We need this, because we want to work with paths relative to the executable.
#file(COPY ${data} DESTINATION resources)

###############################################################################
## dependencies ###############################################################
###############################################################################

# Add the include paths and link dependencies for project.
function(sdl_show TARGET)
    foreach(PKG SDL2 SDL2_image)
    find_package(${PKG} QUIET)
    if(${PKG}_FOUND)
      target_include_directories(${TARGET} PRIVATE ${${PKG}_INCLUDE_DIR})
      target_link_libraries(${TARGET} PRIVATE ${${PKG}_LIBRARY})
      message(STATUS "${PKG} found for ${TARGET}: include path ${${PKG}_INCLUDE_DIR}, link path ${${PKG}_LIBRARY}")
      message(STATUS "${PKG} include path ${${PKG}_INCLUDE_DIR}")
      message(STATUS "${PKG} link path ${${PKG}_LIBRARY}")
    else()
      message(SEND_ERROR "${PKG} not found for ${TARGET}")
    endif()
  endforeach()
endfunction()

if(MSVC)
  set(SDL_INCLUDE_DIR "C:\\SDL2\\SDL2-2.0.12\\include" CACHE PATH "SDL include path" FORCE)
  set(SDL_LIBRARY "C:\\SDL2\\SDL2-2.0.12\\lib\\x64\\SDL2.lib;C:\\SDL2\\SDL2-2.0.12\\lib\\x64\\SDL2main.lib" CACHE FILEPATH "SDL library path" FORCE)
  set(SDL_IMAGE_INCLUDE_DIR "C:\\SDL2\\SDL2_image-2.0.5\\include" CACHE PATH "SDL_image include path" FORCE)
  set(SDL_IMAGE_LIBRARY "C:\\SDL2\\SDL2_image-2.0.5\\lib\\x64\\SDL2_image.lib" CACHE FILEPATH "SDL_image library path" FORCE)
endif()

sdl_show(yay)

# This defines the variables Boost_LIBRARIES that containts all library names
# that we need to link into the program.
#find_package(Boost 1.36.0 COMPONENTS filesystem system REQUIRED)

#target_link_libraries(example PUBLIC
#  ${Boost_LIBRARIES}
#  # here you can add any library dependencies
#)

###############################################################################
## testing ####################################################################
###############################################################################

# This is for our testing framework,
# we don't add REQUIRED because it's just for testing.
# People who might want to build the project to use it should not be required
# to install testing dependencies.
#find_package(GTest)
#
#if(GTEST_FOUND)
#  add_executable(unit_tests ${sources_test} ${sources})
#
#  # This define is added to prevent collision with the main.
#  # It might be better solved by not adding the source with the main to the
#  # testing target.
#  target_compile_definitions(unit_tests PUBLIC UNIT_TESTS)
#
#  # This allows us to use the executable as a link library, and inherit all 
#  # linker options and library dependencies from it, by simply adding it as dependency.
#  set_target_properties(example PROPERTIES ENABLE_EXPORTS on)
#
#  target_link_libraries(unit_tests PUBLIC
#    ${GTEST_BOTH_LIBRARIES}
#    example
#  )
#
#  target_include_directories(unit_tests PUBLIC
#    ${GTEST_INCLUDE_DIRS} # doesn't do anything on linux
#  )
#  
#endif()

###############################################################################
## packaging ##################################################################
###############################################################################

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
#install(TARGETS example DESTINATION example_destination)
install(TARGETS yay DESTINATION bin)

# This is basically a repeat of the file copy instruction that copies the
# resources in the build directory, but here we tell cmake that we want it
# in the package.
#install(DIRECTORY resources DESTINATION example_destination)

# Now comes everything we need, to create a package
# there are a lot more variables you can set, and some
# you need to set for some package types, but we want to
# be minimal here.
set(CPACK_PACKAGE_NAME "yay")
set(CPACK_PACKAGE_VERSION "1.0.0")

# We don't want to split our program up into several incomplete pieces.
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
