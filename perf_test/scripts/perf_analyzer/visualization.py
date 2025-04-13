#!/usr/bin/env python3
"""
Visualization Module - Responsible for generating performance analysis charts
"""

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os
from .utils import add_value_labels, calculate_improvement, get_clean_test_name

# Define colors at the top of the file
colors = ['#4285F4', '#EA4335', '#FBBC05', '#34A853']  # Google style colors

def plot_throughput_comparison(data, output_dir):
    """Draw throughput comparison between mm_logger and spdlog"""
    # Filter comparison test data
    comparison_data = data[data['TestType'].str.contains('comparison_throughput|throughput', na=False)]

    # Ensure there are two different logging libraries
    if len(comparison_data['Logger'].unique()) < 2:
        print("Warning: Not enough data for throughput comparison (need data for both mm_logger and spdlog)")
        print("Available loggers:", comparison_data['Logger'].unique())
        return

    # Group by test conditions
    groupby_columns = [col for col in ['Threads', 'MessageSize', 'QueueSize', 'WorkerThreads']
                       if col in comparison_data.columns]

    if not groupby_columns:
        print("Warning: No grouping columns available for throughput comparison")
        return

    grouped = comparison_data.groupby(groupby_columns)

    comparison_count = 0
    for name, group in grouped:
        # Skip groups without enough data
        if len(group) < 2:
            continue

        # Ensure each group has data for both mm_logger and spdlog
        if not (group['Logger'] == 'mm_logger').any() or not (group['Logger'] == 'spdlog').any():
            continue

        # Get grouping values
        name_values = name if isinstance(name, tuple) else (name,)
        name_dict = {col: val for col, val in zip(groupby_columns, name_values)}

        threads = name_dict.get('Threads', 'N/A')
        msg_size = name_dict.get('MessageSize', 'N/A')
        queue_size = name_dict.get('QueueSize', 'N/A')
        worker_threads = name_dict.get('WorkerThreads', 'N/A')

        # Prepare data
        mm_logger_data = group[group['Logger'] == 'mm_logger']['LogsPerSecond'].values
        spdlog_data = group[group['Logger'] == 'spdlog']['LogsPerSecond'].values

        if len(mm_logger_data) == 0 or len(spdlog_data) == 0:
            continue

        mm_logger_data = mm_logger_data[0]
        spdlog_data = spdlog_data[0]

        # Create chart
        fig, ax = plt.subplots(figsize=(10, 6))
        bars = ax.bar(['MM-Logger', 'spdlog'], [mm_logger_data, spdlog_data],
                      color=['#4285F4', '#EA4335'], width=0.6)

        # Add value labels
        add_value_labels(ax, bars)

        # Set title and labels
        title = f'Throughput Comparison (Threads: {threads}, Message Size: {msg_size})'
        if queue_size != 'N/A' and worker_threads != 'N/A':
            title += f'\nQueue Size: {queue_size}, Worker Threads: {worker_threads}'
        ax.set_title(title, fontsize=15)
        ax.set_ylabel('Logs Per Second', fontsize=13)
        ax.set_ylim(0, max(mm_logger_data, spdlog_data) * 1.2)  # Set Y-axis limit, leave space for values

        # Calculate performance difference percentage
        if spdlog_data > 0:
            diff_pct = calculate_improvement(mm_logger_data, spdlog_data)
            if diff_pct > 0:
                diff_text = f"MM-Logger is {diff_pct:.1f}% faster than spdlog"
                text_color = 'green'
            else:
                diff_text = f"MM-Logger is {-diff_pct:.1f}% slower than spdlog"
                text_color = 'red'
            ax.text(0.5, 0.9, diff_text, transform=ax.transAxes,
                   ha='center', fontsize=14, color=text_color)

        # Save chart
        filename = f"throughput_comparison_t{threads}_m{msg_size}_q{queue_size}_w{worker_threads}.png"
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, filename))
        plt.close()

        comparison_count += 1

    if comparison_count > 0:
        print(f"Generated {comparison_count} throughput comparison charts")
    else:
        # Create a simplified comparison using aggregate data if detailed comparison is not possible
        try:
            mm_logger_throughput = comparison_data[comparison_data['Logger'] == 'mm_logger']['LogsPerSecond'].mean()
            spdlog_throughput = comparison_data[comparison_data['Logger'] == 'spdlog']['LogsPerSecond'].mean()

            if pd.notna(mm_logger_throughput) and pd.notna(spdlog_throughput):
                fig, ax = plt.subplots(figsize=(10, 6))
                bars = ax.bar(['MM-Logger', 'spdlog'], [mm_logger_throughput, spdlog_throughput],
                              color=['#4285F4', '#EA4335'], width=0.6)

                add_value_labels(ax, bars)

                ax.set_title('Overall Throughput Comparison', fontsize=15)
                ax.set_ylabel('Logs Per Second (Average)', fontsize=13)

                if spdlog_throughput > 0:
                    diff_pct = calculate_improvement(mm_logger_throughput, spdlog_throughput)
                    if diff_pct > 0:
                        diff_text = f"MM-Logger is {diff_pct:.1f}% faster than spdlog"
                        text_color = 'green'
                    else:
                        diff_text = f"MM-Logger is {-diff_pct:.1f}% slower than spdlog"
                        text_color = 'red'
                    ax.text(0.5, 0.9, diff_text, transform=ax.transAxes,
                           ha='center', fontsize=14, color=text_color)

                filename = "throughput_comparison_overall.png"
                plt.tight_layout()
                plt.savefig(os.path.join(output_dir, filename))
                plt.close()

                print("Generated simplified throughput comparison chart")
            else:
                print("Warning: Could not generate throughput comparison charts, insufficient data")
        except Exception as e:
            print(f"Error generating simplified throughput comparison: {e}")
            print("Warning: Could not generate throughput comparison charts, insufficient data")

