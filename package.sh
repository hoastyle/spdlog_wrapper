#!/bin/bash
# 打包mm_logger库的脚本

set -e  # 遇到错误时退出

VERSION="1.0.0"
OUTPUT_DIR="./dist"
BUILD_DIR="./build_package"
INSTALL_DIR="${BUILD_DIR}/install"
PACKAGE_NAME="mm_logger-${VERSION}"

# 清理旧的构建目录
rm -rf "${BUILD_DIR}" "${OUTPUT_DIR}"
mkdir -p "${BUILD_DIR}" "${OUTPUT_DIR}" "${INSTALL_DIR}"

echo "===== 配置项目 ====="
cd "${BUILD_DIR}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DMM_LOGGER_BUILD_TESTS=OFF \
    -DMM_LOGGER_BUILD_EXAMPLES=OFF

echo "===== 构建项目 ====="
cmake --build . --config Release

echo "===== 安装到临时目录 ====="
cmake --install .

echo "===== 创建源代码包 ====="
cd ..
git archive --format=tar.gz --prefix="${PACKAGE_NAME}/" -o "${OUTPUT_DIR}/${PACKAGE_NAME}-src.tar.gz" HEAD

echo "===== 创建二进制包 ====="
cd "${INSTALL_DIR}"
tar -czf "../../${OUTPUT_DIR}/${PACKAGE_NAME}-linux.tar.gz" .

echo "===== 完成 ====="
echo "源代码包: ${OUTPUT_DIR}/${PACKAGE_NAME}-src.tar.gz"
echo "二进制包: ${OUTPUT_DIR}/${PACKAGE_NAME}-linux.tar.gz"
