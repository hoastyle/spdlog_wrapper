#!/bin/bash
# MM-Logger性能分析工具安装脚本

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}开始安装MM-Logger性能分析工具...${NC}"

# 检查Python版本
python_version=$(python3 --version 2>&1 | awk '{print $2}')
echo -e "检测到Python版本: ${python_version}"

# 检查Python版本是否符合要求
python3 -c "import sys; sys.exit(0) if sys.version_info >= (3,6) else sys.exit(1)"
if [ $? -ne 0 ]; then
    echo -e "${RED}错误: 需要Python 3.6或更高版本${NC}"
    exit 1
fi

# 创建虚拟环境（可选）
echo -e "${YELLOW}是否要创建虚拟环境? (y/n)${NC}"
read create_venv
if [[ "$create_venv" == "y" || "$create_venv" == "Y" ]]; then
    echo -e "${BLUE}创建虚拟环境...${NC}"
    python3 -m venv venv
    
    # 激活虚拟环境
    source venv/bin/activate
    echo -e "${GREEN}虚拟环境已激活${NC}"
fi

# 安装依赖项
echo -e "${BLUE}安装依赖项...${NC}"
pip install -r requirements.txt

if [ $? -ne 0 ]; then
    echo -e "${RED}安装依赖项失败${NC}"
    exit 1
fi

# 安装工具包
echo -e "${BLUE}安装MM-Logger性能分析工具...${NC}"
pip install -e .

if [ $? -ne 0 ]; then
    echo -e "${RED}安装工具包失败${NC}"
    exit 1
fi

echo -e "${GREEN}安装成功!${NC}"
echo -e "现在您可以使用以下命令分析性能测试结果:"
echo -e "${YELLOW}python analyze_perf_results.py <结果CSV文件> [输出目录]${NC}"
echo -e "或"
echo -e "${YELLOW}analyze_perf_results <结果CSV文件> [输出目录]${NC}"

if [[ "$create_venv" == "y" || "$create_venv" == "Y" ]]; then
    echo -e "\n使用前请确保已激活虚拟环境: ${BLUE}source venv/bin/activate${NC}"
fi

echo -e "\n${GREEN}感谢使用MM-Logger性能分析工具!${NC}"
