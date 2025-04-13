"""
MM-Logger性能分析工具包

此软件包提供了用于分析MM-Logger性能测试结果的工具，
包括数据加载、可视化和报告生成功能。
"""

__version__ = '1.0.0'
__author__ = 'MM-Logger团队'

from .data_loader import load_results
from .visualization import (
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
from .report import create_summary_report
from .utils import create_output_dir, configure_plotting
