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

#ifndef WEBDRIVER_IE_LOCATIONINFO_H_
#define WEBDRIVER_IE_LOCATIONINFO_H_

namespace webdriver {

struct LocationInfo {
  long x = 0;
  long y = 0;
  long width = 0;
  long height = 0;

  LocationInfo(void) {
  }

  LocationInfo(long x, long y, long width, long height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
  }

  void CopyFrom(LocationInfo info) {
    this->x = info.x;
    this->y = info.y;
    this->width = info.width;
    this->height = info.height;
  }

  void CopyFromRect(RECT rect) {
    this->x = rect.left;
    this->y = rect.top;
    this->width = rect.right - rect.left;
    this->height = rect.bottom - rect.top;
  }

  void CopyFromHtmlRect(IHTMLRect* rect) {
    rect->get_left(&this->x);
    rect->get_top(&this->y);
    long right;
    rect->get_right(&right);
    long bottom;
    rect->get_bottom(&bottom);
    this->width = right - this->x;
    this->height = bottom - this->y;
  }

  RECT AsRect(void) const {
    RECT rect;
    rect.left = this->x;
    rect.top = this->y;
    rect.right = this->x + this->width;
    rect.bottom = this->y + this->height;
    return rect;
  }
};

struct FloatingPointLocationInfo {
  double x;
  double y;
  double width;
  double height;

  LocationInfo AsLocationInfo(void) {
    LocationInfo info;
    info.x = static_cast<long>(this->x);
    info.y = static_cast<long>(this->y);
    info.width = static_cast<long>(this->width);
    info.height = static_cast<long>(this->height);
    return info;
  }
};

} // namespace webdriver

#endif // WEBDRIVER_IE_LOCATIONINFO_H_
