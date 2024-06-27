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

#ifndef VULKAN_LOGGER_HEADER
#define VULKAN_LOGGER_HEADER

#pragma once

#include "VulkanCore.h"
#include "VulkanConfig.h"
#include "VulkanMedium.hpp"
#include "VulkanGenerics.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <stdexcept>

/**
 * @warning Fatal level message can NEVER be invoked in an asynchronous thread!
 */
class DVAPI_ATTR VulkanLogger final
{
public:
    static inline VulkanLogger &GetInstance() { return VulkanLogger::s_Logger; }

    void Info(const char *fmt, ...);
    void InfoWithTime(const char *fmt, ...);
    void Warning(const char *fmt, ...);
    void WarningWithTime(const char *fmt, ...);
    void Error(const char *fmt, ...);
    void ErrorWithTime(const char *fmt, ...);
    /**
     * @warning Fatal can NEVER be asynchronous since it doesn't return.
     */
    [[noreturn]] void Fatal(const char *fmt, ...);
    [[noreturn]] void Abort(const char *fmt, ...);

private:
    VulkanLogger(bool toFile, const std::string &filePath = "");
    ~VulkanLogger();
    VulkanLogger(const VulkanLogger &) = delete;
    VulkanLogger &operator=(const VulkanLogger &) = delete;
    VulkanLogger(VulkanLogger &&) = delete;
    VulkanLogger &operator=(VulkanLogger &&) = delete;

private:
    static VulkanLogger s_Logger;
    bool m_LogToFile;
    FILE *p_Stream;
};

#define INFO(fmt, ...) VulkanLogger::GetInstance().Info(fmt, ##__VA_ARGS__)
#define INFO_TIME(fmt, ...) VulkanLogger::GetInstance().InfoWithTime(fmt, ##__VA_ARGS__)
#define WARNING(fmt, ...) VulkanLogger::GetInstance().Warning(fmt, ##__VA_ARGS__)
#define WARNING_TIME(fmt, ...) VulkanLogger::GetInstance().WarningWithTime(fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) VulkanLogger::GetInstance().Error(fmt, ##__VA_ARGS__)
#define ERROR_TIME(fmt, ...) VulkanLogger::GetInstance().ErrorWithTime(fmt, ##__VA_ARGS__)
#define FATAL(fmt, ...) VulkanLogger::GetInstance().Fatal(fmt, ##__VA_ARGS__)
#define ABORT(fmt, ...) VulkanLogger::GetInstance().Abort(fmt, ##__VA_ARGS__)

#endif
