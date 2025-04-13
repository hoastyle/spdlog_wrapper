#!/usr/bin/env python3
"""
Data loading module - Responsible for loading and preprocessing performance test results
"""

import os
import sys
import pandas as pd
import numpy as np
import re

def load_results(csv_file):
    """Load and preprocess performance test results"""
    if not os.path.exists(csv_file):
        print(f"Error: Results file {csv_file} does not exist")
        sys.exit(1)

    try:
        # Load CSV data
        data = pd.read_csv(csv_file)
        original_len = len(data)
        print(f"Loaded {original_len} performance test results")

        # Check for required columns
        required_columns = ['TestName', 'TestType', 'Logger', 'LogsPerSecond']
        missing_columns = [col for col in required_columns if col not in data.columns]
        if missing_columns:
            print(f"Warning: Missing required columns: {', '.join(missing_columns)}")
            print("Attempting to continue with available data...")

        # Handle boolean columns that might be loaded as strings
        bool_columns = ['EnableConsole', 'EnableFile']
        for col in bool_columns:
            if col in data.columns:
                # Convert string 'true'/'false' to boolean
                if data[col].dtype == 'object':
                    data[col] = data[col].str.lower().map({'true': True, 'false': False})

        # Clean data
        data = clean_data(data)

        # Add derived columns for analysis
        data = enrich_data(data)

        if len(data) < original_len:
            print(f"Warning: After data cleaning, {len(data)} records remain (removed {original_len - len(data)} invalid records)")

        return data
    except Exception as e:
        print(f"Error loading results file: {e}")
        sys.exit(1)

def clean_data(data):
    """Clean data, remove invalid records"""
    # Make a copy to avoid warnings
    clean_data = data.copy()

    # Remove records with LogsPerSecond <= 0 (if the column exists)
    if 'LogsPerSecond' in clean_data.columns:
        clean_data = clean_data[clean_data['LogsPerSecond'] > 0]

    # Handle NaN values - fill with defaults for critical columns
    if 'Threads' in clean_data.columns:
        clean_data['Threads'].fillna(1, inplace=True)
    if 'Iterations' in clean_data.columns:
        clean_data['Iterations'].fillna(1000, inplace=True)
    if 'QueueSize' in clean_data.columns:
        clean_data['QueueSize'].fillna(8192, inplace=True)
    if 'WorkerThreads' in clean_data.columns:
        clean_data['WorkerThreads'].fillna(1, inplace=True)

    # Use IQR method to detect outliers, but be more lenient
    if 'LogsPerSecond' in clean_data.columns and len(clean_data) > 5:
        Q1 = clean_data['LogsPerSecond'].quantile(0.25)
        Q3 = clean_data['LogsPerSecond'].quantile(0.75)
        IQR = Q3 - Q1

        # Define outlier boundaries (3 * IQR is more lenient than the standard 1.5 * IQR)
        lower_bound = Q1 - 3 * IQR
        upper_bound = Q3 + 3 * IQR

        # Remove extreme outliers
        data_filtered = clean_data[(clean_data['LogsPerSecond'] >= lower_bound) &
                             (clean_data['LogsPerSecond'] <= upper_bound)]

        # Only use filtered data if we haven't removed too many records
        if len(data_filtered) >= len(clean_data) * 0.8:
            clean_data = data_filtered
        else:
            print("Warning: Outlier detection was too strict, using original data")

    return clean_data

