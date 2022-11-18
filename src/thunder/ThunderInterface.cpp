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

#include "json/json.h"

#include "ThunderInterface.h"
#include "ProtocolHandler.h"
#include "ResponseHandler.h"
#include "EventUtils.h"

// static bool debug = false;
void ThunderInterface::connected(bool connected)
{
    if (nullptr != m_connListener)
        m_connListener(connected);
}
void ThunderInterface::onMsgReceived(const string message)
{
    ResponseHandler *evtHandler = ResponseHandler::getInstance();
    LOGINFO(" %s", message.c_str());
    int msgId = 0;
    if (getMessageId(message, msgId))
    {
        evtHandler->addMessageToResponseQueue(msgId, message);
    }
    else
    {
        evtHandler->addMessageToEventQueue(message);
    }
}

int ThunderInterface::initialize()
{
    LOGTRACE(" Enter.");
    int status = mp_handler->initialize();

    mp_handler->registerConnectionHandler([this](bool isConnected)
                                          { connected(isConnected); });
    mp_handler->registerMessageHandler([this](string message)
                                       { onMsgReceived(message); });

    ResponseHandler::getInstance()->registerEventListener(this);
    LOGTRACE(" Exit.");
    return status;
}

ThunderInterface::~ThunderInterface()
{
    LOGTRACE(" Enter.");

    if (mp_handler->isConnected())
    {
        mp_handler->disconnect();
        mp_thThread->join();
    }

    delete mp_handler;
    delete mp_thThread;
}
void ThunderInterface::setThunderConnectionURL(const std::string &wsurl)
{
    LOGTRACE(" Enter.");
    mp_handler->setConnectURL(wsurl);
}
void ThunderInterface::connectToThunder()
{
    LOGTRACE(" Enter.");
    if (mp_thThread != nullptr)
    {
        // Do we need to join and then delete ?
        delete mp_thThread;
    }
    auto &handler = mp_handler;
    mp_thThread = new std::thread([handler]
                                  { handler->connect(); });
}

bool ThunderInterface::sendMessage(const string jsonmsg, int msgId, int timeout)
{
    bool status = false;
    ResponseHandler *evtHandler = ResponseHandler::getInstance();
    LOGINFO(" Request : %s", jsonmsg.c_str());

    if (mp_handler->sendMessage(jsonmsg) == 1) // Success
    {
        string response = evtHandler->getRequestStatus(msgId, timeout);
        convertResultStringToBool(response, status);
    }
    return status;
}
bool ThunderInterface::sendSubscriptionMessage(const string jsonmsg, int msgId, int timeout)
{
    int status = false;
    ResponseHandler *evtHandler = ResponseHandler::getInstance();
    LOGINFO(" Request : %s", jsonmsg.c_str());

    if (mp_handler->sendMessage(jsonmsg) == 1) // Success
    {
        string response = evtHandler->getRequestStatus(msgId, timeout);
        convertEventSubResponseToInt(response, status);
    }
    return status == 0;
}
bool ThunderInterface::setMemoryLimits(int lowMemLimit, int criticalMemLimit, int timeout)
{
    int id = 0;
    string jsonmsg = getMemoryLimitRequest(lowMemLimit, criticalMemLimit, id);
    return sendMessage(jsonmsg, id, timeout);
}
bool ThunderInterface::launchApplication(const string &callsign, int timeout)
{
    int id = 0;
    string jsonmsg = getLaunchPremiumApp(callsign, id);
    return sendMessage(jsonmsg, id, timeout);
}
bool ThunderInterface::launchResidentApp(int timeout)
{
    return launchResidentApp(m_homeURL, timeout);
}

bool ThunderInterface::launchResidentApp(const string &url, int timeout)
{
    int id = 0;
    string jsonmsg = getLaunchResidentApp(url, id);
    return sendMessage(jsonmsg, id, timeout);
}

std::vector<string> &ThunderInterface::getActiveApplications(int timeout)
{
    int id = 0;
    ResponseHandler *evtHandler = ResponseHandler::getInstance();
    m_appList.clear();
    string jsonmsg = getClientList(id);
    LOGINFO(" Launch request API : %s", jsonmsg.c_str());

    if (mp_handler->sendMessage(jsonmsg) == 1) // Success
    {
        string response = evtHandler->getRequestStatus(id);
        convertResultStringToArray(response, "clients", m_appList);
    }
    return m_appList;
}