def plot_latency_comparison(data, output_dir):
    """Draw latency comparison between mm_logger and spdlog"""
    # Filter comparison test data
    comparison_data = data[data['TestType'].str.contains('comparison_latency|latency', na=False)]

    # Ensure there are two different logging libraries
    if len(comparison_data['Logger'].unique()) < 2:
        print("Warning: Not enough data for latency comparison (need data for both mm_logger and spdlog)")
        return

    # Check if we have the necessary latency columns
    latency_columns = ['Min_Latency_us', 'Median_Latency_us', 'P95_Latency_us', 'P99_Latency_us']
    missing_columns = [col for col in latency_columns if col not in comparison_data.columns]

    if missing_columns:
        print(f"Warning: Missing latency data columns: {', '.join(missing_columns)}")
        print("Attempting to continue with available data...")
        latency_columns = [col for col in latency_columns if col in comparison_data.columns]
        if not latency_columns:
            print("Error: No latency data columns available")
            return

    # Group by test conditions
    groupby_columns = [col for col in ['Threads', 'MessageSize', 'QueueSize', 'WorkerThreads']
                       if col in comparison_data.columns]

    if not groupby_columns:
        print("Warning: No grouping columns available for latency comparison")
        return

    grouped = comparison_data.groupby(groupby_columns)

    comparison_count = 0
    for name, group in grouped:
        if len(group) < 2:
            continue  # Skip incomplete comparison groups

        # Ensure each group has data for both mm_logger and spdlog
        if not (group['Logger'] == 'mm_logger').any() or not (group['Logger'] == 'spdlog').any():
            continue

        # Get grouping values
        name_values = name if isinstance(name, tuple) else (name,)
        name_dict = {col: val for col, val in zip(groupby_columns, name_values)}

        threads = name_dict.get('Threads', 'N/A')
        msg_size = name_dict.get('MessageSize', 'N/A')
        queue_size = name_dict.get('QueueSize', 'N/A')
        worker_threads = name_dict.get('WorkerThreads', 'N/A')

        # Prepare data
        mm_logger_data = group[group['Logger'] == 'mm_logger']
        spdlog_data = group[group['Logger'] == 'spdlog']

        # Extract latency data points
        mm_logger_metrics = []
        spdlog_metrics = []
        metric_names = []

        for col in latency_columns:
            if col in mm_logger_data.columns and col in spdlog_data.columns:
                mm_logger_value = mm_logger_data[col].values[0]
                spdlog_value = spdlog_data[col].values[0]

                # Skip if values are missing
                if pd.isna(mm_logger_value) or pd.isna(spdlog_value):
                    continue

                mm_logger_metrics.append(mm_logger_value)
                spdlog_metrics.append(spdlog_value)

                # Create readable metric name
                if col == 'Min_Latency_us':
                    metric_names.append('Min Latency')
                elif col == 'Median_Latency_us':
                    metric_names.append('Median Latency')
                elif col == 'P95_Latency_us':
                    metric_names.append('95th Percentile')
                elif col == 'P99_Latency_us':
                    metric_names.append('99th Percentile')
                elif col == 'Max_Latency_us':
                    metric_names.append('Max Latency')
                else:
                    metric_names.append(col.replace('_us', '').replace('_', ' '))

        # Skip if no valid metrics found
        if not metric_names:
            continue

        # Create chart
        fig, ax = plt.subplots(figsize=(12, 7))

        x = np.arange(len(metric_names))
        width = 0.35

        # Draw bar chart
        bars1 = ax.bar(x - width/2, mm_logger_metrics, width, label='MM-Logger', color='#4285F4')
        bars2 = ax.bar(x + width/2, spdlog_metrics, width, label='spdlog', color='#EA4335')

        # Add value labels
        def add_labels(bars):
            for bar in bars:
                height = bar.get_height()
                ax.text(bar.get_x() + bar.get_width()/2., height + 1,
                        f'{height:.1f}',
                        ha='center', va='bottom', fontsize=10)

        add_labels(bars1)
        add_labels(bars2)

        # Set title and labels
        title = f'Latency Comparison (Threads: {threads}, Message Size: {msg_size})'
        if queue_size != 'N/A' and worker_threads != 'N/A':
            title += f'\nQueue Size: {queue_size}, Worker Threads: {worker_threads}'
        ax.set_title(title, fontsize=15)
        ax.set_ylabel('Latency (microseconds)', fontsize=13)
        ax.set_xticks(x)
        ax.set_xticklabels(metric_names, fontsize=12)
        ax.legend(fontsize=12)

        # Calculate median latency performance difference percentage
        if len(mm_logger_metrics) > 1 and len(spdlog_metrics) > 1:
            # Assuming the second metric is always the median (index 1)
            median_index = 1 if len(mm_logger_metrics) > 1 else 0
            if spdlog_metrics[median_index] > 0:
                diff_pct = calculate_improvement(mm_logger_metrics[median_index], spdlog_metrics[median_index])
                if diff_pct < 0:  # Lower latency is better
                    diff_text = f"MM-Logger median latency is {-diff_pct:.1f}% lower than spdlog"
                    text_color = 'green'
                else:
                    diff_text = f"MM-Logger median latency is {diff_pct:.1f}% higher than spdlog"
                    text_color = 'red'
                ax.text(0.5, 0.92, diff_text, transform=ax.transAxes,
                       ha='center', fontsize=13, color=text_color)

        # Save chart
        filename = f"latency_comparison_t{threads}_m{msg_size}_q{queue_size}_w{worker_threads}.png"
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, filename))
        plt.close()

        comparison_count += 1

    if comparison_count > 0:
        print(f"Generated {comparison_count} latency comparison charts")
    else:
        # Create a simplified comparison using aggregate data if detailed comparison is not possible
        try:
            # Only use median latency for simplified comparison
            if 'Median_Latency_us' in comparison_data.columns:
                mm_logger_latency = comparison_data[comparison_data['Logger'] == 'mm_logger']['Median_Latency_us'].mean()
                spdlog_latency = comparison_data[comparison_data['Logger'] == 'spdlog']['Median_Latency_us'].mean()

                if pd.notna(mm_logger_latency) and pd.notna(spdlog_latency) and mm_logger_latency > 0 and spdlog_latency > 0:
                    fig, ax = plt.subplots(figsize=(10, 6))
                    bars = ax.bar(['MM-Logger', 'spdlog'], [mm_logger_latency, spdlog_latency],
                                  color=['#4285F4', '#EA4335'], width=0.6)

                    # Add value labels
                    for bar in bars:
                        height = bar.get_height()
                        ax.text(bar.get_x() + bar.get_width()/2., height + 1,
                                f'{height:.1f}',
                                ha='center', va='bottom', fontsize=10)

                    ax.set_title('Overall Median Latency Comparison', fontsize=15)
                    ax.set_ylabel('Latency (microseconds)', fontsize=13)

                    if spdlog_latency > 0:
                        diff_pct = calculate_improvement(mm_logger_latency, spdlog_latency)
                        if diff_pct < 0:  # Lower latency is better
                            diff_text = f"MM-Logger median latency is {-diff_pct:.1f}% lower than spdlog"
                            text_color = 'green'
                        else:
                            diff_text = f"MM-Logger median latency is {diff_pct:.1f}% higher than spdlog"
                            text_color = 'red'
                        ax.text(0.5, 0.9, diff_text, transform=ax.transAxes,
                               ha='center', fontsize=14, color=text_color)

                    filename = "latency_comparison_overall.png"
                    plt.tight_layout()
                    plt.savefig(os.path.join(output_dir, filename))
                    plt.close()

                    print("Generated simplified latency comparison chart")
                else:
                    print("Warning: Could not generate latency comparison charts, insufficient data")
            else:
                print("Warning: Could not generate latency comparison charts, Median_Latency_us column missing")
        except Exception as e:
            print(f"Error generating simplified latency comparison: {e}")
            print("Warning: Could not generate latency comparison charts, insufficient data")