def enrich_data(data):
    """Add derived columns for analysis"""
    # Make a copy to avoid warnings
    enriched_data = data.copy()

    # Add test category
    enriched_data['TestCategory'] = enriched_data['TestName'].apply(categorize_test)

    # Extract thread count
    enriched_data['ThreadCount'] = enriched_data.apply(
        lambda row: extract_thread_count(row['TestName']) if pd.isna(row.get('Threads', np.nan)) else row.get('Threads', np.nan),
        axis=1
    )

    # Extract message size type
    enriched_data['MessageSizeType'] = enriched_data.apply(
        lambda row: extract_message_size(row['TestName']) if pd.isna(row.get('MessageSize', np.nan)) else row.get('MessageSize', np.nan),
        axis=1
    )

    # Extract queue size
    enriched_data['ExtractedQueueSize'] = enriched_data.apply(
        lambda row: extract_queue_size(row['TestName']) if pd.isna(row.get('QueueSize', np.nan)) else row.get('QueueSize', np.nan),
        axis=1
    )

    # Extract worker threads
    enriched_data['ExtractedWorkerThreads'] = enriched_data.apply(
        lambda row: extract_worker_threads(row['TestName']) if pd.isna(row.get('WorkerThreads', np.nan)) else row.get('WorkerThreads', np.nan),
        axis=1
    )

    # Extract file size
    enriched_data['FileSize'] = enriched_data['TestName'].apply(extract_file_size)

    # Extract total file size
    enriched_data['TotalFileSize'] = enriched_data['TestName'].apply(extract_total_file_size)

    # Standardize columns
    enriched_data = standardize_columns(enriched_data)

    # Extract additional information from TestType
    enriched_data = extract_from_test_type(enriched_data)

    # Try to extract from TestType if TestName didn't yield results
    # This helps with 'comparison_throughput' and similar test types
    if 'TestType' in enriched_data.columns:
        for idx, row in enriched_data.iterrows():
            if pd.isna(row.get('TestCategory', np.nan)) and not pd.isna(row.get('TestType', np.nan)):
                if 'throughput' in str(row['TestType']).lower():
                    enriched_data.at[idx, 'TestCategory'] = 'throughput'
                elif 'latency' in str(row['TestType']).lower():
                    enriched_data.at[idx, 'TestCategory'] = 'latency'
                elif 'stress' in str(row['TestType']).lower():
                    enriched_data.at[idx, 'TestCategory'] = 'stress'
                elif 'comparison' in str(row['TestType']).lower():
                    enriched_data.at[idx, 'TestCategory'] = 'comparison'

    return enriched_data

def extract_from_test_type(data):
    """Try to extract additional information from TestType column"""
    # Make a copy to avoid warnings
    result_data = data.copy()

    if 'TestType' not in result_data.columns:
        return result_data

    for idx, row in result_data.iterrows():
        test_type = str(row.get('TestType', '')).lower()

        # If ThreadCount is empty, try to extract from TestType
        if pd.isna(row.get('ThreadCount', np.nan)):
            thread_count = extract_thread_count(test_type)
            if thread_count is not None:
                result_data.at[idx, 'ThreadCount'] = thread_count

        # If MessageSizeType is empty, try to extract from TestType
        if pd.isna(row.get('MessageSizeType', np.nan)):
            message_size = extract_message_size(test_type)
            if message_size is not None:
                result_data.at[idx, 'MessageSizeType'] = message_size

        # If ExtractedQueueSize is empty, try to extract from TestType
        if pd.isna(row.get('ExtractedQueueSize', np.nan)):
            queue_size = extract_queue_size(test_type)
            if queue_size is not None:
                result_data.at[idx, 'ExtractedQueueSize'] = queue_size

        # If ExtractedWorkerThreads is empty, try to extract from TestType
        if pd.isna(row.get('ExtractedWorkerThreads', np.nan)):
            worker_threads = extract_worker_threads(test_type)
            if worker_threads is not None:
                result_data.at[idx, 'ExtractedWorkerThreads'] = worker_threads

    return result_data

def extract_thread_count(test_name):
    """Extract thread count from test name"""
    if not isinstance(test_name, str):
        return None

    test_name = test_name.lower()  # Convert to lowercase for case-insensitive matching

    # Expanded matching patterns
    patterns = [
        r'threads[_\-]?(\d+)',              # threads_8, threads-8
        r'thread[_\-]?(\d+)',               # thread_8, thread-8
        r't[_\-]?(\d+)',                    # t_8, t-8
        r'throughput_threads_(\d+)',        # throughput_threads_8
        r'(\d+)[_\-]?threads',              # 8threads, 8_threads
        r'(\d+)threads',                    # 8threads
        r'线程[数]?[_\-]?(\d+)',            # 线程数8, 线程_8
        r'(\d+)[_\-]?线程',                 # 8线程, 8_线程
        r'高线程数[_\-]?(\d+)',             # 高线程数8
        r'并发[_\-]?(\d+)'                  # 并发8
    ]

    for pattern in patterns:
        match = re.search(pattern, test_name)
        if match:
            try:
                thread_count = int(match.group(1))
                if 0 < thread_count < 10000:  # Reasonable thread count range
                    return thread_count
            except (ValueError, IndexError):
                continue

    return None

