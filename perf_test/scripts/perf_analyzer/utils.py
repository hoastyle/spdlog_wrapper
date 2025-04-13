#!/usr/bin/env python3
"""
Utility functions module - Provides common utility functions
"""

import os
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
import re
import locale

def create_output_dir(output_dir):
    """Create output directory"""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    return output_dir

def configure_plotting():
    """Configure matplotlib plotting settings"""
    # Set font configuration
    plt.rcParams['font.sans-serif'] = ['Arial', 'DejaVu Sans', 'sans-serif']
    plt.rcParams['axes.unicode_minus'] = False  # Correctly display negative signs

    # Set plotting style
    try:
        # Try newest matplotlib style format
        plt.style.use('seaborn-v0_8-darkgrid')
    except:
        try:
            # Try older matplotlib style format
            plt.style.use('seaborn-darkgrid')
        except:
            # Fall back to a basic style
            plt.style.use('ggplot')

    # Set DPI for clearer images
    plt.rcParams['figure.dpi'] = 120

    # Set figure size
    plt.rcParams['figure.figsize'] = (12, 8)

    # Set font sizes
    plt.rcParams['font.size'] = 12
    plt.rcParams['axes.titlesize'] = 16
    plt.rcParams['axes.labelsize'] = 14

def add_value_labels(ax, bars):
    """Add value labels to bar charts"""
    for bar in bars:
        height = bar.get_height()
        # Skip labels for very small values
        if height < 0.01:
            continue

        # Format the value
        if height >= 10000:
            value_text = f'{int(height):,}'  # Use commas for thousands separator
        elif height >= 100:
            value_text = f'{int(height)}'
        elif height >= 1:
            value_text = f'{height:.1f}'
        else:
            value_text = f'{height:.2f}'

        ax.annotate(value_text,
                   xy=(bar.get_x() + bar.get_width() / 2, height),
                   xytext=(0, 3),  # 3 points vertical offset
                   textcoords="offset points",
                   ha='center', va='bottom',
                   fontsize=10)

def calculate_improvement(value1, value2):
    """Calculate percentage improvement between two values"""
    if value2 == 0:
        return 0
    return ((value1 / value2) - 1) * 100

def get_current_timestamp():
    """Get current timestamp string"""
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def format_number(num):
    """Format number with thousands separator for readability"""
    try:
        # Try to set locale to user's default
        locale.setlocale(locale.LC_ALL, '')
    except:
        pass

    try:
        if num >= 1000:
            # Format with commas as thousands separators
            return f"{num:,.2f}"
        elif num >= 100:
            return f"{num:.1f}"
        else:
            return f"{num:.2f}"
    except:
        # Fall back to simple formatting
        return f"{num:.2f}"

def get_clean_test_name(test_name):
    """Get user-friendly display name from test name"""
    if not isinstance(test_name, str):
        return "Unknown Test"

    # Convert to lowercase for case-insensitive matching
    test_name = test_name.lower()

    # Handle thread tests
    if 'throughput_threads' in test_name:
        threads_match = re.search(r'threads_(\d+)', test_name)
        if threads_match:
            return f"{threads_match.group(1)} Threads"

    # Handle message size tests
    if 'msgsize' in test_name:
        size = None
        if 'small' in test_name:
            size = "Small Messages"
        elif 'medium' in test_name:
            size = "Medium Messages"
        elif 'large' in test_name:
            size = "Large Messages"
        if size:
            return size

    # Handle queue size tests
    if 'queue' in test_name:
        queue_match = re.search(r'queue_(\d+)', test_name)
        if queue_match:
            return f"Queue Size {queue_match.group(1)}"

    # Handle worker threads tests
    if 'workers' in test_name:
        workers_match = re.search(r'workers_(\d+)', test_name)
        if workers_match:
            return f"{workers_match.group(1)} Worker Threads"

    # Handle stress tests
    if 'stress' in test_name:
        if 'small_bursts' in test_name:
            return "Small Burst Stress"
        elif 'medium_bursts' in test_name:
            return "Medium Burst Stress"
        elif 'large_burst' in test_name:
            return "Large Burst Stress"
        else:
            return "Stress Test"

    # Handle latency tests
    if 'latency' in test_name:
        return "Latency Test"

    # Handle comparison tests
    if 'compare' in test_name:
        return "Comparison Test"

    # Fall back to original name
    return test_name.replace('_', ' ').title()

def extract_test_info(test_name):
    """Extract test information from test name"""
    if not isinstance(test_name, str):
        return {}

    # Convert to lowercase for case-insensitive matching
    test_name = test_name.lower()

    info = {}

    # Extract thread count
    threads_match = re.search(r'threads_(\d+)', test_name)
    if threads_match:
        info['threads'] = int(threads_match.group(1))

    # Extract message size
    if 'msgsize_small' in test_name or 'small_message' in test_name:
        info['message_size'] = 'small'
    elif 'msgsize_medium' in test_name or 'medium_message' in test_name:
        info['message_size'] = 'medium'
    elif 'msgsize_large' in test_name or 'large_message' in test_name:
        info['message_size'] = 'large'

    # Extract queue size
    queue_match = re.search(r'queue_(\d+)', test_name)
    if queue_match:
        info['queue_size'] = int(queue_match.group(1))

    # Extract worker threads
    workers_match = re.search(r'workers_(\d+)', test_name)
    if workers_match:
        info['worker_threads'] = int(workers_match.group(1))

    # Extract test type
    if 'throughput' in test_name:
        info['type'] = 'throughput'
    elif 'latency' in test_name:
        info['type'] = 'latency'
    elif 'stress' in test_name:
        info['type'] = 'stress'
    elif 'compare' in test_name:
        info['type'] = 'comparison'

    # Extract stress test type
    if 'small_bursts' in test_name:
        info['stress_type'] = 'small_bursts'
    elif 'medium_bursts' in test_name:
        info['stress_type'] = 'medium_bursts'
    elif 'large_burst' in test_name:
        info['stress_type'] = 'large_burst'

    return info

def format_bytes(size):
    """Format bytes into human-readable format"""
    # Handle invalid input
    if not isinstance(size, (int, float)) or size < 0:
        return "0 B"

    # Define units
    units = ['B', 'KB', 'MB', 'GB', 'TB']

    # Calculate appropriate unit
    unit_index = 0
    while size >= 1024.0 and unit_index < len(units) - 1:
        size /= 1024.0
        unit_index += 1

    # Format with appropriate precision
    if size >= 100:
        formatted = f"{size:.1f}"
    else:
        formatted = f"{size:.2f}"

    return f"{formatted} {units[unit_index]}"

def parse_size_string(size_str):
    """Parse size string (e.g., '10MB', '1.5GB') into bytes"""
    if not isinstance(size_str, str):
        return None

    # Remove whitespace and convert to lowercase
    size_str = size_str.strip().lower()

    # Extract number and unit
    match = re.match(r'^([\d.]+)\s*([kmgt]?b)?$', size_str)
    if not match:
        return None

    value = float(match.group(1))
    unit = match.group(2)

    # Convert to bytes
    if unit == 'kb':
        value *= 1024
    elif unit == 'mb':
        value *= 1024 * 1024
    elif unit == 'gb':
        value *= 1024 * 1024 * 1024
    elif unit == 'tb':
        value *= 1024 * 1024 * 1024 * 1024

    return int(value)

def safe_divide(a, b, default=0):
    """Safely divide two numbers, returning default if division by zero"""
    return a / b if b != 0 else default
