#pragma once

#include <Arduino.h>

// ── Nivoi logovanja ───────────────────────────────────────────────────────────
#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3

// ── Glavni prekidač za nivo logovanja ─────────────────────────────────────────
#ifndef PROJECT_LOG_LEVEL
#define PROJECT_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// ── Makroi za logovanje ───────────────────────────────────────────────────────

#if PROJECT_LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(format, ...) Serial.printf("[ERROR] %s:%d @ %s(): " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define LOG_ERROR(format, ...)
#endif

#if PROJECT_LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(format, ...) Serial.printf("[INFO] " format "\n", ##__VA_ARGS__)
#else
#define LOG_INFO(format, ...)
#endif

#if PROJECT_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(format, ...) Serial.printf("[DEBUG] %s:%d @ %s(): " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif
