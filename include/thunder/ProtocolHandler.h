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

std::string getLaunchResidentApp(const std::string &url, int &id);
std::string getOffloadResidentApp(const std::string &allsign, const std::string &type, int &id);
std::string getLaunchPremiumApp(const std::string &callsign, int &id);
std::string getSuspendPremiumApp(const std::string &callsign, int &id);
std::string getOffLoadPremiumApp(const std::string &callsign, int &id);
std::string getSubscribeRequest(const std::string &callsignWithVer, const std::string &event, int &id);
std::string getUnSubscribeRequest(const std::string &callsignWithVer, const std::string &event, int &id);
std::string getMemoryLimitRequest(int lowMem, int criticalMem, int &id);
std::string getClientList(int &id);
bool convertResultStringToArray(const std::string &root, const std::string key, std::vector<std::string> &arr);
bool convertResultStringToBool(const std::string &root, bool &);
bool convertEventSubResponseToInt(const std::string &root, int &);

