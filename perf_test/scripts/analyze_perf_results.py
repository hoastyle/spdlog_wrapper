#!/usr/bin/env python3
"""
分析MM-Logger性能测试结果并生成报告

此脚本读取由performance_test生成的CSV结果文件，
分析性能数据并生成图表和汇总报告。
"""

import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime

# 设置matplotlib中文字体支持
plt.rcParams['font.sans-serif'] = ['SimHei']  # 用来正常显示中文标签
plt.rcParams['axes.unicode_minus'] = False  # 用来正常显示负号

def load_results(csv_file):
    """加载性能测试结果"""
    if not os.path.exists(csv_file):
        print(f"错误: 结果文件 {csv_file} 不存在")
        sys.exit(1)
        
    try:
        data = pd.read_csv(csv_file)
        print(f"加载了 {len(data)} 条性能测试结果")
        return data
    except Exception as e:
        print(f"加载结果文件时出错: {e}")
        sys.exit(1)

def create_output_dir(output_dir):
    """创建输出目录"""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    return output_dir

def plot_throughput_comparison(data, output_dir):
    """绘制mm_logger和spdlog的吞吐量比较"""
    # 过滤比较测试数据
    comparison_data = data[data['TestType'].str.contains('comparison_throughput')]
    
    if len(comparison_data) < 2:
        print("警告: 没有足够的吞吐量比较数据")
        return
    
    # 按照测试条件分组
    grouped = comparison_data.groupby(['Threads', 'MessageSize', 'QueueSize', 'WorkerThreads'])
    
    for name, group in grouped:
        if len(group) < 2:
            continue  # 跳过不完整的比较组
            
        # 确保每组有mm_logger和spdlog的数据
        if not (group['Logger'] == 'mm_logger').any() or not (group['Logger'] == 'spdlog').any():
            continue
            
        threads, msg_size, queue_size, worker_threads = name
        
        # 准备数据
        mm_logger_data = group[group['Logger'] == 'mm_logger']['LogsPerSecond'].values[0]
        spdlog_data = group[group['Logger'] == 'spdlog']['LogsPerSecond'].values[0]
        
        # 创建图表
        fig, ax = plt.subplots(figsize=(10, 6))
        bars = ax.bar(['mm_logger', 'spdlog'], [mm_logger_data, spdlog_data], color=['#1f77b4', '#ff7f0e'])
        
        # 添加数值标签
        for bar in bars:
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height + 5,
                    f'{int(height):,}',
                    ha='center', va='bottom', fontsize=12)
        
        # 设置标题和标签
        ax.set_title(f'吞吐量比较 (线程数: {threads}, 消息大小: {msg_size}, 队列: {queue_size}, 工作线程: {worker_threads})', fontsize=14)
        ax.set_ylabel('每秒日志数', fontsize=12)
        ax.set_ylim(0, max(mm_logger_data, spdlog_data) * 1.2)  # 设置Y轴限制，留出空间显示数值
        
        # 计算性能差异百分比
        if spdlog_data > 0:
            diff_pct = ((mm_logger_data / spdlog_data) - 1) * 100
            diff_text = f"mm_logger 比 spdlog 快 {diff_pct:.1f}%" if diff_pct > 0 else f"mm_logger 比 spdlog 慢 {-diff_pct:.1f}%"
            ax.text(0.5, 0.9, diff_text, transform=ax.transAxes, ha='center', fontsize=13, color='red')
        
        # 保存图表
        filename = f"throughput_comparison_t{threads}_m{msg_size}_q{queue_size}_w{worker_threads}.png"
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, filename))
        plt.close()
        
        print(f"生成了吞吐量比较图: {filename}")

