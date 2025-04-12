#!/usr/bin/env python3
"""
MM-Logger性能测试实时监控工具

此脚本用于在运行性能测试时实时监控和可视化性能数据。
它监视性能测试生成的日志文件，并显示实时的吞吐量、CPU使用率和内存使用情况。

使用方法:
1. 在一个终端启动性能测试: ./run_performance_tests.sh
2. 在另一个终端启动监控工具: python perf_monitor.py
"""

import os
import sys
import time
import psutil
import curses
import re
import threading
import subprocess
from datetime import datetime

# 性能数据存储类
class PerfData:
    def __init__(self):
        self.throughput = 0          # 每秒日志数
        self.cpu_usage = 0.0         # CPU使用率
        self.memory_usage_mb = 0.0   # 内存使用量(MB)
        self.disk_write_kbps = 0.0   # 磁盘写入速度(KB/s)
        self.test_name = "Unknown"   # 当前测试名称
        self.elapsed_time = 0        # 已运行时间(秒)
        self.log_file_size_mb = 0.0  # 日志文件大小(MB)
        self.last_update = time.time()  # 上次更新时间

# 监控日志目录中的文件变化
def monitor_log_directory(perf_data, log_dir="./perf_logs", update_interval=1.0):
    disk_io_prev = psutil.disk_io_counters()
    prev_time = time.time()
    
    while True:
        try:
            # 更新磁盘IO统计信息
            current_time = time.time()
            elapsed = current_time - prev_time
            
            if elapsed >= 0.1:  # 防止除以零或很小的时间间隔
                disk_io_curr = psutil.disk_io_counters()
                write_bytes = disk_io_curr.write_bytes - disk_io_prev.write_bytes
                perf_data.disk_write_kbps = (write_bytes / 1024) / elapsed
                
                disk_io_prev = disk_io_curr
                prev_time = current_time
            
            # 查找最新修改的日志文件
            newest_file = None
            newest_time = 0
            total_log_size = 0
            
            if os.path.exists(log_dir):
                for filename in os.listdir(log_dir):
                    if filename.endswith(".INFO") or filename.endswith(".WARN") or filename.endswith(".ERROR"):
                        file_path = os.path.join(log_dir, filename)
                        if os.path.isfile(file_path):
                            mod_time = os.path.getmtime(file_path)
                            file_size = os.path.getsize(file_path)
                            total_log_size += file_size
                            
                            if mod_time > newest_time:
                                newest_time = mod_time
                                newest_file = file_path
            
            # 计算日志文件总大小
            perf_data.log_file_size_mb = total_log_size / (1024 * 1024)
            
            # 查找正在运行的性能测试进程
            for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
                if 'performance_test' in proc.info['name'] or \
                   (proc.info['cmdline'] and 'performance_test' in ' '.join(proc.info['cmdline'])):
                    perf_data.cpu_usage = proc.cpu_percent()
                    perf_data.memory_usage_mb = proc.memory_info().rss / (1024 * 1024)
                    
                    # 尝试从命令行参数中提取测试名称
                    if proc.info['cmdline']:
                        cmdline = ' '.join(proc.info['cmdline'])
                        test_match = re.search(r'--test=(\w+)', cmdline)
                        if test_match:
                            perf_data.test_name = test_match.group(1)
                    
                    # 计算程序运行时间
                    perf_data.elapsed_time = time.time() - proc.create_time()
                    break
            
            # 估算吞吐量
            # 这里使用一个简单的启发式方法：监控日志文件增长速度
            if newest_file:
                try:
                    # 获取文件大小
                    current_size = os.path.getsize(newest_file)
                    
                    # 简单估算：假设每个日志条目平均100字节
                    # 这只是一个粗略估计，实际吞吐量需要从性能测试程序中获取
                    size_delta = current_size - getattr(monitor_log_directory, 'prev_size', 0)
                    time_delta = time.time() - getattr(monitor_log_directory, 'prev_time', time.time() - 1)
                    
                    if time_delta > 0:
                        estimated_logs = size_delta / 100  # 假设每条日志平均100字节
                        perf_data.throughput = int(estimated_logs / time_delta)
                    
                    monitor_log_directory.prev_size = current_size
                    monitor_log_directory.prev_time = time.time()
                except:
                    pass
            
            # 更新时间
            perf_data.last_update = time.time()
            
            time.sleep(update_interval)
        except Exception as e:
            continue  # 忽略错误，继续监控

