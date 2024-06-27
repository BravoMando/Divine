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

#include "VulkanThreadPool.h"

VulkanThreadPool::VulkanThreadPool(uint32_t maxThreadCount)
{
    if (maxThreadCount > HARD_WARE_THREAD_RATE * static_cast<uint32_t>(std::thread::hardware_concurrency()))
    {
        maxThreadCount = HARD_WARE_THREAD_RATE * static_cast<uint32_t>(std::thread::hardware_concurrency());
    }
    for (uint32_t i = 0; i < maxThreadCount; ++i)
    {
        // Every thread is in while loop to check condition that if there is at least one job.
        this->m_Workers.emplace_back(
            [this](void) -> void
            {
                while (true)
                {
                    std::unique_lock<std::mutex> lock(this->m_Mutex);
                    // If false, then wait/block.
                    this->m_Condition.wait(lock,
                                           [this](void) -> bool
                                           {
                                               return !this->m_Jobs.empty() || this->m_Stop;
                                           });

                    if (this->m_Stop && this->m_Jobs.empty())
                    {
                        return;
                    }

                    std::function<void(void)> job(std::move(this->m_Jobs.front()));
                    this->m_Jobs.pop();
                    job();
                }
            });
    }
}

VulkanThreadPool::~VulkanThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(this->m_Mutex);
        this->m_Stop = true;
        m_Condition.notify_all();
    }

    for (std::thread &t : this->m_Workers)
    {
        t.join();
    }
}
