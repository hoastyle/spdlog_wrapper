#!/usr/bin/env python3
"""
性能分析工具主程序

此模块提供了命令行入口点和主要执行逻辑。
"""

import sys
import os
from datetime import datetime

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

def print_banner():
    """打印程序横幅"""
    banner = """
    ███╗   ███╗███╗   ███╗      ██╗      ██████╗  ██████╗  ██████╗ ███████╗██████╗ 
    ████╗ ████║████╗ ████║      ██║     ██╔═══██╗██╔════╝ ██╔════╝ ██╔════╝██╔══██╗
    ██╔████╔██║██╔████╔██║█████╗██║     ██║   ██║██║  ███╗██║  ███╗█████╗  ██████╔╝
    ██║╚██╔╝██║██║╚██╔╝██║╚════╝██║     ██║   ██║██║   ██║██║   ██║██╔══╝  ██╔══██╗
    ██║ ╚═╝ ██║██║ ╚═╝ ██║      ███████╗╚██████╔╝╚██████╔╝╚██████╔╝███████╗██║  ██║
    ╚═╝     ╚═╝╚═╝     ╚═╝      ╚══════╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚══════╝╚═╝  ╚═╝
                                                                                     
    ██████╗ ███████╗██████╗ ███████╗ ██████╗ ██████╗ ███╗   ███╗ █████╗ ███╗   ██╗ ██████╗███████╗
    ██╔══██╗██╔════╝██╔══██╗██╔════╝██╔═══██╗██╔══██╗████╗ ████║██╔══██╗████╗  ██║██╔════╝██╔════╝
    ██████╔╝█████╗  ██████╔╝█████╗  ██║   ██║██████╔╝██╔████╔██║███████║██╔██╗ ██║██║     █████╗  
    ██╔═══╝ ██╔══╝  ██╔══██╗██╔══╝  ██║   ██║██╔══██╗██║╚██╔╝██║██╔══██║██║╚██╗██║██║     ██╔══╝  
    ██║     ███████╗██║  ██║██║     ╚██████╔╝██║  ██║██║ ╚═╝ ██║██║  ██║██║ ╚████║╚██████╗███████╗
    ╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝      ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝╚══════╝
                                                                                                   
     █████╗ ███╗   ██╗ █████╗ ██╗  ██╗   ██╗███████╗███████╗██████╗ 
    ██╔══██╗████╗  ██║██╔══██╗██║  ╚██╗ ██╔╝╚══███╔╝██╔════╝██╔══██╗
    ███████║██╔██╗ ██║███████║██║   ╚████╔╝   ███╔╝ █████╗  ██████╔╝
    ██╔══██║██║╚██╗██║██╔══██║██║    ╚██╔╝   ███╔╝  ██╔══╝  ██╔══██╗
    ██║  ██║██║ ╚████║██║  ██║███████╗██║   ███████╗███████╗██║  ██║
    ╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝╚═╝   ╚══════╝╚══════╝╚═╝  ╚═╝
    """
    print(banner)
    print("\n性能测试结果分析工具 v1.0.0")
    print("==================================================")

def main():
    """主程序入口点"""
    print_banner()
    
    # 命令行参数解析
    if len(sys.argv) < 2:
        print("用法: analyze_perf_results <结果CSV文件> [输出目录]")
        return 1
    
    csv_file = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "./perf_report"
    
    # 配置绘图设置
    configure_plotting()
    
    # 加载测试结果
    print(f"\n正在加载性能测试数据: {csv_file}")
    try:
        data = load_results(csv_file)
    except Exception as e:
        print(f"错误: 加载数据失败 - {e}")
        return 1
    
    # 创建输出目录
    output_dir = create_output_dir(output_dir)
    print(f"将输出报告和图表到: {output_dir}")
    
    # 执行所有可用的图表生成
    print("\n生成性能分析图表...")
    plots_generated = 0
    
    try:
        # 执行所有图表生成函数
        plot_functions = [
            plot_throughput_comparison,
            plot_latency_comparison,
            plot_thread_scaling,
            plot_message_size_impact,
            plot_queue_size_impact,
            plot_worker_threads_impact,
            plot_stress_test_results,
            plot_console_impact,
            plot_file_size_impact
        ]
        
        for plot_function in plot_functions:
            try:
                plot_function(data, output_dir)
                plots_generated += 1
            except Exception as e:
                print(f"警告: 生成图表 {plot_function.__name__} 时出错 - {e}")
                continue
        
        print(f"成功生成 {plots_generated} 种类型的图表")
        
        # 创建汇总报告
        print("\n生成汇总报告...")
        create_summary_report(data, output_dir)
        
        print(f"\n分析完成! 报告和图表已保存至: {output_dir}")
        print(f"主报告: {os.path.join(output_dir, 'performance_summary_report.md')}")
        
        return 0
    except Exception as e:
        print(f"错误: 分析过程中发生意外错误 - {e}")
        return 1

if __name__ == "__main__":
    start_time = datetime.now()
    exit_code = main()
    end_time = datetime.now()
    duration = (end_time - start_time).total_seconds()
    print(f"\n分析耗时: {duration:.2f} 秒")
    sys.exit(exit_code)
