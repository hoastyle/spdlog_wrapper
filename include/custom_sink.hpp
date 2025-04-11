#pragma once

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/synchronous_factory.h>

#include <mutex>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <atomic>
#include <thread>

namespace mm_log {

// 获取当前时间戳字符串（YYYYMMDD_HHMMSS格式）
inline std::string get_timestamp_str() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y%m%d_%H%M%S");
    return ss.str();
}

// 自定义轮转sink，支持基于总大小限制和使用时间戳命名文件
template<typename Mutex>
class custom_rotating_file_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
    custom_rotating_file_sink(
        const std::string& base_filename,
        const std::string& log_type,       // INFO, WARN, ERROR
        size_t max_size,                   // 单个文件最大大小
        size_t max_total_size)             // 所有文件总大小上限
        : base_filename_(base_filename),
          log_type_(log_type),
          max_size_(max_size),
          max_total_size_(max_total_size),
          current_size_(0),
          last_rotation_check_(std::chrono::steady_clock::now()),
          rotation_check_interval_(std::chrono::seconds(1)) // 每秒检查一次轮转
    {
        // 确保目录存在
        std::filesystem::path path(base_filename_);
        std::filesystem::path dir = path.parent_path();
        if (!dir.empty() && !std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }

        auto current_filename = create_new_file();
        file_helper_.open(current_filename, false);
        current_size_ = file_helper_.size();

        // 创建或更新软链接
        update_symlink(current_filename);

        // 清理旧文件，确保总大小不超过限制
        cleanup_old_files();
    }

    ~custom_rotating_file_sink()
    {
        this->flush();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        size_t msg_size = formatted.size();

        // 使用预测大小来减少实际大小检查的频率
        if (current_size_ + msg_size > max_size_ ||
            should_check_rotation()) {

            std::lock_guard<Mutex> lock(rotation_mutex_);

            // 在锁内再次检查，以确保其他线程没有已经执行轮转
            if (file_helper_.size() + msg_size > max_size_) {
                // 执行轮转
                file_helper_.close();
                auto new_filename = create_new_file();
                file_helper_.open(new_filename, false);
                current_size_ = 0;

                // 更新软链接
                update_symlink(new_filename);

                // 清理旧文件
                cleanup_old_files();
            }
            else {
                // 更新当前大小
                current_size_ = file_helper_.size();
            }

            // 更新最后检查时间
            last_rotation_check_ = std::chrono::steady_clock::now();
        }

        // 写入日志
        file_helper_.write(formatted);
        current_size_ += msg_size;
    }

    void flush_() override
    {
        file_helper_.flush();
    }

private:
    // 检查是否应该进行轮转检查
    bool should_check_rotation() {
        auto now = std::chrono::steady_clock::now();
        auto duration = now - last_rotation_check_;
        return duration >= rotation_check_interval_;
    }

    // 创建新的日志文件
    std::string create_new_file()
    {
        std::string timestamp = get_timestamp_str();
        std::string filename = build_filename(timestamp);
        return filename;
    }

    // 构建文件名
    std::string build_filename(const std::string& timestamp)
    {
        // 获取基础目录和文件名部分
        std::filesystem::path path(base_filename_);
        std::filesystem::path dir = path.parent_path();
        std::string basename = path.filename().string();

        // 避免空目录路径的情况
        if (dir.empty()) {
            dir = ".";
        }

        // 构建新的文件名：目录/类型.时间戳.文件名
        // 例如：logs/INFO.20250410_123045.app_log
        return (dir / (log_type_ + "." + timestamp + "." + basename)).string();
    }

    // 更新符号链接
    void update_symlink(const std::string& target_file)
    {
        // 构建符号链接名称，确保链接在与目标文件相同的目录中
        std::filesystem::path target_path(target_file);
        std::filesystem::path dir_path = target_path.parent_path();
        std::string basename = std::filesystem::path(base_filename_).filename().string();
        std::filesystem::path symlink_path = dir_path / (basename + "." + log_type_);
        std::string symlink_name = symlink_path.string();

        try {
            // 先删除现有的符号链接（如果存在）
            if (std::filesystem::exists(symlink_name)) {
                std::filesystem::remove(symlink_name);
            }

            // 获取目标文件的文件名部分（不含路径）
            std::string target_filename = target_path.filename().string();

            // 创建软链接，使用文件名而非完整路径
            std::filesystem::create_symlink(target_filename, symlink_name);
        }
        catch (const std::exception& e) {
            // 如果符号链接创建失败，尝试使用系统调用
            std::string target_filename = target_path.filename().string();
            std::string cmd = "ln -sf " + target_filename + " " + symlink_name;
            system(cmd.c_str());
        }
    }

    // 清理旧文件，确保总大小不超过上限
    void cleanup_old_files()
    {
        static std::mutex cleanup_mutex;
        std::lock_guard<std::mutex> lock(cleanup_mutex); // 防止多线程同时清理

        // 获取日志目录
        std::filesystem::path path(base_filename_);
        std::filesystem::path dir_path = path.parent_path();
        if (dir_path.empty()) {
            dir_path = "."; // 如果没有指定路径，则使用当前目录
        }

        // 获取日志文件前缀和基本文件名
        std::string file_prefix = log_type_ + ".";
        std::string base_name = path.filename().string();

        // 收集所有匹配的日志文件
        std::vector<std::filesystem::directory_entry> log_files;

        try {
            for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
                if (!entry.is_regular_file()) continue;

                std::string filename = entry.path().filename().string();
                // 检查文件名是否匹配我们的模式：type.timestamp.base_filename
                if (filename.find(file_prefix) == 0 && filename.find(base_name) != std::string::npos) {
                    log_files.push_back(entry);
                }
            }
        }
        catch (const std::exception& e) {
            // 如果目录迭代失败，返回，不进行清理
            return;
        }

        // 如果没有找到文件或只有一个文件，返回
        if (log_files.size() <= 1) {
            return;
        }

        // 按修改时间排序（最老的文件在前面）
        std::sort(log_files.begin(), log_files.end(),
            [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                return a.last_write_time() < b.last_write_time();
            });

        // 计算总大小
        size_t total_size = 0;
        for (const auto& file : log_files) {
            total_size += file.file_size();
        }

        // 从最老的文件开始删除，直到总大小低于限制
        // 但始终保留最新的文件
        for (auto it = log_files.begin(); it != log_files.end() - 1 && total_size > max_total_size_; ++it) {
            size_t file_size = it->file_size();

            try {
                // 检查文件是否不是当前正在写入的文件
                std::string curr_path = file_helper_.filename();
                if (it->path().string() != curr_path) {
                    std::filesystem::remove(it->path());
                    total_size -= file_size;
                }
            }
            catch (const std::exception& e) {
                // 如果删除失败，继续尝试其他文件
                continue;
            }
        }
    }

    spdlog::details::file_helper file_helper_;
    std::string base_filename_;
    std::string log_type_;
    size_t max_size_;
    size_t max_total_size_;
    std::atomic<size_t> current_size_; // 原子变量，跟踪当前文件大小
    std::chrono::steady_clock::time_point last_rotation_check_; // 最后检查轮转的时间
    std::chrono::steady_clock::duration rotation_check_interval_; // 轮转检查间隔
    Mutex rotation_mutex_; // 用于保护文件轮转操作的互斥锁
};

using custom_rotating_file_sink_mt = custom_rotating_file_sink<std::mutex>;
using custom_rotating_file_sink_st = custom_rotating_file_sink<spdlog::details::null_mutex>;

} // namespace mm_log
