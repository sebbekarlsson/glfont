cmake_minimum_required(VERSION 3.20)

add_compile_options(-g -Wall)

project(glfont)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/.)



add_custom_target(
  glfont_shaders ALL
  COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/.
)




file(GLOB PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB glfont_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)


add_executable(glfont_e ${glfont_sources})

# just for debugging purposes
target_link_libraries(glfont_e PRIVATE glfw)



list(FILTER glfont_ources EXCLUDE REGEX "main.c")

add_library(glfont SHARED ${glfont_sources})
add_library(glfont_static STATIC ${glfont_sources})

target_compile_options(glfont PUBLIC -fPIC)
target_compile_options(glfont_static PUBLIC -fPIC)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(
  glfont
    PUBLIC
      $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      ${CMAKE_CURRENT_BINARY_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/include)


target_include_directories(
  glfont_static
    PUBLIC
      $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      ${CMAKE_CURRENT_BINARY_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/include)


target_include_directories(
  glfont_e
    PUBLIC
      $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      ${CMAKE_CURRENT_BINARY_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/include)


include(FetchContent)

macro (glfont_library_exists LIBNAME)
  find_library(freetype freetype)
  if(EXISTS "/usr/lib/lib${LIBNAME}.so" OR EXISTS "/usr/lib/x86_64-linux-gnu/lib${LIBNAME}.so")
    set("${LIBNAME}_exists" true)
  else()
    set("${LIBNAME}_exists" false)
  endif()
endmacro()

glfont_library_exists(freetype)


FetchContent_Declare(
  cglm
  GIT_REPOSITORY https://github.com/recp/cglm.git
  GIT_TAG "v0.8.4"
  )
if (NOT TARGET cglm)
  FetchContent_MakeAvailable(cglm)
endif()

if(freetype_exists)
  include_directories(/usr/include/freetype2)
else()
  add_definitions(-DFREETYPE_SRC)
  message("Fetching freetype...")
  FetchContent_Declare(
          freetype2
          GIT_REPOSITORY https://github.com/aseprite/freetype
          GIT_TAG master
  )
  FetchContent_MakeAvailable(freetype2)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/_deps/freetype2-src/include)
endif()

target_link_libraries(glfont_static PUBLIC m GL GLEW freetype cglm)
target_link_libraries(glfont_e PUBLIC m GL GLEW freetype cglm)
target_link_libraries(glfont PUBLIC m GL GLEW freetype cglm)