def plot_thread_scaling(data, output_dir):
    """Draw thread scalability performance chart"""
    # Try using the extracted ThreadCount column
    thread_data = data[data['ThreadCount'].notna()]

    # If no ThreadCount data, filter using TestCategory
    if len(thread_data) < 3:
        thread_data = data[data['TestCategory'] == 'thread_scaling']

    # Try to use Threads column as fallback
    if len(thread_data) < 3 and 'Threads' in data.columns:
        thread_data = data[data['Threads'].notna()]

    # If still not enough data, abort
    if len(thread_data) < 3:
        print("Warning: Not enough thread scaling test data")
        return

    # Ensure we have a column to use for thread count
    thread_column = 'ThreadCount' if 'ThreadCount' in thread_data.columns else 'Threads'
    if thread_column not in thread_data.columns:
        print(f"Error: Missing {thread_column} column for thread scaling chart")
        return

    # Group by Logger type
    grouped = thread_data.groupby('Logger')

    plt.figure(figsize=(12, 8))

    # Draw thread count vs throughput curves
    for i, (name, group) in enumerate(grouped):
        # Use thread count column
        thread_counts = group[thread_column]

        # Sort by thread count
        sorted_data = group.sort_values(thread_column)

        # Draw thread count vs throughput curve
        plt.plot(sorted_data[thread_column], sorted_data['LogsPerSecond'],
                marker='o', label=name, linewidth=2, color=colors[i % len(colors)])

    plt.title('Thread Count Scalability', fontsize=16)
    plt.xlabel('Thread Count', fontsize=14)
    plt.ylabel('Logs Per Second', fontsize=14)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(fontsize=12)

    # Use log scale for X-axis to make scaling trends clearer
    if len(thread_data[thread_column].dropna().unique()) > 2:
        plt.xscale('log', base=2)

    # Save chart
    filename = "thread_scaling.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()

    print(f"Generated thread scaling chart: {filename}")

