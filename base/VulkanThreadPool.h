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

#ifndef VULKAN_THREAD_POOL_HEADER
#define VULKAN_THREAD_POOL_HEADER

#pragma once

#include "VulkanCore.h"
#include "VulkanConfig.h"
#include "VulkanGenerics.hpp"

#include <cstdint>

#include <chrono>
#include <memory>
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

class DVAPI_ATTR VulkanThreadPool
{
public:
    template <typename F, typename... ArgType>
    auto Enqueue(F &&f, ArgType &&...args) -> std::future<decltype(f(args...))>
    {
        std::shared_ptr<std::packaged_task<decltype(f(args...))(void)>> pJob = std::make_shared<std::packaged_task<decltype(f(args...))(void)>>(std::bind(std::forward<F>(f), std::forward<ArgType>(args)...));
        std::future<decltype(f(args...))> future = pJob->get_future();
        {
            std::unique_lock<std::mutex> lock(this->m_Mutex);
            this->m_Jobs.emplace(std::move(
                [pJob](void) -> void
                {
                    (*pJob)();
                }));
            // Notify one thread without competation
            this->m_Condition.notify_one();
            // Notify all threads to compete
            // this->m_Condition.notify_all();
        }

        return future;
    }

    VulkanThreadPool(uint32_t maxThreadCount = 4U);
    ~VulkanThreadPool();
    VulkanThreadPool(const VulkanThreadPool &) = delete;
    VulkanThreadPool &operator=(const VulkanThreadPool &) = delete;
    VulkanThreadPool(VulkanThreadPool &&) = delete;
    VulkanThreadPool &operator=(VulkanThreadPool &&) = delete;

private:
    std::mutex m_Mutex{};
    std::condition_variable m_Condition{};
    // Shared stopping flags, needs to be locked
    bool m_Stop = false;
    // Threads
    std::vector<std::thread> m_Workers{};
    // For each task can be shared with treads, needs to be locked
    std::queue<std::function<void(void)>> m_Jobs{};
};

#endif
