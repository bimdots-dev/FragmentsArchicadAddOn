cmake_minimum_required (VERSION 3.19)

include (Tools/CMakeCommon.cmake)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)
set (AC_API_DEVKIT_DIR "$ENV{AC_API_DEVKIT_DIR}" CACHE PATH "API DevKit directory.")

if (NOT DEFINED AC_VERSION)
    message (FATAL_ERROR "AC_VERSION is not set!")
endif ()

message (STATUS "Archicad Version: ${AC_VERSION}")
SetGlobalCompilerDefinitions (${AC_VERSION})

ReadConfigJson ()
set (AC_ADDON_LANGUAGE "${addOnDefaultLanguage}" CACHE STRING "Add-On language code.")

project (${addOnName}-${AC_VERSION})

set (AC_ADDON_FOR_DISTRIBUTION OFF CACHE BOOL "")

set (AddOnSourcesFolder Source)
set (AddOnResourcesFolder .)
GenerateAddOnProject (CMakeTarget ${AC_VERSION} "${AC_API_DEVKIT_DIR}" ${AddOnSourcesFolder} ${AddOnResourcesFolder} ${AC_ADDON_LANGUAGE})

set (MinizFolder Libs/miniz-3.0.2)
file (GLOB MinizFiles CONFIGURE_DEPENDS
    ${MinizFolder}/*.h
    ${MinizFolder}/*.c
)
set_source_files_properties(${MinizFiles} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
if (WIN32)
    set_source_files_properties (${MinizFiles} PROPERTIES COMPILE_FLAGS /W0)
endif ()
set_source_files_properties (${MinizFiles} PROPERTIES LANGUAGE CXX)
source_group ("Miniz" FILES ${MinizFiles})
target_include_directories (CMakeTarget PRIVATE ${MinizFolder})
target_sources (CMakeTarget PRIVATE ${MinizFiles})

set (SchemaFolder Source/Schema)
file (GLOB SchemaFiles CONFIGURE_DEPENDS
    ${SchemaFolder}/*.h
    ${SchemaFolder}/flatbuffers/*.h
)
source_group ("Schema" FILES ${SchemaFiles})
target_include_directories (CMakeTarget PRIVATE ${SchemaFolder})
target_sources (CMakeTarget PRIVATE ${SchemaFiles})
