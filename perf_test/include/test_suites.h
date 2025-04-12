/**
 * @file test_suites.h
 * @brief 测试套件函数定义
 */

#pragma once

#include "perf_config.h"

// 运行吞吐量测试套件
void run_throughput_test_suite(TestConfig base_config);

// 运行延迟测试套件
void run_latency_test_suite(TestConfig base_config);

// 运行压力测试套件
void run_stress_test_suite(TestConfig base_config);

// 运行比较测试套件（mm_logger vs spdlog）
void run_comparison_test_suite(TestConfig base_config);

// 运行单个测试
bool run_single_test(TestConfig& config);
