/*
 *
 ******************************************************************************
 *    Copyright [2024] [YongSong]
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 ******************************************************************************
 *
 */

#ifndef VULKAN_CONFIG_HEADER
#define VULKAN_CONFIG_HEADER

#pragma once

/**
 * @brief Change definitions to config Project.
 * @attention Define LOG_TO_FILE to enable log message to default log file, change the _log_message_ function in file VulkanTools.h to log to custom log file.
 */

/////////////////////////////// window title ///////////////////////////////

// Window title
#define WND_TITLE "VulkanExample"

/////////////////////////////// window title ///////////////////////////////

/////////////////////////////// index buffer type ///////////////////////////////

/**
 * @brief index type
 * @note more details see VulkanTools.h index type
 */
#define INDEX_TYPE_UINT32

/////////////////////////////// index buffer type ///////////////////////////////

/////////////////////////////// log system ///////////////////////////////

/** @brief log stream.
 * @note If LOG_TO_FILE was defined, then this macro will be IGNORED and log to default log file however
 * exceptions will ALWAYS be thrown into LOG_STREAM. See VulkanTools.h file function _log_message_.
 */
#define LOG_STREAM stderr
// Control if writing data to log file.
// #define LOG_TO_FILE
// Log file path, if LOG_TO_FILE was not defined then this will be ignored.
#define LOG_FILE_PATH HOME_DIR "res/logs/DefaultLog.log"

/////////////////////////////// log system ///////////////////////////////

/////////////////////////////// thread ///////////////////////////////

// The hard ware thread count rate
#define HARD_WARE_THREAD_RATE 3

/////////////////////////////// thread ///////////////////////////////

/////////////////////////////// version ///////////////////////////////

// Duplicated from VK_MAKE_API_VERSION.
#define MAKE_VERSION(variant, major, minor, patch) \
    (((static_cast<uint32_t>(variant)) << 29U) | ((static_cast<uint32_t>(major)) << 22U) | ((static_cast<uint32_t>(minor)) << 12U) | (static_cast<uint32_t>(patch)))
#define VERSION_VARIANT(version) (static_cast<uint32_t>(version) >> 29U)
#define VERSION_MAJOR(version) ((static_cast<uint32_t>(version) >> 22U) & 0x7FU)
#define VERSION_MINOR(version) ((static_cast<uint32_t>(version) >> 12U) & 0x3FFU)
#define VERSION_PATCH(version) (static_cast<uint32_t>(version) & 0xFFFU)

// Static application name
#define APP_NAME "MyVulkan"
// Static application version
#define APP_VERSION MAKE_VERSION(0, 1, 0, 0)
// Static engine name
#define ENGINE_NAME "None"
// Static engine version
#define ENGINE_VERSION MAKE_VERSION(0, 1, 0, 0)
// Static API version. Patch version should always be set to 0.
#define API_VERSION_1_0 MAKE_VERSION(0, 1, 0, 0)
// Static API version. Patch version should always be set to 0. Support for vkGetPhysicalDeviceProperties2, etc.
#define API_VERSION_1_1 MAKE_VERSION(0, 1, 1, 0)
// Change API_VERSION to support other Vulkan features.
#define API_VERSION API_VERSION_1_1

/////////////////////////////// version ///////////////////////////////

/////////////////////////////// validation ///////////////////////////////

/**
 * @brief define this, validation layer on if in debug mode, off if in release mode
 */
#define AUTO_VALIDATION

/////////////////////////////// validation ///////////////////////////////

/////////////////////////////// allocation callback ///////////////////////////////

#define ALLOCATE_CALLBACK nullptr

/////////////////////////////// allocation callback ///////////////////////////////

/////////////////////////////// other ///////////////////////////////

#define DEFAULT_FENCE_TIMEOUT 0xFFFFFFFFU

/////////////////////////////// other ///////////////////////////////

#endif
