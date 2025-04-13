#!/usr/bin/env python3
"""
Analyze MM-Logger performance test results and generate reports

This script processes CSV result files from performance_test,
analyzes performance data, and generates visualizations and reports.

Usage: python analyze_perf_results.py <results_csv_file> [output_directory]
"""

import sys
import os
import traceback
from datetime import datetime

# 添加当前目录到模块搜索路径以便找到 perf_analyzer 包
current_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, current_dir)

# Try to import required packages
try:
    import pandas as pd
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError as e:
    print(f"Error: Missing required package - {e}")
    print("Please install required packages with: pip install pandas matplotlib numpy")
    sys.exit(1)

# Import analyzer modules
try:
    from perf_analyzer.data_loader import load_results
    from perf_analyzer.visualization import (
        plot_throughput_comparison,
        plot_latency_comparison,
        plot_thread_scaling,
        plot_message_size_impact,
        plot_queue_size_impact,
        plot_worker_threads_impact,
        plot_stress_test_results,
        plot_console_impact,
        plot_file_size_impact
    )
    from perf_analyzer.report import create_summary_report
    from perf_analyzer.utils import create_output_dir, configure_plotting
except ImportError as e:
    # If perf_analyzer module is not available, try to import from current directory
    print(f"Warning: Could not import perf_analyzer module - {e}")
    print("Attempting to import from current directory...")

    # Use direct imports as fallback
    sys.path.append(os.path.join(os.path.dirname(__file__), "perf_analyzer"))
    try:
        from data_loader import load_results
        from visualization import (
            plot_throughput_comparison,
            plot_latency_comparison,
            plot_thread_scaling,
            plot_message_size_impact,
            plot_queue_size_impact,
            plot_worker_threads_impact,
            plot_stress_test_results,
            plot_console_impact,
            plot_file_size_impact
        )
        from report import create_summary_report
        from utils import create_output_dir, configure_plotting
    except ImportError as e:
        print(f"Error: Failed to import required modules - {e}")
        print("Please make sure you're running from the correct directory")
        sys.exit(1)

def print_banner():
    """Print program banner"""
    banner = """
    ╔═══════════════════════════════════════════════════╗
    ║                                                   ║
    ║   MM-Logger Performance Analysis Tool             ║
    ║                                                   ║
    ║   Analyzes performance test results               ║
    ║   Generates visualizations and reports            ║
    ║                                                   ║
    ╚═══════════════════════════════════════════════════╝
    """
    print(banner)

def main():
    """Main entry point"""
    print_banner()

    # Parse command line arguments
    if len(sys.argv) < 2:
        print("Usage: python analyze_perf_results.py <results_csv_file> [output_directory]")
        return 1

    csv_file = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "./perf_report"

    # Check if CSV file exists
    if not os.path.exists(csv_file):
        print(f"Error: Results file '{csv_file}' not found")
        return 1

    print(f"Analysis started at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    start_time = datetime.now()

    try:
        # Configure plotting settings
        print("Configuring plot settings...")
        configure_plotting()

        # Load performance test results
        print(f"Loading data from: {csv_file}")
        data = load_results(csv_file)

        if data is None or len(data) == 0:
            print("Error: No valid data loaded from CSV file")
            return 1

        print(f"Loaded {len(data)} test results")
        print(f"Data columns: {', '.join(data.columns)}")

        # Create output directory
        print(f"Creating output directory: {output_dir}")
        output_dir = create_output_dir(output_dir)

        # Generate visualizations
        print("\nGenerating performance analysis charts...")
        visualization_functions = [
            ("Throughput comparison", plot_throughput_comparison),
            ("Latency comparison", plot_latency_comparison),
            ("Thread scaling", plot_thread_scaling),
            ("Message size impact", plot_message_size_impact),
            ("Queue size impact", plot_queue_size_impact),
            ("Worker threads impact", plot_worker_threads_impact),
            ("Stress test results", plot_stress_test_results),
            ("Console output impact", plot_console_impact),
            ("File size impact", plot_file_size_impact)
        ]

        charts_generated = 0
        failed_charts = []

        for name, func in visualization_functions:
            try:
                print(f"Generating {name} chart...")
                func(data, output_dir)
                charts_generated += 1
            except Exception as e:
                print(f"Warning: Failed to generate {name} chart - {e}")
                failed_charts.append(name)
                # Print stack trace for debugging
                if "--debug" in sys.argv:
                    traceback.print_exc()

        # Generate summary report
        print("\nGenerating performance summary report...")
        try:
            create_summary_report(data, output_dir)
            print("Report generation successful")
        except Exception as e:
            print(f"Error generating summary report: {e}")
            if "--debug" in sys.argv:
                traceback.print_exc()

        # Print summary
        end_time = datetime.now()
        duration = (end_time - start_time).total_seconds()

        print("\n" + "="*60)
        print("Analysis Summary:")
        print(f"- Data records processed: {len(data)}")
        print(f"- Charts generated: {charts_generated} of {len(visualization_functions)}")
        if failed_charts:
            print(f"- Failed charts: {', '.join(failed_charts)}")
        print(f"- Report file: {os.path.join(output_dir, 'performance_summary_report.md')}")
        print(f"- Time taken: {duration:.2f} seconds")
        print("="*60)

        if charts_generated > 0:
            print(f"\nAnalysis completed successfully. Results saved to: {output_dir}")
            return 0
        else:
            print("\nWarning: No charts were generated. Please check your data format.")
            return 1

    except Exception as e:
        print(f"Error during analysis: {e}")
        if "--debug" in sys.argv:
            traceback.print_exc()
        return 1

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)
