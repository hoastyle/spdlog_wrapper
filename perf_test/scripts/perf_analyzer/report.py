#!/usr/bin/env python3
"""
Report generation module - Responsible for creating performance test summary reports
"""

import os
from datetime import datetime
import pandas as pd
import numpy as np
from .utils import get_current_timestamp, calculate_improvement, format_number

def create_summary_report(data, output_dir):
    """Create summary report"""
    # Prepare report file
    report_file = os.path.join(output_dir, "performance_summary_report.md")

    # Get current timestamp
    now = get_current_timestamp()

    # Calculate performance differences between mm_logger and spdlog
    mm_logger_throughput = 0
    spdlog_throughput = 0
    throughput_diff_pct = 0

    # Try different approaches to find comparison data
    comparison_throughput = data[data['TestType'].str.contains('comparison_throughput', na=False)]
    if len(comparison_throughput) < 2:
        # Try using any throughput tests
        throughput_tests = data[data['TestType'].str.contains('throughput', na=False)]
        if len(throughput_tests['Logger'].unique()) >= 2:
            comparison_throughput = throughput_tests

    # If we have enough data, calculate throughput comparison
    if (len(comparison_throughput) >= 2 and
        'mm_logger' in comparison_throughput['Logger'].values and
        'spdlog' in comparison_throughput['Logger'].values):

        # Calculate average throughput comparison
        mm_logger_throughput = comparison_throughput[comparison_throughput['Logger'] == 'mm_logger']['LogsPerSecond'].mean()
        spdlog_throughput = comparison_throughput[comparison_throughput['Logger'] == 'spdlog']['LogsPerSecond'].mean()

        if spdlog_throughput > 0:
            throughput_diff_pct = calculate_improvement(mm_logger_throughput, spdlog_throughput)

    # Calculate latency comparison
    mm_logger_latency = 0
    spdlog_latency = 0
    latency_diff_pct = 0

    # Try different approaches to find comparison data
    comparison_latency = data[data['TestType'].str.contains('comparison_latency', na=False)]
    if len(comparison_latency) < 2:
        # Try using any latency tests
        latency_tests = data[data['TestType'].str.contains('latency', na=False)]
        if len(latency_tests['Logger'].unique()) >= 2:
            comparison_latency = latency_tests

    # Check if we have the needed column
    has_median_latency = 'Median_Latency_us' in comparison_latency.columns

    # If we have enough data, calculate latency comparison
    if (len(comparison_latency) >= 2 and
        'mm_logger' in comparison_latency['Logger'].values and
        'spdlog' in comparison_latency['Logger'].values and
        has_median_latency):

        # Calculate median latency comparison
        mm_logger_latency = comparison_latency[comparison_latency['Logger'] == 'mm_logger']['Median_Latency_us'].mean()
        spdlog_latency = comparison_latency[comparison_latency['Logger'] == 'spdlog']['Median_Latency_us'].mean()

        if spdlog_latency > 0:
            latency_diff_pct = calculate_improvement(mm_logger_latency, spdlog_latency)

    # Get console output impact data
    console_impact_pct = None
    if 'EnableConsole' in data.columns:
        console_impact_data = data[data['Logger'] == 'mm_logger']
        if len(console_impact_data) > 0:
            console_on_data = console_impact_data[console_impact_data['EnableConsole'] == True]
            console_off_data = console_impact_data[console_impact_data['EnableConsole'] == False]

            if len(console_on_data) > 0 and len(console_off_data) > 0:
                console_on_perf = console_on_data['LogsPerSecond'].mean()
                console_off_perf = console_off_data['LogsPerSecond'].mean()

                if console_on_perf > 0:
                    console_impact_pct = calculate_improvement(console_off_perf, console_on_perf)

    # Get best worker threads count
    best_worker_threads = None
    worker_column = None

    # Try different worker thread columns
    for col in ['ExtractedWorkerThreads', 'WorkerThreads']:
        if col in data.columns and data[col].notna().any():
            worker_column = col
            break

    if worker_column:
        worker_data = data[data[worker_column].notna() & (data['Logger'] == 'mm_logger')]

        if len(worker_data) >= 2:
            # Get max throughput row
            best_worker_idx = worker_data['LogsPerSecond'].idxmax()
            if best_worker_idx is not None and best_worker_idx in worker_data.index:
                best_worker_row = worker_data.loc[best_worker_idx]
                best_worker_threads = best_worker_row[worker_column]

    # Get best queue size
    best_queue_size = None
    queue_column = None

    # Try different queue size columns
    for col in ['ExtractedQueueSize', 'QueueSize']:
        if col in data.columns and data[col].notna().any():
            queue_column = col
            break

    if queue_column:
        queue_data = data[data[queue_column].notna() & (data['Logger'] == 'mm_logger')]

        if len(queue_data) >= 2:
            # Get max throughput row
            best_queue_idx = queue_data['LogsPerSecond'].idxmax()
            if best_queue_idx is not None and best_queue_idx in queue_data.index:
                best_queue_row = queue_data.loc[best_queue_idx]
                best_queue_size = best_queue_row[queue_column]

    # Create report content
    with open(report_file, 'w', encoding='utf-8') as f:
        f.write(f"# MM-Logger Performance Test Report\n\n")
        f.write(f"**Generated:** {now}\n\n")

        f.write("## 1. Overall Performance Comparison\n\n")

        # Write throughput comparison
        f.write("### Throughput Comparison (Logs Per Second)\n\n")
        if mm_logger_throughput > 0 and spdlog_throughput > 0:
            f.write("| Logger | Average Throughput | Relative Performance |\n")
            f.write("|--------|------------|----------|\n")
            f.write(f"| MM-Logger | {format_number(mm_logger_throughput)} | {'+' if throughput_diff_pct >= 0 else ''}{throughput_diff_pct:.2f}% |\n")
            f.write(f"| spdlog | {format_number(spdlog_throughput)} | Baseline |\n\n")
        else:
            f.write("Not enough data for throughput comparison.\n\n")

        # Write latency comparison
        f.write("### Latency Comparison (microseconds)\n\n")
        if mm_logger_latency > 0 and spdlog_latency > 0:
            f.write("| Logger | Average Median Latency | Relative Performance |\n")
            f.write("|--------|--------------|----------|\n")
            f.write(f"| MM-Logger | {mm_logger_latency:.2f} | {'+' if latency_diff_pct >= 0 else ''}{latency_diff_pct:.2f}% |\n")
            f.write(f"| spdlog | {spdlog_latency:.2f} | Baseline |\n\n")

            # Note on latency: lower is better
            f.write("> Note: For latency, lower values are better. Negative values indicate MM-Logger has lower latency than spdlog.\n\n")
        else:
            f.write("Not enough data for latency comparison.\n\n")

        # Write thread scalability analysis
        f.write("## 2. Scalability Analysis\n\n")

        # Thread scalability analysis
        thread_column = None
        for col in ['ThreadCount', 'Threads']:
            if col in data.columns and data[col].notna().any():
                thread_column = col
                break

        if thread_column:
            thread_data = data[data[thread_column].notna()]

            if len(thread_data) >= 3:
                f.write("### Thread Scalability\n\n")
                f.write("| Threads | MM-Logger Throughput | spdlog Throughput | Relative Performance |\n")
                f.write("|--------|----------------|--------------|----------|\n")

                # Extract thread counts and sort
                thread_counts = sorted(thread_data[thread_column].unique())

                for thread_count in thread_counts:
                    thread_subset = thread_data[thread_data[thread_column] == thread_count]

                    mm_logger_value = thread_subset[thread_subset['Logger'] == 'mm_logger']['LogsPerSecond'].values
                    spdlog_value = thread_subset[thread_subset['Logger'] == 'spdlog']['LogsPerSecond'].values

                    if len(mm_logger_value) > 0 and len(spdlog_value) > 0:
                        mm_logger_val = mm_logger_value[0]
                        spdlog_val = spdlog_value[0]

                        if spdlog_val > 0:
                            rel_perf = calculate_improvement(mm_logger_val, spdlog_val)
                            rel_perf_str = f"{'+' if rel_perf >= 0 else ''}{rel_perf:.2f}%"
                        else:
                            rel_perf_str = "N/A"

                        f.write(f"| {thread_count} | {format_number(mm_logger_val)} | {format_number(spdlog_val)} | {rel_perf_str} |\n")

                f.write("\n")
            else:
                f.write("Not enough thread scalability test data.\n\n")
        else:
            f.write("Thread count data not available.\n\n")

        # Message size impact analysis
        size_column = None
        for col in ['MessageSizeType', 'MessageSize']:
            if col in data.columns and data[col].notna().any():
                size_column = col
                break

        if size_column:
            size_data = data[data[size_column].notna()]

            if len(size_data) >= 3:
                f.write("### Impact of Message Size on Performance\n\n")
                f.write("| Message Size | MM-Logger Throughput | spdlog Throughput | Relative Performance |\n")
                f.write("|----------|----------------|--------------|----------|\n")

                size_labels = {'small': 'Small', 'medium': 'Medium', 'large': 'Large'}
                sizes = ['small', 'medium', 'large']

                for size in sizes:
                    # Check if this size exists in the data
                    if size not in size_data[size_column].values:
                        continue

                    size_subset = size_data[size_data[size_column] == size]

                    mm_logger_value = size_subset[size_subset['Logger'] == 'mm_logger']['LogsPerSecond'].values
                    spdlog_value = size_subset[size_subset['Logger'] == 'spdlog']['LogsPerSecond'].values

                    if len(mm_logger_value) > 0 and len(spdlog_value) > 0:
                        mm_logger_val = mm_logger_value.mean()
                        spdlog_val = spdlog_value.mean()

                        if spdlog_val > 0:
                            rel_perf = calculate_improvement(mm_logger_val, spdlog_val)
                            rel_perf_str = f"{'+' if rel_perf >= 0 else ''}{rel_perf:.2f}%"
                        else:
                            rel_perf_str = "N/A"

                        f.write(f"| {size_labels.get(size, size)} | {format_number(mm_logger_val)} | {format_number(spdlog_val)} | {rel_perf_str} |\n")

                f.write("\n")
            else:
                f.write("Not enough message size test data.\n\n")
        else:
            f.write("Message size data not available.\n\n")

        # Write recommended configuration
        f.write("## 3. Recommended Configuration\n\n")

        # Build recommendations list
        recommendations = []

        # Worker threads recommendation
        if best_worker_threads is not None:
            recommendations.append(f"* **Recommended Worker Threads:** {best_worker_threads}")
        else:
            # Default recommendation
            recommendations.append("* **Recommended Worker Threads:** 2-4 (depending on CPU cores)")

        # Queue size recommendation
        if best_queue_size is not None:
            recommendations.append(f"* **Recommended Queue Size:** {best_queue_size}")
        else:
            # Default recommendation
            recommendations.append("* **Recommended Queue Size:** 8192-16384 (depending on memory resources)")

        # Console output recommendation
        if console_impact_pct is not None:
            if console_impact_pct >= 10:
                recommendations.append(f"* **Console Output Recommendation:** Disable (performance boost: ~{console_impact_pct:.2f}%)")
            else:
                recommendations.append(f"* **Console Output Recommendation:** Optional (minimal impact: {console_impact_pct:.2f}%)")
        else:
            recommendations.append("* **Console Output Recommendation:** Disable in production, enable in development")

        # Write recommendations
        for recommendation in recommendations:
            f.write(f"{recommendation}\n")

        f.write("\n")

        # Write performance optimization suggestions
        f.write("## 4. Performance Optimization Suggestions\n\n")

        f.write("1. **Async Logging Configuration:**\n")
        if best_worker_threads is not None and best_queue_size is not None:
            f.write(f"   - Set worker threads based on CPU cores and log volume, testing shows {best_worker_threads} worker threads optimal\n")
            f.write(f"   - Set queue size based on memory resources and peak log volume, testing shows {best_queue_size} entries optimal\n")
        else:
            f.write(f"   - Set worker threads based on CPU cores and log volume, typically 2-4 worker threads work well\n")
            f.write(f"   - Set queue size based on memory resources and peak log volume, typically 8192-16384 entries work well\n")
        f.write("\n")

        f.write("2. **Multi-threaded Environment Optimization:**\n")
        f.write("   - Increase worker threads and queue size when application thread count exceeds 8\n")
        f.write("   - For bursty logging scenarios, increase queue size for better stability\n")
        f.write("   - Use async mode to avoid blocking worker threads\n")
        f.write("\n")

        f.write("3. **I/O Performance Optimization:**\n")
        f.write("   - Store log files on SSD for better write performance\n")
        f.write("   - Set appropriate file size and total size limits to avoid frequent file rotation\n")
        f.write("   - For high-frequency logging, use larger single file size limits\n")
        f.write("\n")

        f.write("4. **Memory Usage Optimization:**\n")
        f.write("   - Monitor async queue usage to avoid excessive memory consumption\n")
        f.write("   - In resource-constrained environments, reduce queue size and increase worker threads\n")
        f.write("   - Use appropriate message sizes, as larger messages increase memory usage\n")
        f.write("\n")

        f.write("5. **Usage Recommendations:**\n")
        f.write("   - Disable console output in production for better performance\n")
        f.write("   - Enable DEBUG level and console output for debug builds\n")
        f.write("   - Disable DEBUG level logs in release builds to reduce log volume\n")
        f.write("\n")

        # Write conclusion
        f.write("## 5. Conclusion\n\n")

        # Generate conclusion based on comparison results
        if throughput_diff_pct > 5:
            f.write(f"MM-Logger demonstrates excellent throughput performance, averaging **{throughput_diff_pct:.2f}%** faster than native spdlog.")
        elif throughput_diff_pct >= -5:
            f.write(f"MM-Logger shows comparable throughput performance to native spdlog ({throughput_diff_pct:.2f}%).")
        else:
            f.write(f"MM-Logger throughput is slightly lower than native spdlog ({throughput_diff_pct:.2f}%), but offers additional features.")

        if latency_diff_pct < 0:
            f.write(f" In terms of latency, MM-Logger is **{-latency_diff_pct:.2f}%** lower than native spdlog, providing better responsiveness.\n\n")
        elif latency_diff_pct <= 10:
            f.write(f" Latency performance is comparable to native spdlog ({latency_diff_pct:.2f}%).\n\n")
        else:
            f.write(f" MM-Logger has **{latency_diff_pct:.2f}%** higher latency than native spdlog, which is still acceptable for most applications.\n\n")

        f.write("Overall, MM-Logger provides good performance with additional features like multi-level log rotation and total size limits, making it suitable for production environments. By following the recommendations in this report, you can optimize its performance for your specific use case.\n\n")

        # Add appendix with test environment info
        f.write("## Appendix: Test Environment\n\n")
        f.write("This test was conducted with the following configuration:\n\n")

        f.write("- **Test Tool:** MM-Logger Performance Test\n")
        f.write("- **Test Date:** " + now.split(" ")[0] + "\n")
        f.write("- **Test Script:** run_performance_tests.sh\n")
        f.write("- **Test Types:** Throughput, Latency, Stress, Comparison\n")
        f.write("- **Total Tests:** " + str(len(data)) + "\n\n")

        f.write("**Note:** Actual performance may vary depending on system configuration, compiler version, and operating system. It's recommended to run tests in your target environment for the most accurate results.\n")

    print(f"Generated performance summary report: {report_file}")