def extract_message_size(test_name):
    """Extract message size type from test name"""
    if not isinstance(test_name, str):
        return None

    test_name = test_name.lower()  # Convert to lowercase for case-insensitive matching

    # Check for direct message size type keywords
    if any(term in test_name for term in ['msgsize_small', 'small_message', 'small消息', '小消息', '小型消息']):
        return 'small'
    elif any(term in test_name for term in ['msgsize_medium', 'medium_message', 'medium消息', '中消息', '中型消息']):
        return 'medium'
    elif any(term in test_name for term in ['msgsize_large', 'large_message', 'large消息', '大消息', '大型消息']):
        return 'large'

    # Use more generic patterns
    size_patterns = {
        'small': [r'small', r'小[型]?[消息]', r'msg[_\-]?s'],
        'medium': [r'medium', r'中[型]?[消息]', r'msg[_\-]?m'],
        'large': [r'large', r'大[型]?[消息]', r'msg[_\-]?l']
    }

    for size, patterns in size_patterns.items():
        for pattern in patterns:
            if re.search(pattern, test_name):
                return size

    return None

def extract_queue_size(test_name):
    """Extract queue size from test name"""
    if not isinstance(test_name, str):
        return None

    test_name = test_name.lower()  # Convert to lowercase for case-insensitive matching

    # Expanded matching patterns
    patterns = [
        r'queue[_\-]?size[_\-]?(\d+)',      # queue_size_8192
        r'queue[_\-]?(\d+)',                # queue_8192, queue-8192
        r'队列[大小]?[_\-]?(\d+)',           # 队列大小8192, 队列_8192
        r'队列[_\-]?(\d+)',                  # 队列8192
        r'(\d+)[_\-]?队列',                  # 8192队列
        r'q[_\-]?(\d+)',                    # q_8192
        r'async[_\-]?(\d+)',                # async_8192
        r'异步队列[_\-]?(\d+)'               # 异步队列8192
    ]

    for pattern in patterns:
        match = re.search(pattern, test_name)
        if match:
            try:
                queue_size = int(match.group(1))
                if 0 < queue_size < 1000000:  # Reasonable queue size range
                    return queue_size
            except (ValueError, IndexError):
                continue

    return None

def extract_worker_threads(test_name):
    """Extract worker threads count from test name"""
    if not isinstance(test_name, str):
        return None

    test_name = test_name.lower()  # Convert to lowercase for case-insensitive matching

    # Expanded matching patterns
    patterns = [
        r'workers[_\-]?(\d+)',              # workers_4, workers-4
        r'worker[_\-]?threads[_\-]?(\d+)',  # worker_threads_4
        r'工作线程[数]?[_\-]?(\d+)',         # 工作线程数4, 工作线程_4
        r'(\d+)[_\-]?工作线程',              # 4工作线程, 4_工作线程
        r'线程数[_\-]?(\d+)',                # 线程数4
        r'w[_\-]?(\d+)',                    # w_4, w-4
        r'async[_\-]?workers[_\-]?(\d+)',   # async_workers_4
        r'异步工作线程[_\-]?(\d+)'            # 异步工作线程4
    ]

    for pattern in patterns:
        match = re.search(pattern, test_name)
        if match:
            try:
                worker_threads = int(match.group(1))
                if 0 < worker_threads < 100:  # Reasonable worker thread count range
                    return worker_threads
            except (ValueError, IndexError):
                continue

    return None

def extract_file_size(test_name):
    """Extract file size from test name"""
    if not isinstance(test_name, str):
        return None

    test_name = test_name.lower()  # Convert to lowercase for case-insensitive matching

    # Expanded matching patterns
    patterns = [
        r'file[_\-]?size[_\-]?(\d+)[_\-]?mb',    # file_size_10_mb
        r'file[_\-]?size[_\-]?(\d+)',            # file_size_10
        r'文件[大小]?[_\-]?(\d+)[_\-]?mb',         # 文件大小10mb, 文件_10_mb
        r'日志文件[_\-]?(\d+)[_\-]?mb',            # 日志文件10mb
        r'max[_\-]?file[_\-]?size[_\-]?(\d+)',   # max_file_size_10
        r'单文件[_\-]?(\d+)[_\-]?mb',             # 单文件10mb
        r'fs[_\-]?(\d+)[_\-]?mb'                 # fs_10_mb
    ]

    for pattern in patterns:
        match = re.search(pattern, test_name)
        if match:
            try:
                file_size = int(match.group(1))
                if 0 < file_size < 10000:  # Reasonable file size range (MB)
                    return file_size
            except (ValueError, IndexError):
                continue

    return None