def plot_message_size_impact(data, output_dir):
    """Draw the impact of message size on performance"""
    # Filter message size test data
    size_data = data[data['MessageSizeType'].notna()].copy()

    if len(size_data) < 3:
        size_data = data[data['TestCategory'] == 'message_size'].copy()

    # Try to use MessageSize column as fallback
    if len(size_data) < 3 and 'MessageSize' in data.columns:
        size_data = data[data['MessageSize'].notna()].copy()

    if len(size_data) < 3:
        print("Warning: Not enough message size test data")
        return

    # Determine which column to use
    size_column = 'MessageSizeType' if 'MessageSizeType' in size_data.columns else 'MessageSize'
    if size_column not in size_data.columns:
        print(f"Error: Missing {size_column} column for message size impact chart")
        return

    # Set correct sort order
    size_order = {'small': 0, 'medium': 1, 'large': 2}
    size_labels = {'small': 'Small', 'medium': 'Medium', 'large': 'Large'}

    # Ensure SizeOrder column exists
    if 'SizeOrder' not in size_data.columns:
        size_data.loc[:, 'SizeOrder'] = size_data[size_column].map(size_order)

    # Group by Logger type
    grouped = size_data.groupby('Logger')

    # Create chart
    plt.figure(figsize=(12, 8))

    width = 0.35
    x_positions = []
    bar_labels = []

    # Draw bar chart
    for i, (name, group) in enumerate(grouped):
        # Sort and get all size types
        sizes = group[size_column].unique()

        # Create x positions for each message size
        x_pos = np.arange(len(sizes)) + i * width
        x_positions.extend(x_pos)

        # Create labels
        size_label_list = []
        for size in sizes:
            label = size_labels.get(size, size) if size in size_labels else size
            size_label_list.append(f"{label}\n({name})")
        bar_labels.extend(size_label_list)

        # Get throughput data for each size
        throughputs = []
        for size in sizes:
            size_throughput = group[group[size_column] == size]['LogsPerSecond'].mean()
            throughputs.append(size_throughput)

        # Draw bar chart
        plt.bar(x_pos, throughputs, width=width, label=name, color=colors[i % len(colors)])

    # Set x-axis labels
    plt.xticks(x_positions, bar_labels, fontsize=11)

    plt.title('Impact of Message Size on Throughput', fontsize=16)
    plt.ylabel('Logs Per Second', fontsize=14)
    plt.grid(True, linestyle='--', alpha=0.7, axis='y')
    plt.legend(fontsize=12)

    # Save chart
    filename = "message_size_impact.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()

    print(f"Generated message size impact chart: {filename}")

