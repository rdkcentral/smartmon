/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
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
#include <vector>
#include <map>
#include <mutex>
#include <thread>

#include "EventUtils.h"
#include "TransportHandler.h"
#include "EventListener.h"
class ThunderInterface : public EventListener
{
public:
    ThunderInterface() : m_isInitialized(false), m_connListener(nullptr),mp_thThread(nullptr)
    {
        mp_handler = new TransportHandler();
    }
    int initialize();
    bool launchResidentApp(const std::string &url, int timeout = REQUEST_TIMEOUT_IN_MS);
    bool launchResidentApp(int timeout = REQUEST_TIMEOUT_IN_MS);
    void setThunderConnectionURL(const std::string &wsurl);
    void connectToThunder();

    std::vector<std::string> &getActiveApplications(int timeout = REQUEST_TIMEOUT_IN_MS);
    bool launchApplication(const std::string &callsign, int timeout = REQUEST_TIMEOUT_IN_MS);
    int suspendApplication(const std::string &callsign, int timeout = REQUEST_TIMEOUT_IN_MS);
    int offloadApplication(const std::string &callsign, int timeout = REQUEST_TIMEOUT_IN_MS);

    void shutdown();
    ~ThunderInterface();

    // no copying allowed
    ThunderInterface(const ThunderInterface &) = delete;
    ThunderInterface &operator=(const ThunderInterface &) = delete;

    bool setMemoryLimits(int lowMemLimit, int criticalMemLimit, int timeout = REQUEST_TIMEOUT_IN_MS);

    // Inherited from EventListener class
    void registerKeyListener(std::function<void(int, int, bool)>) override;
    void registerLaunchListener(std::function<void(const std::string &, const std::string &)>) override;
    void registerShutdownListener(std::function<void(const std::string &)>) override;
    void registerMemoryListener(std::function<void(bool, MemoryEvent, int)>) override;
    void registerConnectStatusListener(std::function<void(bool)> callback)
    {
        m_connListener = callback;
    };

    void removeKeyListener() override;
    void removeLaunchListener() override;
    void removeShutdownListener() override;
    void removeMemoryListener() override;

    // Do not call this directly. These are callback functions
    void onKeyPress(int keycode, int flag, bool keyPressed) override;
    void onLaunched(const std::string &client, const std::string &launchType) override;
    void onDestroyed(const std::string &client) override;
    void onLowMemory(MemoryEvent memType, int memValue) override;
    void onLowMemoryCleared(MemoryEvent memType, int memValue) override;

private:
    TransportHandler *mp_handler;
    bool m_isInitialized;
    std::vector<std::string> m_appList;

    std::function<void(bool)> m_connListener;

    std::thread *mp_thThread;
    const std::string m_homeURL;

    void connected(bool connected);
    void onMsgReceived(const std::string message);
    void registerEvent(const std::string &event, bool isBinding);

    bool sendMessage(const std::string jsonmsg, int msgId, int timeout = REQUEST_TIMEOUT_IN_MS);
    bool sendSubscriptionMessage(const std::string jsonmsg, int msgId, int timeout = REQUEST_TIMEOUT_IN_MS);
};