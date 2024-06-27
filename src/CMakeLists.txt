function(BuildTarget TARGET_NAME)
    file(MAKE_DIRECTORY ${ROOT_DIR}/shaders/${TARGET_NAME})
    file(MAKE_DIRECTORY ${ROOT_DIR}/bin/${TARGET_NAME})
    set(TARGET_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME})
    message(STATUS "Configure Target: ${TARGET_NAME}")
    message(STATUS "Build type is: ${CMAKE_BUILD_TYPE}")
    file(GLOB
        GLSL_SRC_LIST
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.vert
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.tesc
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.tese
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.geom
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.frag
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.comp
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.mesh
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.task
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.rgen
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.rchit
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.rmiss
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.rcall
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.rahit
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.rint
        ${ROOT_DIR}/shaders/${TARGET_NAME}/*.glsl)
    foreach(GLSL ${GLSL_SRC_LIST})
        get_filename_component(FILE_NAME ${GLSL} NAME)
        set(SPIRV ${ROOT_DIR}/bin/${TARGET_NAME}/${FILE_NAME}.spv)
        add_custom_command(OUTPUT ${SPIRV}
                            PRE_BUILD
                            COMMAND ${GLSLC} ${GLSL} -o ${SPIRV}
                            DEPENDS ${GLSL})
        list(APPEND SPIRV_BIN_LIST ${SPIRV})
    endforeach()

    set(SHADER_TARGET "${TARGET_NAME}_Shader")
    add_custom_target(${SHADER_TARGET}
                        ALL
                        DEPENDS ${SPIRV_BIN_LIST})

    file(GLOB_RECURSE
        SRC_LIST
        ${TARGET_FOLDER}/*.cpp
        ${TARGET_FOLDER}/*.hpp
        ${TARGET_FOLDER}/*.h)
    add_executable(${TARGET_NAME} ${SRC_LIST})
    add_dependencies(${TARGET_NAME} ${SHADER_TARGET})
    target_compile_definitions(${TARGET_NAME} PRIVATE SHADER_DIR="${ROOT_DIR}/bin/${TARGET_NAME}/")
    target_include_directories(${TARGET_NAME} PRIVATE ${ROOT_DIR}/base)
    target_link_libraries(${TARGET_NAME} PRIVATE base)

    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_DIR}/bin/${TARGET_NAME}
                                                ARCHIVE_OUTPUT_DIRECTORY ${ROOT_DIR}/lib
                                                LIBRARY_OUTPUT_DIRECTORY ${ROOT_DIR}/lib)
endfunction()

file(MAKE_DIRECTORY ${ROOT_DIR}/lib)
set(TARGETS
    VulkanExperiment
    VulkanRayTracing)
foreach(TARGET ${TARGETS})
    BuildTarget(${TARGET})
endforeach()
