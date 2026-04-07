#pragma once

#include <stdio.h>

// ── C-compatible logging using printf (for use in .c files) ───────────────────

#ifndef PROJECT_LOG_LEVEL
#define PROJECT_LOG_LEVEL 2  // LOG_LEVEL_INFO
#endif

#if PROJECT_LOG_LEVEL >= 1
#define LOG_C_ERROR(format, ...) printf("[ERROR] %s:%d @ %s(): " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define LOG_C_ERROR(format, ...)
#endif

#if PROJECT_LOG_LEVEL >= 2
#define LOG_C_INFO(format, ...) printf("[INFO] " format "\n", ##__VA_ARGS__)
#else
#define LOG_C_INFO(format, ...)
#endif

#if PROJECT_LOG_LEVEL >= 3
#define LOG_C_DEBUG(format, ...) printf("[DEBUG] %s:%d @ %s(): " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define LOG_C_DEBUG(format, ...)
#endif
