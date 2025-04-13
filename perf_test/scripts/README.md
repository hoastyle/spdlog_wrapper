# MM-Logger 性能分析工具

一个用于分析MM-Logger性能测试结果的工具包，支持数据可视化和报告生成。

## 概述

MM-Logger性能分析工具从性能测试结果CSV文件中提取数据，生成丰富的图表和详细的性能报告。该工具设计用于分析由`performance_test`和`run_performance_tests.sh`产生的测试结果。

## 功能特性

- **数据分析**：加载、清洗和准备性能测试数据
- **可视化**：生成多种类型的图表展示不同维度的性能指标
- **报告生成**：创建包含性能分析结果和优化建议的详细报告
- **配置推荐**：基于测试结果提供最佳配置参数

## 快速开始

### 安装

```bash
# 克隆仓库
git clone https://github.com/yourusername/mm_logger_perf_analyzer.git
cd mm_logger_perf_analyzer

# 安装依赖
pip install -r requirements.txt

# 安装工具包（可选）
pip install -e .
```

### 使用方法

```bash
python analyze_perf_results.py mm_logger_perf_results.csv ./perf_report
```

或者安装后使用命令行工具：

```bash
analyze_perf_results mm_logger_perf_results.csv ./perf_report
```

### 输出示例

运行后，工具将在指定的输出目录中生成以下内容：

- **图表**：各种性能指标的可视化展示
- **报告**：名为`performance_summary_report.md`的综合性能报告

## 详细文档

工具生成的图表包括：

1. **吞吐量比较**：比较MM-Logger和spdlog的日志吞吐量
2. **延迟比较**：比较不同日志库的延迟特性
3. **线程扩展性**：展示日志库在不同线程数下的性能扩展
4. **消息大小影响**：分析消息大小对性能的影响
5. **队列大小影响**：评估异步队列大小对吞吐量的影响
6. **工作线程数影响**：展示工作线程数量与性能的关系
7. **压力测试结果**：在不同负载模式下的性能表现
8. **控制台输出影响**：量化控制台输出对性能的影响
9. **文件大小影响**：分析文件大小设置对性能的影响

生成的报告`performance_summary_report.md`包含以下内容：

1. **总体性能对比**：MM-Logger与spdlog的吞吐量和延迟比较
2. **扩展性分析**：线程扩展性能和不同消息大小的影响
3. **推荐配置**：基于测试结果的最佳配置参数
4. **性能优化建议**：改进性能的具体建议
5. **结论**：综合评估和建议

在 perf_test 目录中，三种主要的性能测试类型——throughput、latency 和 stress 测试着眼于日志系统性能的不同方面。让我为您详细说明这三种测试的目的和测量内容：
1. Throughput（吞吐量）测试
测量内容：系统在单位时间内能处理的日志量（日志条目/秒）。
具体实现：

创建多个工作线程并行记录日志
使用线程屏障(ThreadBarrier)确保所有线程同时开始
记录完成预设数量日志条目所需的总时间
计算吞吐量 = 总日志条数 ÷ 总耗时（秒）

目的：评估日志系统的最大处理能力，适用于高负载场景下的性能评估。
在代码中，run_throughput_test() 函数实现了这一测试。
2. Latency（延迟）测试
测量内容：单个日志操作的响应时间（微秒）。
具体实现：

对每个日志操作单独计时
测量从调用日志函数到函数返回的时间
收集所有操作的延迟数据
计算关键指标：最小延迟、中位数延迟、95%分位延迟、99%分位延迟、最大延迟

目的：评估日志系统的响应速度，对实时系统或要求低延迟的应用尤为重要。
run_latency_test() 函数实现了这一测试逻辑。
3. Stress（压力）测试
测量内容：系统在突发负载下的稳定性和性能表现。
具体实现：

模拟突发式日志记录模式
使用两个关键参数：

burst_size：每次突发的日志数量
burst_count：突发次数


每个线程快速生成一批日志，然后短暂暂停，再生成下一批

目的：评估日志系统处理非均匀负载的能力，特别是测试异步队列和缓冲区在高峰负载下的表现。
这种测试通过 run_stress_test() 函数实现。
性能测试的配置参数
所有测试都支持多种配置参数，可以评估不同因素对性能的影响：

线程数：模拟多线程应用场景
消息大小：小型/中型/大型日志消息
队列大小：异步日志队列容量
工作线程数：处理异步日志的线程数
控制台输出：启用/禁用控制台输出对性能的影响
文件大小限制：单个文件和总大小限制对性能的影响

这些测试还可以比较 mm_logger 与原生 spdlog 的性能差异，为您选择最适合应用场景的日志系统提供数据支持。

## 项目结构

```
mm_logger_perf_analyzer/
├── analyze_perf_results.py  # 主入口点脚本
├── setup.py                 # 安装脚本
├── requirements.txt         # 依赖项列表
└── perf_analyzer/          # 主要代码包
    ├── __init__.py         # 包初始化
    ├── main.py             # 主程序逻辑
    ├── data_loader.py      # 数据加载和预处理
    ├── visualization.py    # 图表生成
    ├── report.py           # 报告生成
    └── utils.py            # 通用工具函数
```

## 依赖项

- Python 3.6+
- pandas
- matplotlib
- numpy

## 贡献指南

欢迎贡献！如果您想为项目做出贡献，请遵循以下步骤：

1. Fork 仓库
2. 创建您的功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交您的更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 打开一个 Pull Request

## 常见问题

**Q: 支持哪些输入文件格式？**
A: 目前支持CSV格式的性能测试结果文件。

**Q: 如何定制生成的图表？**
A: 您可以修改`visualization.py`中的相应函数来定制图表样式和内容。

**Q: 测试结果CSV文件必须包含哪些列？**
A: 基本需要包含TestName、TestType、Logger、LogsPerSecond等列。更多细节请参考`data_loader.py`。

## 更新日志

### 1.0.0 (2025-04-13)
- 初始版本发布
- 支持基本的数据分析、可视化和报告生成

## 许可证

MIT License - 详情请查看 LICENSE 文件

## 联系方式

如有问题或建议，请通过GitHub问题(Issues)功能提交或发送邮件至 example@example.com

---

**注意**: 此工具是为MM-Logger性能测试而设计的。在使用前，请确保您已正确运行了性能测试并生成了CSV格式的测试结果。
