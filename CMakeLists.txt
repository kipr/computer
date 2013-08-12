project(computer)

cmake_minimum_required(VERSION 2.8.0)

find_package(Qt4 REQUIRED)

set(INCLUDE ${computer_SOURCE_DIR}/include)
set(RC ${computer_SOURCE_DIR}/rc)
set(SRC ${computer_SOURCE_DIR}/src)
set(UI ${computer_SOURCE_DIR}/ui)

include_directories(${computer_BINARY_DIR})
include_directories(${INCLUDE})

include(${QT_USE_FILE})

file(GLOB INCLUDES ${INCLUDE}/*.hpp)
file(GLOB SOURCES ${SRC}/*)
file(GLOB UIS ${UI}/*)

set(computer_SRCS_CXX ${SOURCES})
set(computer_UI ${UIS})
set(computer_MOC_SRCS ${INCLUDES})

set(computer_QRC_FILES ${RC}/icons.qrc ${RC}/target.qrc)

qt4_wrap_cpp(computer_SRCS_CXX ${computer_MOC_SRCS})
qt4_wrap_ui(computer_SRCS_CXX ${computer_UI})
qt4_add_resources(computer_SRCS_CXX ${computer_QRC_FILES})

if(WIN32)
	include_directories(${CMAKE_SOURCE_DIR}/../prefix/include)
	include_directories(${CMAKE_SOURCE_DIR}/../opencv/kiss-prefix/include)
	link_directories(${CMAKE_SOURCE_DIR}/../prefix/lib)
	link_directories(${CMAKE_SOURCE_DIR}/../opencv/kiss-prefix/lib)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
add_definitions(-O3)

if(APPLE)
	# set(CMAKE_OSX_SYSROOT "${OSX_DEVELOPER_ROOT}/SDKs/MacOSX10.6.sdk")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch x86_64 -g")
	link_directories(/Library/Frameworks/)
	add_executable(computer MACOSX_BUNDLE ${computer_SRCS_CXX} ${INCLUDES})
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
	add_executable(computer ${computer_SRCS_CXX} ${INCLUDES})
elseif(WIN32)
	add_executable(computer WIN32 ${computer_SRCS_CXX} ${INCLUDES})
endif()

set(EXECUTABLE_OUTPUT_PATH ${computer_SOURCE_DIR}/deploy)
set(LIBRARY_OUTPUT_PATH ${computer_SOURCE_DIR}/lib)
target_link_libraries(computer ${QT_LIBRARIES} pcompiler kovanserial kar kovan)

if(WIN32)
	target_link_libraries(computer opencv_core249 opencv_imgproc249)
else()
	target_link_libraries(computer opencv_core opencv_imgproc)
endif()

macro(copy_files NAME FILES DESTINATION)
  add_custom_target(copy_${NAME} ALL
    COMMENT "Copying files: ${FILES}")

  foreach(FILENAME ${FILES})
    get_filename_component(REAL_FILENAME ${FILENAME} NAME)
    set(DST "${DESTINATION}/${REAL_FILENAME}")

    add_custom_command(
      TARGET copy_${NAME}
      COMMAND ${CMAKE_COMMAND} -E copy "${FILENAME}" "${DST}"
      )
  endforeach(FILENAME)
endmacro(copy_files)

if(APPLE)
	copy_files(target ${RC}/target.c ${computer_SOURCE_DIR}/deploy/computer.app/Contents/prefix/usr/include)
	copy_files(platform.hints ${RC}/platform.hints ${computer_SOURCE_DIR}/deploy/computer.app/Contents)
else()
	copy_files(target ${RC}/target.c ${computer_SOURCE_DIR}/deploy/prefix/usr/include)
	copy_files(platform.hints ${RC}/platform.hints ${computer_SOURCE_DIR}/deploy)
endif()

if(APPLE)
	add_custom_target(computer_MacDeployQt ALL 
		COMMAND ${QT_BINARY_DIR}/macdeployqt ${computer_SOURCE_DIR}/deploy/computer.app -verbose=0
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		DEPENDS computer)
	
	add_custom_target(Info.plist ALL 
		COMMAND ${CMAKE_COMMAND} -E copy ${RC}/Info.plist ${computer_SOURCE_DIR}/deploy/computer.app/Contents
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		DEPENDS computer)
	
	add_custom_target(Icons ALL 
		COMMAND ${CMAKE_COMMAND} -E copy ${RC}/mac_icon.icns ${computer_SOURCE_DIR}/deploy/computer.app/Contents/Resources
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		DEPENDS computer Info.plist computer_MacDeployQt)
endif()
