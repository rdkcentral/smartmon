/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <string>
#include <functional>

enum MemoryEvent
{
    CRITICAL_MEMORY_EVENT,
    LOW_MEMORY_EVENT
};

class EventListener
{
protected:
    std::function<void(int, int, bool)> m_keyListener;
    std::function<void(const std::string &, const std::string &)> m_launchListener;
    std::function<void(const std::string &)> m_shutdownListener;
    std::function<void(bool, MemoryEvent, int)> m_memListener;

public:
    virtual void registerKeyListener(std::function<void(int, int, bool)>) = 0;
    virtual void registerLaunchListener(std::function<void(const std::string &, const std::string &)>) = 0;
    virtual void registerShutdownListener(std::function<void(const std::string &)>) = 0;
    virtual void registerMemoryListener(std::function<void( bool, MemoryEvent, int)>) = 0;

    virtual void removeKeyListener()=0;
    virtual void removeLaunchListener()=0;
    virtual void removeShutdownListener()=0;
    virtual void removeMemoryListener() =0;

    // Do not call this directly. These are callback functions
    virtual void onKeyPress(int keycode, int flag, bool keyPressed) = 0;
    virtual void onLaunched(const std::string &client, const std::string &launchType) = 0;
    virtual void onDestroyed(const std::string &client) = 0;
    virtual void onLowMemory(MemoryEvent memType, int memValue) = 0;
    virtual void onLowMemoryCleared(MemoryEvent memType, int memValue) = 0;
};