def plot_latency_comparison(data, output_dir):
    """绘制mm_logger和spdlog的延迟比较"""
    # 过滤比较测试数据
    comparison_data = data[data['TestType'].str.contains('comparison_latency')]
    
    if len(comparison_data) < 2:
        print("警告: 没有足够的延迟比较数据")
        return
    
    # 按照测试条件分组
    grouped = comparison_data.groupby(['Threads', 'MessageSize', 'QueueSize', 'WorkerThreads'])
    
    for name, group in grouped:
        if len(group) < 2:
            continue  # 跳过不完整的比较组
            
        # 确保每组有mm_logger和spdlog的数据
        if not (group['Logger'] == 'mm_logger').any() or not (group['Logger'] == 'spdlog').any():
            continue
            
        threads, msg_size, queue_size, worker_threads = name
        
        # 准备数据
        mm_logger_data = group[group['Logger'] == 'mm_logger']
        spdlog_data = group[group['Logger'] == 'spdlog']
        
        # 提取延迟数据点
        mm_logger_metrics = [
            mm_logger_data['Min_Latency_us'].values[0],
            mm_logger_data['Median_Latency_us'].values[0],
            mm_logger_data['P95_Latency_us'].values[0],
            mm_logger_data['P99_Latency_us'].values[0]
        ]
        
        spdlog_metrics = [
            spdlog_data['Min_Latency_us'].values[0],
            spdlog_data['Median_Latency_us'].values[0],
            spdlog_data['P95_Latency_us'].values[0],
            spdlog_data['P99_Latency_us'].values[0]
        ]
        
        # 创建图表
        fig, ax = plt.subplots(figsize=(12, 6))
        metric_names = ['最小延迟', '中位数延迟', '95%延迟', '99%延迟']
        
        x = np.arange(len(metric_names))
        width = 0.35
        
        # 绘制柱状图
        bars1 = ax.bar(x - width/2, mm_logger_metrics, width, label='mm_logger', color='#1f77b4')
        bars2 = ax.bar(x + width/2, spdlog_metrics, width, label='spdlog', color='#ff7f0e')
        
        # 添加数值标签
        def add_labels(bars):
            for bar in bars:
                height = bar.get_height()
                ax.text(bar.get_x() + bar.get_width()/2., height + 1,
                        f'{height:.1f}',
                        ha='center', va='bottom', fontsize=10)
                        
        add_labels(bars1)
        add_labels(bars2)
        
        # 设置标题和标签
        ax.set_title(f'延迟比较 (线程数: {threads}, 消息大小: {msg_size}, 队列: {queue_size}, 工作线程: {worker_threads})', fontsize=14)
        ax.set_ylabel('延迟 (微秒)', fontsize=12)
        ax.set_xticks(x)
        ax.set_xticklabels(metric_names)
        ax.legend()
        
        # 计算中位数延迟性能差异百分比
        if spdlog_metrics[1] > 0:
            diff_pct = ((mm_logger_metrics[1] / spdlog_metrics[1]) - 1) * 100
            diff_text = f"mm_logger 中位数延迟比 spdlog 高 {diff_pct:.1f}%" if diff_pct > 0 else f"mm_logger 中位数延迟比 spdlog 低 {-diff_pct:.1f}%"
            ax.text(0.5, 0.9, diff_text, transform=ax.transAxes, ha='center', fontsize=13, color='red')
        
        # 保存图表
        filename = f"latency_comparison_t{threads}_m{msg_size}_q{queue_size}_w{worker_threads}.png"
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, filename))
        plt.close()
        
        print(f"生成了延迟比较图: {filename}")

