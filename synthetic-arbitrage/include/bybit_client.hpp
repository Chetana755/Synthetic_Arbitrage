#pragma once
#include "TradeTick.hpp"
#include <functional>

void run_bybit_stream(std::function<void(const TradeTick&)> callback);