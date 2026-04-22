#pragma once

#include <iostream>
#include <cstddef>

#include "../inc/ui/visualizer.hpp"
#include "../inc/processing/signal_processor.hpp"
#include "../inc/math/fft.hpp"

std::mutex magMutex;
std::vector<double> sharedMagnitudes;