def plot_queue_size_impact(data, output_dir):
    """Draw queue size impact chart"""
    # Filter queue size test data
    queue_data = data[data['ExtractedQueueSize'].notna()]

    if len(queue_data) < 3:
        queue_data = data[data['TestCategory'] == 'queue_size']

    # Try to use QueueSize column as fallback
    if len(queue_data) < 3 and 'QueueSize' in data.columns:
        queue_data = data[data['QueueSize'].notna()]

    if len(queue_data) < 3:
        print("Warning: Not enough queue size test data")
        return

    # Determine which column to use
    queue_column = None
    for col in ['ExtractedQueueSize', 'QueueSize']:
        if col in queue_data.columns and queue_data[col].notna().any():
            queue_column = col
            break

    if not queue_column:
        print("Error: No valid queue size column found")
        return

    # Group by Logger type
    grouped = queue_data.groupby('Logger')

    plt.figure(figsize=(12, 8))

    # Draw queue size vs throughput curves
    for i, (name, group) in enumerate(grouped):
        # Sort by queue size
        sorted_data = group.sort_values(queue_column)

        # Draw queue size vs throughput curve
        plt.plot(sorted_data[queue_column], sorted_data['LogsPerSecond'],
                marker='o', label=name, linewidth=2, color=colors[i % len(colors)])

    plt.title('Impact of Async Queue Size on Throughput', fontsize=16)
    plt.xlabel('Queue Size (entries)', fontsize=14)
    plt.ylabel('Logs Per Second', fontsize=14)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(fontsize=12)

    # Queue sizes typically grow as powers of 2, use log scale
    if len(queue_data[queue_column].dropna().unique()) > 2:
        plt.xscale('log', base=2)

    # Save chart
    filename = "queue_size_impact.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()

    print(f"Generated queue size impact chart: {filename}")

def plot_worker_threads_impact(data, output_dir):
    """Draw worker threads impact chart"""
    # Filter worker threads test data
    worker_data = data[data['ExtractedWorkerThreads'].notna()]

    if len(worker_data) < 3:
        worker_data = data[data['TestCategory'] == 'worker_threads']

    # Try to use WorkerThreads column as fallback
    if len(worker_data) < 3 and 'WorkerThreads' in data.columns:
        worker_data = data[data['WorkerThreads'].notna()]

    if len(worker_data) < 3:
        print("Warning: Not enough worker threads test data")
        return

    # Determine which column to use
    worker_column = None
    for col in ['ExtractedWorkerThreads', 'WorkerThreads']:
        if col in worker_data.columns and worker_data[col].notna().any():
            worker_column = col
            break

    if not worker_column:
        print("Error: No valid worker threads column found")
        return

    # Group by Logger type
    grouped = worker_data.groupby('Logger')

    plt.figure(figsize=(12, 8))

    # Draw worker threads vs throughput curves
    for i, (name, group) in enumerate(grouped):
        # Sort by worker threads
        sorted_data = group.sort_values(worker_column)

        # Draw worker threads vs throughput curve
        plt.plot(sorted_data[worker_column], sorted_data['LogsPerSecond'],
                marker='o', label=name, linewidth=2, color=colors[i % len(colors)])

    plt.title('Impact of Async Worker Threads on Throughput', fontsize=16)
    plt.xlabel('Worker Threads', fontsize=14)
    plt.ylabel('Logs Per Second', fontsize=14)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(fontsize=12)

    # Save chart
    filename = "worker_threads_impact.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()

    print(f"Generated worker threads impact chart: {filename}")

