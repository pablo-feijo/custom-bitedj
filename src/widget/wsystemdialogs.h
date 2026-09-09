// Copyright (C) 2026 Custom Bite DJ contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <functional>
#include "util/bootsettings.h"
class QWidget;
namespace mixxx::systemdialogs {
void clock(QWidget* parent);
// Native tests inject a temporary boot-file reader; production uses fixed OS paths.
void overclock(QWidget* parent,
        std::function<bootsettings::Settings()> reader = bootsettings::read);
void power(QWidget* parent);
}
