# MM-Logger 性能分析工具

这个工具包用于分析MM-Logger性能测试结果并生成图表和报告。它处理由`performance_test`工具和
`run_performance_tests.sh`脚本生成的CSV格式测试结果数据。

## 功能特性

- 加载和预处理性能测试数据
- 生成多种性能分析图表
- 创建综合性能报告
- 提供性能优化建议

## 目录结构

```
perf_analyzer/
├── __init__.py         # 包初始化文件
├── data_loader.py      # 数据加载和预处理模块
├── visualization.py    # 图表生成模块
├── report.py           # 报告生成模块
└── utils.py            # 工具函数模块
```

## 使用方法

### 命令行使用

```bash
python analyze_perf_results.py <results_csv_file> [output_directory]
```

- `<results_csv_file>`: 性能测试结果CSV文件
- `[output_directory]`: 可选的输出目录，默认为"./perf_report"

### 示例

```bash
python analyze_perf_results.py mm_logger_perf_results.csv ./perf_report
```

## 生成的输出

### 图表

- 吞吐量比较图
- 延迟比较图
- 线程扩展性能图
- 消息大小影响图
- 队列大小影响图
- 工作线程数影响图
- 压力测试结果图
- 控制台输出影响图
- 文件大小影响图

### 报告

生成的报告包含以下内容：

1. **总体性能对比** - 比较MM-Logger和spdlog的吞吐量和延迟
2. **扩展性分析** - 线程扩展性和消息大小影响分析
3. **推荐配置** - 基于测试结果的最佳配置参数
4. **性能优化建议** - 改进性能的具体建议
5. **结论** - 总体性能评估

## 依赖项

- Python 3.6+
- pandas
- matplotlib
- numpy

## 安装依赖

```bash
pip install pandas matplotlib numpy
```

## 注意事项

- 确保提供的CSV文件包含所有必要的列
- 对于最佳结果，应使用`run_performance_tests.sh`脚本生成的完整测试集
- 输出质量取决于输入数据的完整性和一致性
