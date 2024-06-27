set(VulkanSDK_FOUND TRUE)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

    if(NOT DEFINED ENV{VULKAN_SDK})
        find_package(Vulkan)
        if(NOT Vulkan_FOUND)
            message(FATAL_ERROR "\nYour PC doesn't seem to have Vulkan installed!\n")
        endif()
    else()
        set(VulkanSDK_Include_Dir $ENV{VULKAN_SDK}/Include)
        set(VulkanSDK_Libraries_Dir $ENV{VULKAN_SDK}/Lib)
        set(VulkanSDK_Runtime_Dir $ENV{VULKAN_SDK}/Bin)
        find_program(GLSLC NAMES glslc glslc.exe
                    HINTS ${VulkanSDK_Runtime_Dir}
                    DOC "glslc program")
    endif()

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(Vulkan REQUIRED)
    find_program(GLSLC NAMES glslc glslc.exe
                HINTS /usr/bin
                DOC "glslc program")
endif()