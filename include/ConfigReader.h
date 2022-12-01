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
#include <json/json.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib> //For getenv

#include "EventUtils.h"

const std::string CONFIG_FILE("smartmon.json");

struct MonitorConfig
{
    std::string homeurl;
    int homekey;
    int lowmem;
    int buzzmem;
    std::string wsurl;
    std::map<std::string,std::string> callsignMap;
    std::vector<std::string> criticalapps;
};

struct ConfigReader
{
#include <algorithm>
#include <cctype>
    static void  lowercase(std::string &str){
        std::transform(str.begin(),str.end(),str.begin(),[](unsigned char x){
            return std::tolower(x);
        });
    }
    static MonitorConfig *readConfiguration()
    {
        Json::Value root;
        std::ifstream ifs;

        const char *configPath = getenv("SMCONFIG");
        if (nullptr == configPath)
            configPath = CONFIG_FILE.c_str();

        std::cout << " Configuration read from "<<configPath<<std::endl;
        ifs.open(configPath);

        if(!ifs.is_open())
        {
            LOGERR("Failed to open %s. Returning default values..", configPath);
            return loadDefaultValues();
        }

        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        if (!parseFromStream(builder, ifs, &root, &errs))
        {
            std::cout << errs << std::endl;
            return loadDefaultValues();
        }

        MonitorConfig *monconf = new MonitorConfig();
        monconf->homeurl = root["homeurl"].asString();
        monconf->wsurl = root["wsurl"].asString();
        monconf->homekey = root["homekey"].asInt();
        monconf->lowmem = root["lowmem"].asInt();
        monconf->buzzmem = root["criticalmem"].asInt();
        //This is required becase of an error in RDKShell which returns all
        //callsigns in lowercase.
        if(!root["callsigns"].empty())
        {
             Json::ArrayIndex size = root["callsigns"].size();
            for (int x = 0; x < size; x++)
            {
                std::string  callsign = root["callsigns"][x].asString();
                std::string lcallsign = callsign;
                lowercase(lcallsign);

                monconf->callsignMap[lcallsign]= callsign;
            }
        }


        if(!root["criticalapps"].empty())
        {
             Json::ArrayIndex size = root["criticalapps"].size();
            for (int x = 0; x < size; x++)
            {
                monconf->criticalapps.push_back(root["criticalapps"][x].asString());
            }           
        }
        return monconf;
    }
    static  MonitorConfig * loadDefaultValues()
    {
        MonitorConfig *monconf = new MonitorConfig();
        monconf->homeurl = "https://apps.rdkcentral.com/rdk-apps/accelerator-home-ui/index.html#menu";
        monconf->wsurl = "ws://127.0.0.1:9998/jsonrpc";
        monconf->homekey = 36;
        monconf->lowmem = 120;
        monconf->buzzmem = 60;
        monconf->callsignMap["lightningapp"]="LightningApp";
        monconf->callsignMap["htmlapp"]="HtmlApp";
        monconf->callsignMap["amazon"]="Amazon";
        monconf->callsignMap["cobalt"]="Cobalt";
        monconf->callsignMap["netflix"]="Netflix";

        return monconf;        
    }
};