def plot_stress_test_results(data, output_dir):
    """Draw stress test results"""
    # Filter stress test data
    stress_data = data[data['TestCategory'].str.contains('stress', na=False)]

    if len(stress_data) < 2:
        stress_data = data[data['TestType'].str.contains('stress', na=False)]

    if len(stress_data) < 2:
        print("Warning: Not enough stress test data")
        return

    # Extract stress test types
    stress_data = stress_data.copy()
    stress_data.loc[:, 'StressType'] = 'Standard Stress Test'  # Default value

    # Try to determine stress test types from test names
    for idx, row in stress_data.iterrows():
        test_name = str(row['TestName']).lower()
        if 'small_bursts' in test_name or '小型突发' in test_name:
            stress_data.at[idx, 'StressType'] = "Small Bursts"
        elif 'medium_bursts' in test_name or '中型突发' in test_name:
            stress_data.at[idx, 'StressType'] = "Medium Bursts"
        elif 'large_burst' in test_name or '大型突发' in test_name:
            stress_data.at[idx, 'StressType'] = "Large Burst"

    # Group by Logger type and StressType
    stress_categories = stress_data['StressType'].unique()
    loggers = stress_data['Logger'].unique()

    # Create x-axis positions
    x = np.arange(len(stress_categories))
    width = 0.8 / len(loggers) if len(loggers) > 0 else 0.4  # Adjust bar width

    fig, ax = plt.subplots(figsize=(14, 8))

    # Draw bar chart for each Logger
    for i, logger in enumerate(loggers):
        throughputs = []
        for stress_type in stress_categories:
            # Get data for specific stress type and Logger
            subset = stress_data[(stress_data['StressType'] == stress_type) &
                              (stress_data['Logger'] == logger)]
            if len(subset) > 0:
                throughputs.append(subset['LogsPerSecond'].mean())
            else:
                throughputs.append(0)

        # Calculate bar positions
        positions = x - 0.4 + (i + 0.5) * width

        # Draw bar chart
        bars = ax.bar(positions, throughputs, width, label=logger, color=colors[i % len(colors)])

        # Add value labels
        for bar in bars:
            height = bar.get_height()
            if height > 0:  # Only add labels for non-zero values
                ax.text(bar.get_x() + bar.get_width() / 2, height * 1.01,
                       f'{int(height):,}', ha='center', va='bottom', fontsize=10)

    # Set x-axis labels
    ax.set_xticks(x)
    ax.set_xticklabels(stress_categories, fontsize=12)

    # Set title and labels
    ax.set_title('Throughput Performance Under Different Stress Patterns', fontsize=16)
    ax.set_ylabel('Logs Per Second', fontsize=14)
    ax.grid(True, linestyle='--', alpha=0.7, axis='y')
    ax.legend(fontsize=12)

    # Save chart
    filename = "stress_test_results.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()

    print(f"Generated stress test results chart: {filename}")

