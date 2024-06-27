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

#include "VulkanLogger.h"

#ifdef LOG_TO_FILE
VulkanLogger VulkanLogger::s_Logger{true, LOG_FILE_PATH};
#else
VulkanLogger VulkanLogger::s_Logger{false};
#endif

VulkanLogger::VulkanLogger(bool toFile, const std::string &filePath)
    : m_LogToFile(toFile), p_Stream(nullptr)
{
    if (toFile)
    {
        if (fopen_s(&p_Stream, filePath.c_str(), "a") != 0)
        {
            fprintf_s(stderr, "Failed to open file: %s!\n", filePath.c_str());
        }
    }
    else
    {
        p_Stream = LOG_STREAM;
    }
}

void VulkanLogger::Info(const char *fmt, ...)
{
    std::va_list args1;
    va_start(args1, fmt);
    std::va_list args2;
    va_copy(args2, args1);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, fmt, args1));
    va_end(args1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args2);
    va_end(args2);
    std::fprintf(p_Stream, "[INFO] %s\n", buf.data());
}

void VulkanLogger::InfoWithTime(const char *fmt, ...)
{
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    gmtime_s(&tm, &t);
    char time_buf[100];
    std::strftime(time_buf, sizeof(time_buf), "%D %T", &tm);
    std::va_list args1;
    va_start(args1, fmt);
    std::va_list args2;
    va_copy(args2, args1);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, fmt, args1));
    va_end(args1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args2);
    va_end(args2);
    std::fprintf(p_Stream, "%s\n[INFO] %s\n", time_buf, buf.data());
}

void VulkanLogger::Warning(const char *fmt, ...)
{
    std::va_list args1;
    va_start(args1, fmt);
    std::va_list args2;
    va_copy(args2, args1);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, fmt, args1));
    va_end(args1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args2);
    va_end(args2);
    std::fprintf(p_Stream, "[WARNING] %s\n", buf.data());
}

void VulkanLogger::WarningWithTime(const char *fmt, ...)
{
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    gmtime_s(&tm, &t);
    char time_buf[100];
    std::strftime(time_buf, sizeof(time_buf), "%D %T", &tm);
    std::va_list args1;
    va_start(args1, fmt);
    std::va_list args2;
    va_copy(args2, args1);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, fmt, args1));
    va_end(args1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args2);
    va_end(args2);
    std::fprintf(p_Stream, "%s\n[WARNING] %s\n", time_buf, buf.data());
}

void VulkanLogger::Error(const char *fmt, ...)
{
    std::va_list args1;
    va_start(args1, fmt);
    std::va_list args2;
    va_copy(args2, args1);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, fmt, args1));
    va_end(args1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args2);
    va_end(args2);
    std::fprintf(p_Stream, "[ERROR] %s\n", buf.data());
}

void VulkanLogger::ErrorWithTime(const char *fmt, ...)
{
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    gmtime_s(&tm, &t);
    char time_buf[100];
    std::strftime(time_buf, sizeof(time_buf), "%D %T", &tm);
    std::va_list args1;
    va_start(args1, fmt);
    std::va_list args2;
    va_copy(args2, args1);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, fmt, args1));
    va_end(args1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args2);
    va_end(args2);
    std::fprintf(p_Stream, "%s\n[ERROR] %s\n", time_buf, buf.data());
}

/**
 * @warning Fatal can NEVER be asynchronous since it doesn't return.
 */
void VulkanLogger::Fatal(const char *fmt, ...)
{
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    gmtime_s(&tm, &t);
    char time_buf[100];
    std::strftime(time_buf, sizeof(time_buf), "%D %T", &tm);
    std::va_list args1;
    va_start(args1, fmt);
    std::va_list args2;
    va_copy(args2, args1);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, fmt, args1));
    va_end(args1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args2);
    va_end(args2);
    std::fprintf(p_Stream, "%s\n[FATAL] %s\n", time_buf, buf.data());
    throw std::runtime_error(buf.data());
}

void VulkanLogger::Abort(const char *fmt, ...)
{
    std::va_list args1;
    va_start(args1, fmt);
    std::va_list args2;
    va_copy(args2, args1);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, fmt, args1));
    va_end(args1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args2);
    va_end(args2);
    std::fprintf(p_Stream, "[ABORT] %s\nAbort the program!\n", buf.data());
    std::fflush(p_Stream);
    std::abort();
}

VulkanLogger::~VulkanLogger()
{
    if (m_LogToFile)
    {
        fclose(p_Stream);
    }
}
