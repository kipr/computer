PROJECT(computer)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

FIND_PACKAGE(Qt4 REQUIRED)

SET(INCLUDE ${computer_SOURCE_DIR}/include)
SET(RC ${computer_SOURCE_DIR}/rc)
SET(SRC ${computer_SOURCE_DIR}/src)
SET(UI ${computer_SOURCE_DIR}/ui)

INCLUDE_DIRECTORIES(${computer_BINARY_DIR})
INCLUDE_DIRECTORIES(${INCLUDE})

INCLUDE(${QT_USE_FILE})

FILE(GLOB INCLUDES ${INCLUDE}/*.hpp)
FILE(GLOB SOURCES ${SRC}/*)
FILE(GLOB UIS ${UI}/*)

SET(computer_SRCS_CXX ${SOURCES})
SET(computer_UI ${UIS})
SET(computer_MOC_SRCS ${INCLUDES})

SET(computer_QRC_FILES ${RC}/icons.qrc ${RC}/target.qrc)

QT4_WRAP_CPP(computer_SRCS_CXX ${computer_MOC_SRCS})
QT4_WRAP_UI(computer_SRCS_CXX ${computer_UI})
QT4_ADD_RESOURCES(computer_SRCS_CXX ${computer_QRC_FILES})

IF(WIN32)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/../prefix/include)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/../prefix/lib)
ENDIF(WIN32)

SET(CMAKE_CXX_FLAGS "-Wall")

IF(APPLE)
	# SET(CMAKE_OSX_SYSROOT "${OSX_DEVELOPER_ROOT}/SDKs/MacOSX10.6.sdk")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch x86_64 -g")
	LINK_DIRECTORIES(/Library/Frameworks/)
	ADD_EXECUTABLE(computer MACOSX_BUNDLE ${computer_SRCS_CXX})
ELSEIF(UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
	ADD_EXECUTABLE(computer ${computer_SRCS_CXX})
ELSEIF(WIN32)
	SET(CMAKE_CXX_FLAGS "-Wl,--enable-auto-import")
	ADD_EXECUTABLE(computer WIN32 ${computer_SRCS_CXX})
ENDIF()

SET(EXECUTABLE_OUTPUT_PATH ${computer_SOURCE_DIR}/deploy)
SET(LIBRARY_OUTPUT_PATH ${computer_SOURCE_DIR}/lib)
TARGET_LINK_LIBRARIES(computer ${QT_LIBRARIES} pcompiler kovanserial kar kovan opencv_core opencv_imgproc)

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

IF(APPLE)
	copy_files(target ${RC}/target.c ${computer_SOURCE_DIR}/deploy/computer.app/Contents/prefix/usr/include)
	copy_files(platform.hints ${RC}/platform.hints ${computer_SOURCE_DIR}/deploy/computer.app/Contents)
ELSE(APPLE)
	copy_files(target ${RC}/target.c ${computer_SOURCE_DIR}/deploy/prefix/usr/include)
	copy_files(platform.hints ${RC}/platform.hints ${computer_SOURCE_DIR}/deploy)
ENDIF(APPLE)


IF(APPLE)
	ADD_CUSTOM_TARGET(computer_MacDeployQt ALL 
		COMMAND ${QT_BINARY_DIR}/macdeployqt ${computer_SOURCE_DIR}/deploy/computer.app -verbose=0
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		DEPENDS computer)
	
	ADD_CUSTOM_TARGET(Info.plist ALL 
		COMMAND ${CMAKE_COMMAND} -E copy ${RC}/Info.plist ${computer_SOURCE_DIR}/deploy/computer.app/Contents
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		DEPENDS computer)
	
	ADD_CUSTOM_TARGET(Icons ALL 
		COMMAND ${CMAKE_COMMAND} -E copy ${RC}/mac_icon.icns ${computer_SOURCE_DIR}/deploy/computer.app/Contents/Resources
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		DEPENDS computer Info.plist computer_MacDeployQt)
ENDIF()