def plot_console_impact(data, output_dir):
    """Draw the impact of console output on performance"""
    # Check if EnableConsole column exists
    if 'EnableConsole' not in data.columns:
        print("Warning: EnableConsole column not found, cannot generate console impact chart")
        return

    # Find paired tests with EnableConsole true and false
    console_on = data[data['EnableConsole'] == True]
    console_off = data[data['EnableConsole'] == False]

    # Ensure there's enough data
    if len(console_on) < 2 or len(console_off) < 2:
        print(f"Warning: Not enough console output impact test data (On: {len(console_on)}, Off: {len(console_off)})")
        return

    # Group by Logger
    loggers = data['Logger'].unique()

    # Create chart
    fig, ax = plt.subplots(figsize=(12, 8))

    x = np.arange(len(loggers))
    width = 0.35

    # Store results for labels
    impact_percentages = {}

    # Draw bar chart
    for i, logger in enumerate(loggers):
        # Find console_on and console_off data for this Logger
        logger_on = console_on[console_on['Logger'] == logger]
        logger_off = console_off[console_off['Logger'] == logger]

        if len(logger_on) == 0 or len(logger_off) == 0:
            continue

        # Calculate average throughput
        on_throughput = logger_on['LogsPerSecond'].mean()
        off_throughput = logger_off['LogsPerSecond'].mean()

        # Draw bar chart
        bars1 = ax.bar(x[i] - width/2, on_throughput, width, label='Console Enabled' if i == 0 else "", color='#4285F4')
        bars2 = ax.bar(x[i] + width/2, off_throughput, width, label='Console Disabled' if i == 0 else "", color='#34A853')

        # Add value labels
        ax.text(x[i] - width/2, on_throughput * 1.01, f'{int(on_throughput):,}',
               ha='center', va='bottom', fontsize=10)
        ax.text(x[i] + width/2, off_throughput * 1.01, f'{int(off_throughput):,}',
               ha='center', va='bottom', fontsize=10)

        # Calculate impact percentage
        if on_throughput > 0:
            impact_pct = ((off_throughput / on_throughput) - 1) * 100
            impact_percentages[logger] = impact_pct

    # Set x-axis labels
    ax.set_xticks(x)
    ax.set_xticklabels(loggers, fontsize=12)

    # Add impact percentage labels
    for i, logger in enumerate(impact_percentages.keys()):
        if logger in impact_percentages:
            impact_pct = impact_percentages[logger]
            label_text = f"+{impact_pct:.1f}%" if impact_pct > 0 else f"{impact_pct:.1f}%"
            color = 'green' if impact_pct > 0 else 'red'
            ax.text(x[i], max(console_on['LogsPerSecond'].max(), console_off['LogsPerSecond'].max()) * 1.05,
                   f"Impact: {label_text}", ha='center', color=color, fontsize=12, fontweight='bold')

    # Set title and labels
    ax.set_title('Impact of Console Output on Throughput', fontsize=16)
    ax.set_ylabel('Logs Per Second', fontsize=14)
    ax.grid(True, linestyle='--', alpha=0.7, axis='y')
    ax.legend(fontsize=12)

    # Save chart
    filename = "console_output_impact.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()

    print(f"Generated console output impact chart: {filename}")

def plot_file_size_impact(data, output_dir):
    """Draw the impact of file size settings on performance"""
    # Filter file size test data
    file_size_data = data[data['FileSize'].notna() | data['TotalFileSize'].notna()]

    if len(file_size_data) < 3:
        print("Warning: Not enough file size test data")
        return

    # Create single file size chart
    single_file_data = file_size_data[file_size_data['FileSize'].notna()].copy()
    if len(single_file_data) >= 3:
        # Group by Logger
        grouped = single_file_data.groupby('Logger')

        plt.figure(figsize=(12, 8))

        # Draw file size vs throughput curves
        for i, (name, group) in enumerate(grouped):
            # Sort by file size
            sorted_data = group.sort_values('FileSize')

            # Draw curve
            plt.plot(sorted_data['FileSize'], sorted_data['LogsPerSecond'],
                    marker='o', label=name, linewidth=2, color=colors[i % len(colors)])

        plt.title('Impact of Single Log File Size Limit on Throughput', fontsize=16)
        plt.xlabel('File Size Limit (MB)', fontsize=14)
        plt.ylabel('Logs Per Second', fontsize=14)
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.legend(fontsize=12)

        # Save chart
        filename = "single_file_size_impact.png"
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, filename))
        plt.close()

        print(f"Generated single file size impact chart: {filename}")

    # Create total file size chart
    total_file_data = file_size_data[file_size_data['TotalFileSize'].notna()].copy()
    if len(total_file_data) >= 3:
        # Group by Logger
        grouped = total_file_data.groupby('Logger')

        plt.figure(figsize=(12, 8))

        # Draw total file size vs throughput curves
        for i, (name, group) in enumerate(grouped):
            # Sort by total file size
            sorted_data = group.sort_values('TotalFileSize')

            # Draw curve
            plt.plot(sorted_data['TotalFileSize'], sorted_data['LogsPerSecond'],
                    marker='o', label=name, linewidth=2, color=colors[i % len(colors)])

        plt.title('Impact of Total Log File Size Limit on Throughput', fontsize=16)
        plt.xlabel('Total Size Limit (MB)', fontsize=14)
        plt.ylabel('Logs Per Second', fontsize=14)
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.legend(fontsize=12)

        # Save chart
        filename = "total_file_size_impact.png"
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, filename))
        plt.close()

        print(f"Generated total file size impact chart: {filename}")