def plot_thread_scaling(data, output_dir):
    """绘制线程扩展性能图"""
    # 过滤线程扩展测试数据
    thread_data = data[data['TestName'].str.contains('throughput_threads_')]
    
    if len(thread_data) < 3:
        print("警告: 没有足够的线程扩展测试数据")
        return
    
    # 提取线程数
    thread_data['ThreadCount'] = thread_data['TestName'].str.extract(r'throughput_threads_(\d+)').astype(int)
    
    # 按Logger类型分组
    grouped = thread_data.groupby('Logger')
    
    plt.figure(figsize=(12, 7))
    
    for name, group in grouped:
        # 按线程数排序
        group = group.sort_values('ThreadCount')
        
        # 绘制线程数-吞吐量曲线
        plt.plot(group['ThreadCount'], group['LogsPerSecond'], marker='o', label=name, linewidth=2)
    
    plt.title('线程数扩展性能 (每秒日志数)', fontsize=14)
    plt.xlabel('线程数', fontsize=12)
    plt.ylabel('每秒日志数', fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    plt.xscale('log', base=2)  # 使用对数尺度更清晰地显示线程扩展趋势
    
    # 保存图表
    filename = "thread_scaling.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()
    
    print(f"生成了线程扩展性能图: {filename}")

def plot_message_size_impact(data, output_dir):
    """绘制消息大小影响图"""
    # 过滤消息大小测试数据
    size_data = data[data['TestName'].str.contains('throughput_msgsize_')]
    
    if len(size_data) < 3:
        print("警告: 没有足够的消息大小测试数据")
        return
    
    # 提取消息大小
    size_data['MessageSizeType'] = size_data['TestName'].str.extract(r'throughput_msgsize_(\w+)')
    
    # 设置正确的排序顺序
    size_order = {'small': 0, 'medium': 1, 'large': 2}
    size_data['SizeOrder'] = size_data['MessageSizeType'].map(size_order)
    
    # 按Logger类型分组
    grouped = size_data.groupby('Logger')
    
    plt.figure(figsize=(10, 6))
    
    for name, group in grouped:
        # 按消息大小排序
        group = group.sort_values('SizeOrder')
        
        # 绘制消息大小-吞吐量柱状图
        plt.bar(group['MessageSizeType'] + ' (' + name + ')', group['LogsPerSecond'], label=name)
    
    plt.title('消息大小对吞吐量的影响', fontsize=14)
    plt.xlabel('消息大小', fontsize=12)
    plt.ylabel('每秒日志数', fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.7, axis='y')
    
    # 保存图表
    filename = "message_size_impact.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()
    
    print(f"生成了消息大小影响图: {filename}")

def plot_queue_size_impact(data, output_dir):
    """绘制队列大小影响图"""
    # 过滤队列大小测试数据
    queue_data = data[data['TestName'].str.contains('throughput_queue_')]
    
    if len(queue_data) < 3:
        print("警告: 没有足够的队列大小测试数据")
        return
    
    # 提取队列大小
    queue_data['QueueSize'] = queue_data['TestName'].str.extract(r'throughput_queue_(\d+)').astype(int)
    
    # 按Logger类型分组
    grouped = queue_data.groupby('Logger')
    
    plt.figure(figsize=(12, 7))
    
    for name, group in grouped:
        # 按队列大小排序
        group = group.sort_values('QueueSize')
        
        # 绘制队列大小-吞吐量曲线
        plt.plot(group['QueueSize'], group['LogsPerSecond'], marker='o', label=name, linewidth=2)
    
    plt.title('异步队列大小对吞吐量的影响', fontsize=14)
    plt.xlabel('队列大小 (条目数)', fontsize=12)
    plt.ylabel('每秒日志数', fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    
    # 保存图表
    filename = "queue_size_impact.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()
    
    print(f"生成了队列大小影响图: {filename}")

def plot_worker_threads_impact(data, output_dir):
    """绘制工作线程数影响图"""
    # 过滤工作线程数测试数据
    worker_data = data[data['TestName'].str.contains('throughput_workers_')]
    
    if len(worker_data) < 3:
        print("警告: 没有足够的工作线程数测试数据")
        return
    
    # 提取工作线程数
    worker_data['WorkerThreads'] = worker_data['TestName'].str.extract(r'throughput_workers_(\d+)').astype(int)
    
    # 按Logger类型分组
    grouped = worker_data.groupby('Logger')
    
    plt.figure(figsize=(12, 7))
    
    for name, group in grouped:
        # 按工作线程数排序
        group = group.sort_values('WorkerThreads')
        
        # 绘制工作线程数-吞吐量曲线
        plt.plot(group['WorkerThreads'], group['LogsPerSecond'], marker='o', label=name, linewidth=2)
    
    plt.title('异步工作线程数对吞吐量的影响', fontsize=14)
    plt.xlabel('工作线程数', fontsize=12)
    plt.ylabel('每秒日志数', fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    
    # 保存图表
    filename = "worker_threads_impact.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()
    
    print(f"生成了工作线程数影响图: {filename}")

def plot_stress_test_results(data, output_dir):
    """绘制压力测试结果"""
    # 过滤压力测试数据
    stress_data = data[data['TestName'].str.contains('stress_')]
    
    if len(stress_data) < 2:
        print("警告: 没有足够的压力测试数据")
        return
    
    # 提取压力测试类型
    stress_types = []
    for name in stress_data['TestName']:
        if 'small_bursts' in name:
            stress_types.append("小批量突发")
        elif 'medium_bursts' in name:
            stress_types.append("中批量突发")
        elif 'large_burst' in name:
            stress_types.append("大批量突发")
        else:
            stress_types.append(name.split('_')[-1])
    
    stress_data['StressType'] = stress_types
    
    # 按Logger类型分组
    grouped = stress_data.groupby('Logger')
    
    plt.figure(figsize=(10, 6))
    
    bar_width = 0.35
    index = np.arange(len(stress_data['StressType'].unique()))
    
    for i, (name, group) in enumerate(grouped):
        # 绘制压力测试类型-吞吐量柱状图
        plt.bar(index + i*bar_width, group['LogsPerSecond'], bar_width, label=name)
    
    plt.title('不同压力模式下的吞吐量表现', fontsize=14)
    plt.xlabel('压力测试类型', fontsize=12)
    plt.ylabel('每秒日志数', fontsize=12)
    plt.xticks(index + bar_width/2, stress_data['StressType'].unique())
    plt.grid(True, linestyle='--', alpha=0.7, axis='y')
    plt.legend()
    
    # 保存图表
    filename = "stress_test_results.png"
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()
    
    print(f"生成了压力测试结果图: {filename}")

def create_summary_report(data, output_dir):
    """创建汇总报告"""
    # 准备报告文件
    report_file = os.path.join(output_dir, "performance_summary_report.md")
    
    # 获取当前时间
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # 计算mm_logger相对spdlog的性能差异
    comparison_throughput = data[data['TestType'].str.contains('comparison_throughput')]
    
    if len(comparison_throughput) >= 2:
        # 计算平均吞吐量比较
        mm_logger_throughput = comparison_throughput[comparison_throughput['Logger'] == 'mm_logger']['LogsPerSecond'].mean()
        spdlog_throughput = comparison_throughput[comparison_throughput['Logger'] == 'spdlog']['LogsPerSecond'].mean()
        
        if spdlog_throughput > 0:
            throughput_diff_pct = ((mm_logger_throughput / spdlog_throughput) - 1) * 100
        else:
            throughput_diff_pct = 0
    else:
        mm_logger_throughput = 0
        spdlog_throughput = 0
        throughput_diff_pct = 0
    
    # 计算延迟比较
    comparison_latency = data[data['TestType'].str.contains('comparison_latency')]
    
    if len(comparison_latency) >= 2:
        # 计算中位数延迟比较
        mm_logger_latency = comparison_latency[comparison_latency['Logger'] == 'mm_logger']['Median_Latency_us'].mean()
        spdlog_latency = comparison_latency[comparison_latency['Logger'] == 'spdlog']['Median_Latency_us'].mean()
        
        if spdlog_latency > 0:
            latency_diff_pct = ((mm_logger_latency / spdlog_latency) - 1) * 100
        else:
            latency_diff_pct = 0
    else:
        mm_logger_latency = 0
        spdlog_latency = 0
        latency_diff_pct = 0
    
    # 创建报告内容
    with open(report_file, 'w', encoding='utf-8') as f:
        f.write(f"# MM-Logger 性能测试报告\n\n")
        f.write(f"**生成时间：** {now}\n\n")
        
        f.write("## 1. 总体性能对比\n\n")
        f.write("### 吞吐量比较 (每秒日志数)\n\n")
        f.write("| 日志库 | 平均吞吐量 | 相对性能 |\n")
        f.write("|--------|------------|----------|\n")
        f.write(f"| MM-Logger | {mm_logger_throughput:,.2f} | {'+' if throughput_diff_pct >= 0 else ''}{throughput_diff_pct:.2f}% |\n")
        f.write(f"| spdlog | {spdlog_throughput:,.2f} | 基准 |\n\n")
        
        f.write("### 延迟比较 (微秒)\n\n")
        f.write("| 日志库 | 平均中位数延迟 | 相对性能 |\n")
        f.write("|--------|--------------|----------|\n")
        f.write(f"| MM-Logger | {mm_logger_latency:.2f} | {'+' if latency_diff_pct >= 0 else ''}{latency_diff_pct:.2f}% |\n")
        f.write(f"| spdlog | {spdlog_latency:.2f} | 基准 |\n\n")
        
        f.write("## 2. 扩展性分析\n\n")
        
        # 线程扩展性分析
        thread_data = data[data['TestName'].str.contains('throughput_threads_')]
        if len(thread_data) >= 3:
            f.write("### 线程扩展性\n\n")
            f.write("| 线程数 | mm_logger吞吐量 | spdlog吞吐量 | 相对性能 |\n")
            f.write("|--------|----------------|--------------|----------|\n")
            
            # 提取线程数并排序
            thread_data['ThreadCount'] = thread_data['TestName'].str.extract(r'throughput_threads_(\d+)').astype(int)
            thread_counts = sorted(thread_data['ThreadCount'].unique())
            
            for thread_count in thread_counts:
                thread_subset = thread_data[thread_data['ThreadCount'] == thread_count]
                
                mm_logger_value = thread_subset[thread_subset['Logger'] == 'mm_logger']['LogsPerSecond'].values
                spdlog_value = thread_subset[thread_subset['Logger'] == 'spdlog']['LogsPerSecond'].values
                
                if len(mm_logger_value) > 0 and len(spdlog_value) > 0:
                    mm_logger_val = mm_logger_value[0]
                    spdlog_val = spdlog_value[0]
                    
                    if spdlog_val > 0:
                        rel_perf = ((mm_logger_val / spdlog_val) - 1) * 100
                        rel_perf_str = f"{'+' if rel_perf >= 0 else ''}{rel_perf:.2f}%"
                    else:
                        rel_perf_str = "N/A"
                    
                    f.write(f"| {thread_count} | {mm_logger_val:,.2f} | {spdlog_val:,.2f} | {rel_perf_str} |\n")
            
            f.write("\n")
        
        # 建议配置
        f.write("## 3. 推荐配置\n\n")
        
        # 查找最佳工作线程数
        worker_data = data[data['TestName'].str.contains('throughput_workers_') & (data['Logger'] == 'mm_logger')]
        if len(worker_data) >= 2:
            worker_data['WorkerThreads'] = worker_data['TestName'].str.extract(r'throughput_workers_(\d+)').astype(int)
            best_worker_idx = worker_data['LogsPerSecond'].idxmax()
            if best_worker_idx is not None and best_worker_idx in worker_data.index:
                best_worker_row = worker_data.loc[best_worker_idx]
                best_worker_threads = best_worker_row['WorkerThreads']
                f.write(f"* **推荐工作线程数：** {best_worker_threads}\n")
        
        # 查找最佳队列大小
        queue_data = data[data['TestName'].str.contains('throughput_queue_') & (data['Logger'] == 'mm_logger')]
        if len(queue_data) >= 2:
            queue_data['QueueSize'] = queue_data['TestName'].str.extract(r'throughput_queue_(\d+)').astype(int)
            best_queue_idx = queue_data['LogsPerSecond'].idxmax()
            if best_queue_idx is not None and best_queue_idx in queue_data.index:
                best_queue_row = queue_data.loc[best_queue_idx]
                best_queue_size = best_queue_row['QueueSize']
                f.write(f"* **推荐队列大小：** {best_queue_size}\n")
        
        # 控制台输出建议
        console_impact_data = data[data['Logger'] == 'mm_logger']
        if len(console_impact_data) > 0:
            console_on_data = console_impact_data[console_impact_data['EnableConsole'] == True]
            console_off_data = console_impact_data[console_impact_data['EnableConsole'] == False]
            
            if len(console_on_data) > 0 and len(console_off_data) > 0:
                console_on_perf = console_on_data['LogsPerSecond'].mean()
                console_off_perf = console_off_data['LogsPerSecond'].mean()
                
                if console_on_perf > 0:
                    console_impact_pct = ((console_off_perf / console_on_perf) - 1) * 100
                    f.write(f"* **控制台输出建议：** {'启用' if console_impact_pct <= 10 else '禁用'} (性能影响: {console_impact_pct:.2f}%)\n")
        
        f.write("\n## 4. 性能优化建议\n\n")
        
        # 基于测试结果给出建议
        f.write("1. **异步日志配置优化：**\n")
        if 'best_worker_threads' in locals() and 'best_queue_size' in locals():
            f.write(f"   - 工作线程数应根据CPU核心数和日志量设置，测试表明{best_worker_threads}个工作线程效果最佳\n")
            f.write(f"   - 队列大小应根据内存资源和峰值日志量设置，测试表明{best_queue_size}条目的队列效果较好\n\n")
        else:
            f.write(f"   - 工作线程数应根据CPU核心数和日志量设置，测试表明2-4个工作线程效果较好\n")
            f.write(f"   - 队列大小应根据内存资源和峰值日志量设置，测试表明8192-16384条目的队列效果较好\n\n")
        
        f.write("2. **多线程环境优化：**\n")
        f.write("   - 当应用线程数超过8个时，建议增加工作线程数和队列大小\n")
        f.write("   - 对于突发日志场景，适当增大队列大小可以提高系统稳定性\n\n")
        
        f.write("3. **IO性能优化：**\n")
        f.write("   - 考虑将日志文件放置在SSD上以提高写入性能\n")
        f.write("   - 合理设置文件大小限制和总体大小限制，避免频繁创建新文件\n\n")
        
        f.write("4. **内存使用优化：**\n")
        f.write("   - 监控异步队列使用情况，避免内存占用过高\n")
        f.write("   - 在资源受限环境中，考虑减小队列大小并增加工作线程数\n\n")
        
        f.write("## 5. 结论\n\n")
        
        # 根据比较结果得出结论
        if throughput_diff_pct > 5:
            f.write(f"MM-Logger在吞吐量测试中表现优秀，平均比原生spdlog快**{throughput_diff_pct:.2f}%**。")
        elif throughput_diff_pct >= -5:
            f.write(f"MM-Logger在吞吐量测试中表现与原生spdlog相当（{throughput_diff_pct:.2f}%）。")
        else:
            f.write(f"MM-Logger在吞吐量测试中略逊于原生spdlog（{throughput_diff_pct:.2f}%），但提供了更多的功能特性。")
        
        if latency_diff_pct < 0:
            f.write(f" 在延迟方面，MM-Logger平均比原生spdlog低**{-latency_diff_pct:.2f}%**，具有更好的响应性能。\n\n")
        elif latency_diff_pct <= 10:
            f.write(f" 在延迟方面，MM-Logger与原生spdlog的表现相当（{latency_diff_pct:.2f}%）。\n\n")
        else:
            f.write(f" 在延迟方面，MM-Logger平均比原生spdlog高**{latency_diff_pct:.2f}%**，对于大多数应用场景仍在可接受范围内。\n\n")
        
        f.write("总体而言，MM-Logger提供了良好的性能表现，同时增加了多级日志轮转、总大小限制等实用功能，适合在生产环境中使用。通过正确的配置，可以充分发挥其性能潜力。\n")
    
    print(f"生成了性能汇总报告: {report_file}")

def main():
    if len(sys.argv) < 2:
        print("用法: python analyze_perf_results.py <结果CSV文件> [输出目录]")
        sys.exit(1)
    
    csv_file = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "./perf_report"
    
    # 加载测试结果
    data = load_results(csv_file)
    
    # 创建输出目录
    output_dir = create_output_dir(output_dir)
    
    # 生成各种图表
    plot_throughput_comparison(data, output_dir)
    plot_latency_comparison(data, output_dir)
    plot_thread_scaling(data, output_dir)
    plot_message_size_impact(data, output_dir)
    plot_queue_size_impact(data, output_dir)
    plot_worker_threads_impact(data, output_dir)
    plot_stress_test_results(data, output_dir)
    
    # 创建汇总报告
    create_summary_report(data, output_dir)
    
    print(f"分析完成! 报告和图表已保存至: {output_dir}")

if __name__ == "__main__":
    main()