# 主界面绘制函数
def draw_interface(stdscr, perf_data):
    # 清除屏幕
    stdscr.clear()
    height, width = stdscr.getmaxyx()
    
    # 设置颜色
    curses.start_color()
    curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)  # 标题/正常值
    curses.init_pair(2, curses.COLOR_YELLOW, curses.COLOR_BLACK) # 警告值
    curses.init_pair(3, curses.COLOR_RED, curses.COLOR_BLACK)    # 危险值
    curses.init_pair(4, curses.COLOR_CYAN, curses.COLOR_BLACK)   # 信息值
    
    GREEN = curses.color_pair(1)
    YELLOW = curses.color_pair(2)
    RED = curses.color_pair(3)
    CYAN = curses.color_pair(4)
    
    # 标题
    title = " MM-Logger 性能测试监控 "
    stdscr.addstr(0, (width - len(title)) // 2, title, GREEN | curses.A_BOLD)
    
    # 当前时间
    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    stdscr.addstr(0, width - len(current_time) - 1, current_time, CYAN)
    
    # 分隔线
    stdscr.addstr(1, 0, "=" * (width - 1), GREEN)
    
    # 测试信息
    elapsed_time_str = f"{int(perf_data.elapsed_time // 60):02d}:{int(perf_data.elapsed_time % 60):02d}"
    stdscr.addstr(2, 2, f"当前测试: ", curses.A_BOLD)
    stdscr.addstr(2, 12, f"{perf_data.test_name}")
    stdscr.addstr(2, width - 20, f"运行时间: {elapsed_time_str}", CYAN)
    
    # 性能指标
    stdscr.addstr(4, 2, "性能指标", curses.A_BOLD | curses.A_UNDERLINE)
    
    # 日志吞吐量
    throughput_color = GREEN if perf_data.throughput < 50000 else (YELLOW if perf_data.throughput < 100000 else RED)
    stdscr.addstr(6, 4, "日志吞吐量:")
    stdscr.addstr(6, 16, f"{perf_data.throughput:,} 条/秒", throughput_color)
    
    # CPU使用率
    cpu_color = GREEN if perf_data.cpu_usage < 50 else (YELLOW if perf_data.cpu_usage < 80 else RED)
    stdscr.addstr(7, 4, "CPU 使用率:")
    stdscr.addstr(7, 16, f"{perf_data.cpu_usage:.1f}%", cpu_color)
    
    # 内存使用量
    mem_color = GREEN if perf_data.memory_usage_mb < 100 else (YELLOW if perf_data.memory_usage_mb < 500 else RED)
    stdscr.addstr(8, 4, "内存使用量:")
    stdscr.addstr(8, 16, f"{perf_data.memory_usage_mb:.1f} MB", mem_color)
    
    # 磁盘写入速度
    disk_color = GREEN if perf_data.disk_write_kbps < 5000 else (YELLOW if perf_data.disk_write_kbps < 20000 else RED)
    stdscr.addstr(9, 4, "磁盘写入速度:")
    stdscr.addstr(9, 19, f"{perf_data.disk_write_kbps:.1f} KB/s", disk_color)
    
    # 日志文件大小
    log_size_color = GREEN if perf_data.log_file_size_mb < 50 else (YELLOW if perf_data.log_file_size_mb < 200 else RED)
    stdscr.addstr(10, 4, "日志文件大小:")
    stdscr.addstr(10, 19, f"{perf_data.log_file_size_mb:.1f} MB", log_size_color)
    
    # 吞吐量可视化 - 简单条形图
    graph_width = width - 20
    throughput_bar_len = min(int(perf_data.throughput / 1000), graph_width)
    
    stdscr.addstr(12, 2, "吞吐量图示:", curses.A_BOLD)
    stdscr.addstr(13, 4, "0" + " " * (graph_width - 8) + f"{graph_width * 1000:,}")
    stdscr.addstr(14, 4, "[" + "=" * throughput_bar_len + " " * (graph_width - throughput_bar_len) + "]", GREEN)
    
    # 系统负载图示 - CPU和内存使用率条形图
    stdscr.addstr(16, 2, "系统负载:", curses.A_BOLD)
    
    # CPU使用率条形图
    cpu_bar_len = min(int(perf_data.cpu_usage / 100 * graph_width), graph_width)
    cpu_bar_color = GREEN if perf_data.cpu_usage < 50 else (YELLOW if perf_data.cpu_usage < 80 else RED)
    stdscr.addstr(17, 4, "CPU: ")
    stdscr.addstr(17, 9, "[" + "=" * cpu_bar_len + " " * (graph_width - cpu_bar_len) + "]", cpu_bar_color)
    
    # 内存使用率条形图 - 假设500MB是100%
    mem_percent = min(int(perf_data.memory_usage_mb / 500 * 100), 100)
    mem_bar_len = min(int(mem_percent / 100 * graph_width), graph_width)
    mem_bar_color = GREEN if mem_percent < 50 else (YELLOW if mem_percent < 80 else RED)
    stdscr.addstr(18, 4, "内存:")
    stdscr.addstr(18, 9, "[" + "=" * mem_bar_len + " " * (graph_width - mem_bar_len) + "]", mem_bar_color)
    
    # 状态信息
    stdscr.addstr(height - 2, 2, "上次更新: " + datetime.fromtimestamp(perf_data.last_update).strftime("%H:%M:%S"), CYAN)
    stdscr.addstr(height - 2, width - 30, "按 'q' 退出监控", YELLOW)
    
    # 更新屏幕
    stdscr.refresh()

# 主函数
def main(stdscr):
    # 隐藏光标
    curses.curs_set(0)
    
    # 设置非阻塞模式
    stdscr.nodelay(True)
    
    # 初始化性能数据
    perf_data = PerfData()
    
    # 启动监控线程
    monitor_thread = threading.Thread(target=monitor_log_directory, args=(perf_data,), daemon=True)
    monitor_thread.start()
    
    # 主循环
    try:
        while True:
            # 处理按键
            c = stdscr.getch()
            if c == ord('q'):
                break
            
            # 绘制界面
            draw_interface(stdscr, perf_data)
            
            # 刷新频率
            time.sleep(0.5)
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    curses.wrapper(main)
