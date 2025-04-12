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

// Get current timestamp string (YYYYMMDD_HHMMSS format)
inline std::string get_timestamp_str() {
  auto now        = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t_now), "%Y%m%d_%H%M%S");
  return ss.str();
}

// Custom rotating sink with total size limit and timestamp-based naming
template <typename Mutex>
class custom_rotating_file_sink : public spdlog::sinks::base_sink<Mutex> {
 public:
  custom_rotating_file_sink(const std::string& base_filename,
      const std::string& log_type,  // INFO, WARN, ERROR
      uint64_t max_size_mb,         // Single file max size
      uint64_t max_total_size_mb)   // All files total size limit
      : base_filename_(base_filename),
        log_type_(log_type),
        max_size_(max_size_mb * 1024ULL * 1024ULL),
        max_total_size_(max_total_size_mb * 1024ULL * 1024ULL),
        current_size_(0),
        last_rotation_check_(std::chrono::steady_clock::now()),
        rotation_check_interval_(std::chrono::seconds(1))  // Check every second
  {
    // Ensure directory exists
    std::filesystem::path path(base_filename_);
    std::filesystem::path dir = path.parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
      std::filesystem::create_directories(dir);
    }

    auto current_filename = create_new_file();
    file_helper_.open(current_filename, false);
    current_size_ = file_helper_.size();

    // Create or update symlink
    update_symlink(current_filename);

    // Clean up old files to ensure total size doesn't exceed limit
    cleanup_old_files();
  }

  ~custom_rotating_file_sink() { this->flush(); }

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    spdlog::memory_buf_t formatted;
    this->formatter_->format(msg, formatted);

    uint64_t msg_size = formatted.size();

    // Use predicted size to reduce frequency of actual size checks
    if (current_size_ + msg_size > max_size_ || should_check_rotation()) {
      std::lock_guard<Mutex> lock(rotation_mutex_);

      // Check again with lock to ensure other threads haven't already rotated
      if (file_helper_.size() + msg_size > max_size_) {
        // Perform rotation
        file_helper_.close();
        auto new_filename = create_new_file();
        file_helper_.open(new_filename, false);
        current_size_ = 0;

        // Update symlink
        update_symlink(new_filename);

        // Clean up old files
        cleanup_old_files();
      } else {
        // Update current size
        current_size_ = file_helper_.size();
      }

      // Update last check time
      last_rotation_check_ = std::chrono::steady_clock::now();
    }

    // Write log
    file_helper_.write(formatted);
    current_size_ += msg_size;
  }

  void flush_() override { file_helper_.flush(); }

 private:
  // Check if rotation should be checked
  bool should_check_rotation() {
    auto now      = std::chrono::steady_clock::now();
    auto duration = now - last_rotation_check_;
    return duration >= rotation_check_interval_;
  }

  // Create new log file
  std::string create_new_file() {
    std::string timestamp = get_timestamp_str();
    std::string filename  = build_filename(timestamp);
    return filename;
  }

  // Build filename
  std::string build_filename(const std::string& timestamp) {
    // Get base directory and filename parts
    std::filesystem::path path(base_filename_);
    std::filesystem::path dir = path.parent_path();
    std::string basename      = path.filename().string();

    // Handle empty directory path
    if (dir.empty()) {
      dir = ".";
    }

    // Build new filename: dir/type.timestamp.basename
    // Example: logs/INFO.20250412_143045.app_log
    return (dir / (log_type_ + "." + timestamp + "." + basename)).string();
  }

  // Update symlink - FIXED to handle system call warning
  void update_symlink(const std::string& target_file) {
    // Build symlink name in the same directory as target file
    std::filesystem::path target_path(target_file);
    std::filesystem::path dir_path = target_path.parent_path();
    std::string basename =
        std::filesystem::path(base_filename_).filename().string();
    std::filesystem::path symlink_path =
        dir_path / (basename + "." + log_type_);
    std::string symlink_name = symlink_path.string();

    try {
      // Remove existing symlink if it exists
      if (std::filesystem::exists(symlink_name)) {
        std::filesystem::remove(symlink_name);
      }

      // Get target filename (without path)
      std::string target_filename = target_path.filename().string();

      // Create symlink using relative path
      std::filesystem::create_symlink(target_filename, symlink_name);
    } catch (const std::exception& e) {
      // If filesystem API fails, fall back to system call with proper checking
      std::string target_filename = target_path.filename().string();
      std::string cmd = "ln -sf " + target_filename + " " + symlink_name;

      // Fix: Check the system call return value
      int ret = system(cmd.c_str());
      if (ret != 0) {
        // Handle the error but continue - symlink failure shouldn't stop
        // logging We could log this error, but that would cause recursion, so
        // just continue silently
      }
    }
  }

  // Clean up old files to ensure total size doesn't exceed limit
  void cleanup_old_files() {
    static std::mutex cleanup_mutex;
    std::lock_guard<std::mutex> lock(
        cleanup_mutex);  // Prevent multiple threads from cleaning up
                         // simultaneously

    // Get log directory
    std::filesystem::path path(base_filename_);
    std::filesystem::path dir_path = path.parent_path();
    if (dir_path.empty()) {
      dir_path = ".";  // If no path specified, use current directory
    }

    // Get log file prefix and base filename
    std::string file_prefix = log_type_ + ".";
    std::string base_name   = path.filename().string();

    // Collect all matching log files
    std::vector<std::filesystem::directory_entry> log_files;

    try {
      for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        // Check if filename matches pattern: type.timestamp.base_filename
        if (filename.find(file_prefix) == 0 &&
            filename.find(base_name) != std::string::npos) {
          log_files.push_back(entry);
        }
      }
    } catch (const std::exception& e) {
      // If directory iteration fails, return without cleanup
      return;
    }

    // If no files or only one file found, return
    if (log_files.size() <= 1) {
      return;
    }

    // Sort by modification time (oldest first)
    std::sort(log_files.begin(), log_files.end(),
        [](const std::filesystem::directory_entry& a,
            const std::filesystem::directory_entry& b) {
          return a.last_write_time() < b.last_write_time();
        });

    // Calculate total size
    uint64_t total_size = 0;
    for (const auto& file : log_files) {
      total_size += static_cast<uint64_t>(file.file_size());
    }

    // Delete oldest files until total size is below limit
    // But always keep the newest file
    for (auto it = log_files.begin();
         it != log_files.end() - 1 && total_size > max_total_size_; ++it) {
      uint64_t file_size = static_cast<uint64_t>(it->file_size());

      try {
        // Check if file is not the current one being written to
        std::string curr_path = file_helper_.filename();
        if (it->path().string() != curr_path) {
          std::filesystem::remove(it->path());
          total_size -= file_size;
        }
      } catch (const std::exception& e) {
        // If deletion fails, continue with other files
        continue;
      }
    }
  }

  spdlog::details::file_helper file_helper_;
  std::string base_filename_;
  std::string log_type_;
  uint64_t max_size_;
  uint64_t max_total_size_;
  std::atomic<uint64_t>
      current_size_;  // Atomic variable tracking current file size
  std::chrono::steady_clock::time_point
      last_rotation_check_;  // Last rotation check time
  std::chrono::steady_clock::duration
      rotation_check_interval_;  // Rotation check interval
  Mutex rotation_mutex_;         // Mutex to protect file rotation operations
};

using custom_rotating_file_sink_mt = custom_rotating_file_sink<std::mutex>;
using custom_rotating_file_sink_st =
    custom_rotating_file_sink<spdlog::details::null_mutex>;

}  // namespace mm_log
