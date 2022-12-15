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

#include "SmartMonitor.h"
#include "EventUtils.h"
#include <csignal>
#include <thread>

using namespace std;
using std::string;
bool debug = isDebugEnabled();
const string SmartMonitor::LAUNCH_URL = "http://127.0.0.1:50050/lxresui/index.html#menu";
SmartMonitor *SmartMonitor::_instance = nullptr;

const int THUNDER_TIMEOUT = 2000; // milliseconds

void SmartMonitor::handleTermSignal(int _signal)
{
    LOGINFO("Exiting from app..");

    unique_lock<std::mutex> ulock(m_lock);
    m_isActive = false;
    unRegisterForEvents();
    m_act_cv.notify_one();
}

void SmartMonitor::waitForTermSignal()
{
    LOGTRACE("Waiting for term signal.. ");
    thread termThread([&, this]()
                      {
    while (m_isActive)
    {
        unique_lock<std::mutex> ulock(m_lock);
        m_act_cv.wait(ulock);
    }
    
    LOGTRACE("[SmartMonitor::waitForTermSignal] Received term signal."); });
    termThread.join();
}
SmartMonitor::SmartMonitor() : m_isActive(false), config(nullptr), isConnected(false)
{
    LOGTRACE("Constructor.. ");
    tiface = new ThunderInterface();
}
SmartMonitor::~SmartMonitor()
{
    LOGTRACE("Destructor.. ");

    tiface->shutdown();
    delete tiface;
    tiface = nullptr;
    delete config;
    config = nullptr;
}
SmartMonitor *SmartMonitor::getInstance()
{
    LOGTRACE("Getting instance.. ");
    if (nullptr == _instance)
    {
        _instance = new SmartMonitor();
    }
    return _instance;
}
int SmartMonitor::initialize()
{
    LOGTRACE("Initializing new instance.. ");

    int status = false;
    // Remote reference of RDKShell instance
    config = ConfigReader::readConfiguration();
    {
        lock_guard<mutex> lkgd(m_lock);
        m_isActive = true;
    }
    signal(SIGTERM, [](int x)
           { SmartMonitor::getInstance()->handleTermSignal(x); });
    tiface->registerConnectStatusListener([&, this](bool connectionStatus)
                                          { isConnected = connectionStatus; });
    if (!config->wsurl.empty())
        tiface->setThunderConnectionURL(config->wsurl);
    tiface->initialize();

    status = true;
    return status;
}
void SmartMonitor::connectToThunder()
{
    LOGTRACE("Connecting to thunder.. ");
    tiface->connectToThunder();
}
int SmartMonitor::launchResidentApp()
{
    LOGTRACE("Launching residentapp.. ");
    int status = 0;
    if (tiface->launchResidentApp((config->homeurl.empty() ? nullptr : config->homeurl), 20000))
        status = 1;
    return status;
}

void SmartMonitor::registerForEvents()
{
    LOGTRACE("Enter.. ");
    if (config->lowmem != 0 && config->buzzmem != 0)
    {
        tiface->setMemoryLimits(config->lowmem, config->buzzmem);
    }
    tiface->registerKeyListener([&, this](int key, int flags, bool isKeyDown)
                                { onKeyPress(key, flags, isKeyDown); });
    tiface->registerLaunchListener([&, this](const std::string &client, const std::string &launchType)
                                   { onLaunched(client, launchType); });
    tiface->registerShutdownListener([&, this](const std::string &client)
                                     { onDestroyed(client); });
    tiface->registerMemoryListener([&, this](bool isPressured, MemoryEvent memType, int memValue)
                                   { memoryStatusUpdated(isPressured, memType, memValue); });
}

void SmartMonitor::onLaunched(const std::string client, const std::string &launchType)
{
    LOGTRACE("%s launched as %s", client.c_str(), launchType.c_str());
    if (launchType != "suspend") // It is a launch, not suspend
    {
        m_activeApp = client;
    }
    else
    {
        LOGTRACE("%Skipping  %s", client.c_str());
    }
}
void SmartMonitor::onKeyPress(int key, int flags, bool isKeyDown)
{
    LOGINFO(" %d , is key down ? %d ", key, isKeyDown);
    // We should attempt only for key pressed and homekey configured
    if (isKeyDown && config->homekey != 0 && config->homekey == key)
    {
        // Launch residentapp if needed.
        if (!isResidentAppRunning())
        {
            if (!m_activeApp.empty())
                tiface->suspendApplication(m_activeApp);
            tiface->launchResidentApp((config->homeurl.empty() ? nullptr : config->homeurl), 20000);
        }
    }
}

void SmartMonitor::onDestroyed(const std::string &client)
{
    LOGINFO(" %s offloaded", client.c_str());
}

void SmartMonitor::handleLowMemory(MemoryEvent memType, int memValue)
{
    LOGINFO("Currently active app is %s", m_activeApp.c_str());

    if (memType == LOW_MEMORY_EVENT)
    {
        // Let us try to ease memory pressure by offloading residentapp
        if (!stringCompareIgnoreCase(m_activeApp, "ResidentApp") && isResidentAppRunning())
        {
            // Is it worth to call isResidentAppRunning ?
            tiface->offloadApplication("ResidentApp");
            return;
        }
    }
    vector<std::string> apps = tiface->getActiveApplications();
    LOGINFO("Active app count %d ", apps.size());


    for (auto &app : apps)
    {
        bool found = false;
        // Try unloading all apps that are not critical.
        for (auto &callsign : config->criticalapps)
        {
            if (stringCompareIgnoreCase(app, callsign))
            {
                found = true;
                break;
            }
        }

        // Not the active application and not a critical app
        if (!found && !stringCompareIgnoreCase(app,m_activeApp))
        {
            LOGINFO(" Offloading %s", app.c_str());
            std::string csign = config->callsignMap[app];
            if(!csign.empty())
                tiface->offloadApplication(csign);
            else
                LOGINFO(" Failed to find call sign %s", app.c_str());
        }
    }
}
void SmartMonitor::handleMemoryGained(MemoryEvent memType, int memValue)
{
    // We need to load apps back.
}
void SmartMonitor::memoryStatusUpdated(bool isPressured, MemoryEvent memType, int memValue)
{

    LOGINFO(" Warning memory threshold reached %d", memValue);
    if (isPressured)
        handleLowMemory(memType, memValue);
    else
    {
        handleMemoryGained(memType, memValue);
    }
}
void SmartMonitor::unRegisterForEvents()
{
    tiface->removeKeyListener();
    tiface->removeLaunchListener();
    tiface->removeShutdownListener();
    tiface->removeMemoryListener();
}

bool SmartMonitor::getConnectStatus()
{
    return isConnected;
}

bool SmartMonitor::isResidentAppRunning()
{
    vector<std::string> apps = tiface->getActiveApplications();
    for (const string &app : apps)
    {
        if (stringCompareIgnoreCase(app, "ResidentApp"))
            return true;
    }
    return false;
}
