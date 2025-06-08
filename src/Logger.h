//
// Created by work on 5/27/25.
//

#ifndef GAMEOFLIFEMPI_LOGGER_H
#define GAMEOFLIFEMPI_LOGGER_H

#pragma once

#include <iostream>

#ifdef DEBUG
#define DEBUG_LOG(x) do { std::cerr << x << std::endl; } while (0)
#else
#define DEBUG_LOG(x) do { } while (0)
#endif

#endif //GAMEOFLIFEMPI_LOGGER_H