int ThunderInterface::suspendApplication(const string &callsign, int timeout)
{
    int msgId = 0;
    string jsonmsg = getSuspendPremiumApp(callsign, msgId);
    return sendMessage(jsonmsg, msgId, timeout);
}
int ThunderInterface::offloadApplication(const string &callsign, int timeout)
{
    int msgId = 0;
    string jsonmsg = getOffLoadPremiumApp(callsign, msgId);
    return sendMessage(jsonmsg, msgId, timeout);
    return 0;
}

void ThunderInterface::shutdown()
{
    mp_handler->disconnect();
    ResponseHandler::getInstance()->shutdown();
    mp_thThread->join();
}

void ThunderInterface::registerEvent(const std::string &event, bool isbinding)
{
    int msgId = 0;
    bool status = false;
    std::string callsign = "org.rdk.RDKShell.1.";

    std::string jsonmsg;
    if (isbinding)
        jsonmsg = getSubscribeRequest(callsign, event, msgId);
    else
        jsonmsg = getUnSubscribeRequest(callsign, event, msgId);
    status = sendMessage(jsonmsg, msgId);

    LOGINFO(" Event %s, response  %d ", event.c_str(), status);
}
/**
 *
 * Implementation of event handlers
 */
// Do not call this directly. These are callback functions
void ThunderInterface::onKeyPress(int keycode, int flag, bool keyPressed)
{
    LOGINFO(" Code %d , Pressed ? %d ", keycode, keyPressed);

    if (nullptr != m_keyListener)
        m_keyListener(keycode, flag, keyPressed);
}
void ThunderInterface::onLaunched(const std::string &client, const std::string &launchType)
{
    LOGINFO("%s launched as %s", client.c_str(), launchType.c_str());
    if (nullptr != m_launchListener)
        m_launchListener(client, launchType);
}
void ThunderInterface::onDestroyed(const std::string &client)
{
    LOGINFO(" App gets deleted %s", client.c_str());
    if (nullptr != m_shutdownListener)
        m_shutdownListener(client);
}
void ThunderInterface::onLowMemory(MemoryEvent memType, int memValue)
{

    LOGINFO(" Memory reduced to %d", memValue);
    if (nullptr != m_memListener)
        m_memListener(true, memType, memValue);
}
void ThunderInterface::onLowMemoryCleared(MemoryEvent memType, int memValue)
{

    LOGINFO("Memory pressure relieved to %d", memValue);
    if (nullptr != m_memListener)
        m_memListener(false, memType, memValue);
}

void ThunderInterface::registerKeyListener(std::function<void(int, int, bool)> callback)
{
    m_keyListener = callback;
    std::string event = "onKeyEvent";
    registerEvent(event, true);
}
void ThunderInterface::registerLaunchListener(std::function<void(const std::string &, const std::string &)> callback)
{
    m_launchListener = callback;
    std::string event = "onLaunched";
    registerEvent(event, true);
}
void ThunderInterface::registerShutdownListener(std::function<void(const std::string &)> callback)
{
    m_shutdownListener = callback;
    std::string event = "onDestroyed";
    registerEvent(event, true);
}
void ThunderInterface::registerMemoryListener(std::function<void(bool, MemoryEvent, int)> callback)
{
    m_memListener = callback;
    const char *events[] = {"onDeviceLowRamWarning", "onDeviceCriticallyLowRamWarning", "onDeviceCriticallyLowRamWarningCleared", "onDeviceLowRamWarningCleared"};
    for (auto &event : events)
    {
        registerEvent(event, true);
    }
}

void ThunderInterface::removeKeyListener()
{
    m_keyListener = nullptr;
    std::string event = "onDeviceLowRamWarning";
    registerEvent(event, false);
}
void ThunderInterface::removeLaunchListener()
{
    m_launchListener = nullptr;
    std::string event = "onLaunched";
    registerEvent(event, false);
}
void ThunderInterface::removeShutdownListener()
{
    std::string event = "onDestroyed";
    registerEvent(event, true);
}
void ThunderInterface::removeMemoryListener()
{
    m_memListener = nullptr;
    const char *events[] = {"onDeviceLowRamWarning", "onDeviceCriticallyLowRamWarning", "onDeviceCriticallyLowRamWarningCleared", "onDeviceLowRamWarningCleared"};
    for (auto &event : events)
    {
        registerEvent(event, false);
    }
}
