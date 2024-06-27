cmake_minimum_required(VERSION 3.20.0)

set(LIBNAME base)

message(STATUS "Configure Target: ${LIBNAME}")
message(STATUS "Build type is: ${CMAKE_BUILD_TYPE}")

project(${LIBNAME})

if(NOT ROOT_DIR)
    set(ROOT_DIR ${PROJECT_SOURCE_DIR})
endif(NOT ROOT_DIR)

# Configure VulkanUI
set(TARGET_NAME VulkanUI)
file(MAKE_DIRECTORY ${ROOT_DIR}/bin/${TARGET_NAME})
message(STATUS "Configure Target: ${TARGET_NAME}")
message(STATUS "Build type is: ${CMAKE_BUILD_TYPE}")
file(GLOB
    GLSL_SRC_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.tesc
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.tese
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.mesh
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.task
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.rgen
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.rchit
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.rmiss
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.rcall
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.rahit
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.rint
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}/*.glsl)
foreach(GLSL ${GLSL_SRC_LIST})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV ${ROOT_DIR}/bin/${TARGET_NAME}/${FILE_NAME}.spv)
    add_custom_command(OUTPUT ${SPIRV}
                        PRE_BUILD
                        COMMAND ${GLSLC} ${GLSL} -o ${SPIRV}
                        DEPENDS ${GLSL})
    list(APPEND SPIRV_BIN_LIST ${SPIRV})
endforeach()

set(SHADER_TARGET "Build${TARGET_NAME}")
add_custom_target(${SHADER_TARGET}
                    ALL
                    DEPENDS ${SPIRV_BIN_LIST})

# Configure Dear ImGui API for shared library
# if(${LIB_TYPE} MATCHES "SHARED")
#     message(STATUS "Check Dear ImGui")
#     set(TARGET_FILE "${ROOT_DIR}/deps/imgui/imconfig.h")
#     set(INPUT_STR_VAR "#include \"../../base/VulkanCore.h\"\n#define IMGUI_API DVAPI_ATTR\n#define IMGUI_IMPL_API DVAPI_ATTR\n")
#     string(LENGTH "${INPUT_STR_VAR}" INPUT_STR_LENGTH)
#     file(SIZE "${TARGET_FILE}" FILE_SIZE)
#     math(EXPR START "${FILE_SIZE} - ${INPUT_STR_LENGTH} * 2")
#     file(READ "${TARGET_FILE}" FILE_CONTENT OFFSET ${START})
#     string(FIND
#         "${FILE_CONTENT}"
#         "${INPUT_STR_VAR}"
#         WRITE)
#     if (${WRITE} EQUAL -1)
#         message(STATUS "Configure Dear ImGui")
#         file(APPEND
#         "${TARGET_FILE}"
#         "${INPUT_STR_VAR}")
#     endif()
# endif()

file(GLOB
    SRC_LIST
    ./*.cpp
    ./*.hpp
    ./*.h
    ${ROOT_DIR}/deps/imgui/*.h
    ${ROOT_DIR}/deps/imgui/*.cpp)

if(${LIB_TYPE} MATCHES "SHARED")
    add_library(${LIBNAME} SHARED ${SRC_LIST})
    target_compile_definitions(${LIBNAME} PUBLIC EXPORT_LIB)
else()
    add_library(${LIBNAME} STATIC ${SRC_LIST})
    target_compile_definitions(${LIBNAME} PUBLIC NO_EXPORT_IMPORT)
endif()

add_dependencies(${LIBNAME} ${SHADER_TARGET})

target_compile_definitions(${LIBNAME} PUBLIC HOME_DIR="${ROOT_DIR}/")

target_include_directories(${LIBNAME} PUBLIC ${ROOT_DIR}/deps/opm/src)
target_include_directories(${LIBNAME} PUBLIC ${ROOT_DIR}/deps/glfw/include)
target_include_directories(${LIBNAME} PUBLIC ${ROOT_DIR}/deps/tiny_model_loader)
target_include_directories(${LIBNAME} PUBLIC ${ROOT_DIR}/deps/stb)
target_include_directories(${LIBNAME} PUBLIC ${VulkanSDK_Include_Dir})
target_link_directories(${LIBNAME} PUBLIC ${VulkanSDK_Libraries_Dir})
target_include_directories(${LIBNAME} PUBLIC ${ROOT_DIR}/deps/imgui)
target_link_libraries(${LIBNAME} PUBLIC opm)
target_link_libraries(${LIBNAME} PUBLIC glfw)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(${LIBNAME} PUBLIC vulkan-1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(${LIBNAME} PUBLIC vulkan)
endif()

set_target_properties(${LIBNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_DIR}/bin/${LIBNAME}
                                            ARCHIVE_OUTPUT_DIRECTORY ${ROOT_DIR}/lib
                                            LIBRARY_OUTPUT_DIRECTORY ${ROOT_DIR}/lib)