def extract_total_file_size(test_name):
    """Extract total file size from test name"""
    if not isinstance(test_name, str):
        return None

    test_name = test_name.lower()  # Convert to lowercase for case-insensitive matching

    # Expanded matching patterns
    patterns = [
        r'total[_\-]?size[_\-]?(\d+)[_\-]?mb',       # total_size_100_mb
        r'total[_\-]?size[_\-]?(\d+)',               # total_size_100
        r'总[大小]?[_\-]?(\d+)[_\-]?mb',               # 总大小100mb, 总_100_mb
        r'总文件[大小]?[_\-]?(\d+)[_\-]?mb',            # 总文件大小100mb
        r'max[_\-]?total[_\-]?size[_\-]?(\d+)',      # max_total_size_100
        r'total[_\-]?file[_\-]?size[_\-]?(\d+)',     # total_file_size_100
        r'日志总大小[_\-]?(\d+)[_\-]?mb',               # 日志总大小100mb
        r'ts[_\-]?(\d+)[_\-]?mb'                     # ts_100_mb
    ]

    for pattern in patterns:
        match = re.search(pattern, test_name)
        if match:
            try:
                total_size = int(match.group(1))
                if 0 < total_size < 100000:  # Reasonable total file size range (MB)
                    return total_size
            except (ValueError, IndexError):
                continue

    return None

def categorize_test(test_name):
    """Categorize test name"""
    if not isinstance(test_name, str):
        return 'unknown'

    test_name = test_name.lower()  # Convert to lowercase for case-insensitive matching

    # Check by priority order
    if re.search(r'throughput_threads|thread[s]?|线程|并发', test_name):
        return 'thread_scaling'
    elif re.search(r'throughput_msgsize|msg|message|small|medium|large|消息|小[型]?[消息]?|中[型]?[消息]?|大[型]?[消息]?', test_name):
        return 'message_size'
    elif re.search(r'throughput_queue|queue|队列|异步队列', test_name):
        return 'queue_size'
    elif re.search(r'throughput_workers|worker|工作线程|异步工作', test_name):
        return 'worker_threads'
    elif re.search(r'file[_\-]?size|文件大小|单文件', test_name):
        return 'file_size'
    elif re.search(r'total[_\-]?size|总大小|总文件', test_name):
        return 'total_size'
    elif re.search(r'stress|压力|突发', test_name):
        if re.search(r'small|小[型]?[突发]?', test_name):
            return 'stress_small'
        elif re.search(r'medium|中[型]?[突发]?', test_name):
            return 'stress_medium'
        elif re.search(r'large|大[型]?[突发]?', test_name):
            return 'stress_large'
        else:
            return 'stress_other'
    elif re.search(r'latency|延迟', test_name):
        return 'latency'
    elif re.search(r'compare|对比', test_name):
        return 'comparison'
    else:
        return 'other'

def standardize_columns(data):
    """Standardize column values"""
    # Create sort order for message size
    size_order = {'small': 0, 'medium': 1, 'large': 2}
    if 'MessageSizeType' in data.columns:
        data['SizeOrder'] = data['MessageSizeType'].map(size_order)

    # If necessary, rename columns to standard names
    column_mapping = {
        'Timestamp': 'Timestamp',
        'TestName': 'TestName',
        'TestType': 'TestType',
        'Logger': 'Logger',
        'Threads': 'Threads',  # This might come in as ThreadCount
        'Iterations': 'Iterations',
        'MessageSize': 'MessageSize',  # This might come in as MessageSizeType
        'QueueSize': 'QueueSize',  # This might come in as ExtractedQueueSize
        'WorkerThreads': 'WorkerThreads',  # This might come in as ExtractedWorkerThreads
        'EnableConsole': 'EnableConsole',
        'EnableFile': 'EnableFile',
        'TotalTime_ms': 'TotalTime_ms',
        'LogsPerSecond': 'LogsPerSecond',
        'Min_Latency_us': 'Min_Latency_us',
        'Median_Latency_us': 'Median_Latency_us',
        'P95_Latency_us': 'P95_Latency_us',
        'P99_Latency_us': 'P99_Latency_us',
        'Max_Latency_us': 'Max_Latency_us',
        'Memory_KB': 'Memory_KB'
    }

    # Only rename columns that exist
    rename_dict = {old: new for old, new in column_mapping.items() if old in data.columns and old != new}
    if rename_dict:
        data = data.rename(columns=rename_dict)

    return data
