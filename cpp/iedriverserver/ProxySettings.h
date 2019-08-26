// Licensed to the Software Freedom Conservancy (SFC) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership. The SFC licenses this file
// to you under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef WEBDRIVER_IE_PROXYSETTINGS_H_
#define WEBDRIVER_IE_PROXYSETTINGS_H_

#define WD_PROXY_TYPE_DIRECT "direct"
#define WD_PROXY_TYPE_SYSTEM "system"
#define WD_PROXY_TYPE_MANUAL "manual"
#define WD_PROXY_TYPE_AUTOCONFIGURE "pac"
#define WD_PROXY_TYPE_AUTODETECT "autodetect"

#include <string>

#include <json.h>

namespace webdriver {

struct ProxySettings {
  bool is_set;
  bool use_per_process_proxy;
  std::string proxy_type;
  std::string http_proxy;
  std::string ftp_proxy;
  std::string ssl_proxy;
  std::string socks_proxy;
  std::string socks_user_name;
  std::string socks_password;
  std::string proxy_bypass;
  std::string proxy_autoconfig_url;

  ProxySettings() : is_set(false),
                    use_per_process_proxy(false),
                    proxy_type(""),
                    http_proxy(""),
                    ftp_proxy(""),
                    ssl_proxy(""),
                    socks_proxy(""),
                    socks_user_name(""),
                    socks_password(""),
                    proxy_bypass(""),
                    proxy_autoconfig_url("") {}

  Json::Value AsJson() {
    Json::Value proxy_value;
    proxy_value["proxyType"] = proxy_type;
    if (proxy_type == WD_PROXY_TYPE_MANUAL) {
      if (http_proxy.size() > 0) {
        proxy_value["httpProxy"] = http_proxy;
      }
      if (ftp_proxy.size() > 0) {
        proxy_value["ftpProxy"] = this->ftp_proxy;
      }
      if (ssl_proxy.size() > 0) {
        proxy_value["sslProxy"] = ssl_proxy;
      }
      if (socks_proxy.size() > 0) {
        proxy_value["socksProxy"] = socks_proxy;
        if (socks_user_name.size() > 0) {
          proxy_value["socksUsername"] = socks_user_name;
        }
        if (socks_password.size() > 0) {
          proxy_value["socksPassword"] = socks_password;
        }
      }
    } else if (proxy_type == WD_PROXY_TYPE_AUTOCONFIGURE) {
      proxy_value["proxyAutoconfigUrl"] = proxy_autoconfig_url;
    }
    return proxy_value;
  }

  void CopyTo(ProxySettings* destination) {
    destination->use_per_process_proxy = use_per_process_proxy;
    destination->proxy_type = proxy_type;
    destination->http_proxy = http_proxy;
    destination->ftp_proxy = ftp_proxy;
    destination->ssl_proxy = ssl_proxy;
    destination->socks_proxy = socks_proxy;
    destination->socks_user_name = socks_user_name;
    destination->socks_password = socks_password;
    destination->proxy_bypass = proxy_bypass;
    destination->proxy_autoconfig_url = proxy_autoconfig_url;
  }
};

} // namespace webdriver

#endif // WEBDRIVER_IE_PROXYSETTINGS_H_
