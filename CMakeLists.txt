cmake_minimum_required(VERSION 3.24)
PROJECT(QtOSGWidget)
SET(TARGETNAME "QtOSGWidget")

SET(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

SET(CMAKE_CXX_STANDARD  20)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  

set(JPEG_DIR D:/tjb/opensrc/install/jpeg/lib/cmake/libjpeg-turbo)
find_package(JPEG)

set(OSG_DIR D:/tjb/opensrc/install/osg)
find_package(OpenSceneGraph)
#qt

set(QT_DIR D:/Qt/Qt5.12.9/5.12.9/msvc2017_64/lib/cmake/Qt5)
# 设置moc rcc uic
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL REQUIRED)

add_definitions(-DOSG_USE_FLOAT_MATRIX)

# 添加可执行文件，注意添加WIN32，否则有一个控制台黑框

set(COMMON_SOURCE_FILES
    main.cpp
    QtOSGWidget.cpp
    QtOSGWidget.h
)
if(WIN32)
    set(PLATFORM_SPECIFIC_SOURCE_FILES
    )
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SPECIFIC_SOURCE_FILES
    )
endif()

add_executable(QtOSGWidget
${COMMON_SOURCE_FILES}
${PLATFORM_SPECIFIC_SOURCE_FILES}
)


#include_directories(${TARGETNAME} 
#    ${OSG_DIR}/include
#)

include_directories(${TARGETNAME} ${OSG_INCLUDE_DIR})

if(JPEG_FOUND)
target_link_libraries(${TARGETNAME} 
    optimized ${JPEG_LIBRARY_RELEASE}
    debug ${JPEG_LIBRARY_DEBUG}
)
endif()

# 链接Qt库
target_link_libraries(${TARGETNAME} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL)
target_link_libraries(${TARGETNAME} 

    debug ${OSG_DIR}/lib/osgd.lib
    debug ${OSG_DIR}/lib/osgUtild.lib
    debug ${OSG_DIR}/lib/osgDBd.lib
    debug ${OSG_DIR}/lib/osgViewerd.lib
    debug ${OSG_DIR}/lib/osgGAd.lib
    debug ${OSG_DIR}/lib/OpenThreadsd.lib
    debug ${OSG_DIR}/lib/osgTextd.lib

#    optimized ${OSG_DIR}/lib/osg162-osg.lib
#   	optimized ${OSG_DIR}/lib/osg162-osgUtil.lib
#  	optimized ${OSG_DIR}/lib/osg162-osgDB.lib
# 	optimized ${OSG_DIR}/lib/osg162-osgViewer.lib
#
#    debug ${OSG_DIR}/lib/osg162-osgd.lib
#    debug ${OSG_DIR}/lib/osg162-osgUtild.lib
#    debug ${OSG_DIR}/lib/osg162-osgDBd.lib
#    debug ${OSG_DIR}/lib/osg162-osgViewerd.lib
#    debug ${OSG_DIR}/lib/osg162-osgGAd.lib
#    debug ${OSG_DIR}/lib/ot21-OpenThreadsd.lib
#    debug ${OSG_DIR}/lib/osg162-osgTextd.lib
#
#    debug ${OSG_DIR}/lib/osgPlugins-3.6.5/osgdb_osgd.lib
#    debug ${OSG_DIR}/lib/osgPlugins-3.6.5/osgdb_rgbd.lib
#    debug ${OSG_DIR}/lib/osgPlugins-3.6.5/osgdb_serializers_osgd.lib
#    debug ${OSG_DIR}/lib/osgPlugins-3.6.5/osgdb_deprecated_osgd.lib
#    
#    debug ${OSG_DIR}/lib/osgPlugins-3.6.5/osgdb_jpegd.lib

    opengl32.lib
)

GET_FILENAME_COMPONENT(CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Visual Studio中调试依赖的独立环境变量
set_property(TARGET ${PROJECT_NAME}	 PROPERTY VS_DEBUGGER_ENVIRONMENT "PATH=${QT_DIR}/../../../bin;${OSG_DIR}/bin;%PATH%")  