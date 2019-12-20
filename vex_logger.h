/**
 * vex_logger.h
 * Copyright (c) 2019 Vushu <danvu.hustle@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once
#include <algorithm>
#include <cctype>
#define log_info(msg, ...) \
    vex_logger::log("INFO", __LINE__, __FILE__, msg, __VA_ARGS__)

#define log_warn(msg, ...) \
    vex_logger::log("WARN", __LINE__, __FILE__, msg, __VA_ARGS__)

#define log_debug(msg, ...) \
    vex_logger::log("DEBUG", __LINE__, __FILE__, msg, __VA_ARGS__)

#define log_error(msg, ...) \
    vex_logger::log("ERROR", __LINE__, __FILE__, msg, __VA_ARGS__)

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <thread>
#include <vector>

namespace vex_logger {

struct Log_entry {
    //public:
    std::string log_severity;
    std::string log_message;
    std::string log_date;
    std::string current_file;
    std::string current_line;
};

static std::string log_path = "/tmp/vex_log.txt";

static std::map<std::string, std::string> filters;
static bool append_log = false;
static bool log_listener_started = false;
//lock
static std::mutex buffer_mutex;
//max buffersize
static const int buffer_size = 128;
static Log_entry ring_buffer[buffer_size];
static int read_index = 0;
static int write_index = 0;

static std::string get_file_name(std::string filePath, bool withExtension = true, char seperator = '/')
{
    // Get last dot position
    std::size_t dotPos = filePath.rfind('.');
    std::size_t sepPos = filePath.rfind(seperator);

    if (sepPos != std::string::npos) {
        return filePath.substr(sepPos + 1, filePath.size() - (withExtension || dotPos != std::string::npos ? 1 : dotPos));
    }
    return "";
}

static inline void output_path(std::string path)
{
    log_path = path;
}

static inline void append_output(bool enable)
{
    append_log = enable;
}
static inline void enqueue(Log_entry entry)
{
    std::unique_lock<std::mutex> lock{ buffer_mutex };
    if (write_index >= buffer_size) {
        write_index = 0;
    }
    //std::cout << "write_index " << write_index << std::endl;
    ring_buffer[write_index] = entry;
    write_index++;
    lock.unlock();
}

static inline bool has_entry()
{

    if (read_index > write_index) {
        throw "THIS CANT BE HAPPENING!";
    }
    return read_index > write_index;
}

static Log_entry dequeue()
{
    std::unique_lock<std::mutex> lock{ buffer_mutex };
    if (read_index > write_index) {
        std::cout << "failed" << std::endl;
    }

    if (read_index >= buffer_size) {
        read_index = 0;
    }
    std::cout << "read_index " << read_index << std::endl;
    Log_entry entry = ring_buffer[read_index];
    //print_log_entry(entry);
    std::cout << "after read_index " << std::endl;
    read_index++;

    lock.unlock();
    return entry;
}

static std::string get_color_text(std::string text)
{
    if (text == "INFO")
        return "\x1B[92m" + text + "\033[0m ";
    else if (text == "ERROR")
        return "\x1B[91m" + text + "\033[0m";

    else if (text == "WARN")
        return "\x1B[93m" + text + "\033[0m ";
    else if (text == "DEBUG")
        return "\x1B[96m" + text + "\033[0m";

    return text;
}

static std::string get_text(std::string text)
{

    if (text == "WARN")
        return text + " ";
    if (text == "INFO")
        return text + " ";
    return text;
}

static void write_file(std::string entry)
{
    std::ofstream outfile;
    outfile.open(log_path, std::ios_base::app); // append instead of overwrite
    outfile << entry;
}

static void print_log_entry(Log_entry log_entry)
{
    std::string color_text = get_color_text(log_entry.log_severity);
    std::string log_str = log_entry.log_date + " " + color_text + " " + log_entry.current_file + ":" + log_entry.current_line + " | " + log_entry.log_message + "\n";
    std::string log_text = log_entry.log_date + " " + get_text(log_entry.log_severity) + " " + log_entry.current_file + ":" + log_entry.current_line + " | " + log_entry.log_message + "\n";
    std::cout << log_str;
    write_file(log_text);
}

static inline void read_from_buffer()
{
    while (read_index <= write_index) {
        Log_entry entry = dequeue();
        print_log_entry(entry);
    }
}

static std::string get_timestamp()
{
    struct timeval now;
    struct tm* local;

    gettimeofday(&now, NULL);
    local = localtime(&now.tv_sec);

    char buffer[100];
    sprintf(buffer, "%d-%d-%d %02d:%02d:%02d.%03ld", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, now.tv_usec / 1000);
    return buffer;
}

static Log_entry create_log_entry(std::string log_severity, std::string message, std::string file, int line)
{
    Log_entry log_entry;
    log_entry.log_severity = log_severity;
    log_entry.log_message = message;
    log_entry.log_date = get_timestamp();
    log_entry.current_line = std::to_string(line);
    log_entry.current_file = get_file_name(file);
    return log_entry;
}

static void run_log_listener()
{
    while (true) {
        if (has_entry()) {
            Log_entry entry = dequeue();
            print_log_entry(entry);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

static inline void start_logging()
{
    if (log_listener_started) {
        if (!append_log) {
            std::ofstream outfile(log_path);
            outfile << "";
            outfile.close();
        }
        std::thread log_thread(run_log_listener);
        log_thread.join();
    }
}

static inline std::string str_toupper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::toupper(c); } // correct
    );
    return s;
}

static inline void add_to_filters(std::string filter)
{
    std::string upper_filter = str_toupper(filter);
    if (upper_filter == "INFO" || upper_filter == "DEBUG" || upper_filter == "WARN" || upper_filter == "ERROR") {
        filters.emplace(upper_filter, "");
    } else {
        std::cout << "vex_logger: couldn't add, because of unknown filter: (" << filter << ")" << std::endl;
    }
}

static inline void remove_from_filter(std::string filter)
{
    std::string upper_filter = str_toupper(filter);
    if (upper_filter == "INFO" || upper_filter == "DEBUG" || upper_filter == "WARN" || upper_filter == "ERROR") {
        if (filters.count(upper_filter) > 0) {
            filters.erase(upper_filter);
        } else {

            std::cout << "vex_logger: couldn't remove: (" << filter << ") since filter isn't added." << std::endl;
        }
    } else {
        std::cout << "vex_logger: couldn't remove, because of unknown filter: (" << filter << ")" << std::endl;
    }
}

static inline void remove_filter(std::string filter1)
{
    vex_logger::remove_from_filter(filter1);
}

static inline void remove_filter(std::string filter1, std::string filter2)
{
    vex_logger::remove_from_filter(filter1);
    vex_logger::remove_from_filter(filter2);
}

static inline void remove_filter(std::string filter1, std::string filter2, std::string filter3)
{
    vex_logger::remove_from_filter(filter1);
    vex_logger::remove_from_filter(filter2);
    vex_logger::remove_from_filter(filter3);
}

static inline void remove_filter(std::string filter1, std::string filter2, std::string filter3, std::string filter4)
{
    vex_logger::remove_from_filter(filter1);
    vex_logger::remove_from_filter(filter2);
    vex_logger::remove_from_filter(filter3);
    vex_logger::remove_from_filter(filter4);
}

static inline void print_filters()
{
    for (const auto& x : filters) {
        std::cout << x.first << ": " << x.second << "\n";
    }
}

static inline void log(std::string log_severity, int line, const char* file, const char* msg, ...)
{
    if (filters.empty() || (!filters.empty() && filters.count(log_severity) > 0)) {
        start_logging();
        std::va_list args;
        char buffer[256] = { 0 };
        va_start(args, msg);
        vsprintf(buffer, msg, args);
        va_end(args);

        std::string message = std::string(buffer);
        Log_entry entry = create_log_entry(log_severity, message, file, line);
        print_log_entry(entry);
        // for non blocking maybe threadpool some day
        std::thread enqueue_thread(enqueue, entry);
        enqueue_thread.join();
    }
}
};


static inline void log_append(bool enable)
{
    vex_logger::append_output(enable);
}

static inline void log_output(std::string path)
{
    vex_logger::output_path(path);
}

static inline void log_filter(std::string filter1)
{
    vex_logger::add_to_filters(filter1);
}

static inline void log_filter(std::string filter1, std::string filter2)
{
    vex_logger::add_to_filters(filter1);
    vex_logger::add_to_filters(filter2);
}

static inline void log_filter(std::string filter1, std::string filter2, std::string filter3)
{
    vex_logger::add_to_filters(filter1);
    vex_logger::add_to_filters(filter2);
    vex_logger::add_to_filters(filter3);
}

static inline void log_filter(std::string filter1, std::string filter2, std::string filter3, std::string filter4)
{
    vex_logger::add_to_filters(filter1);
    vex_logger::add_to_filters(filter2);
    vex_logger::add_to_filters(filter3);
    vex_logger::add_to_filters(filter4);
}

static inline void log_remove_filter(std::string filter1)
{
    vex_logger::remove_from_filter(filter1);
}

static inline void log_remove_filter(std::string filter1, std::string filter2)
{
    vex_logger::remove_from_filter(filter1);
    vex_logger::remove_from_filter(filter2);
}

static inline void log_remove_filter(std::string filter1, std::string filter2,
    std::string filter3)
{
    vex_logger::remove_from_filter(filter1);
    vex_logger::remove_from_filter(filter2);
    vex_logger::remove_from_filter(filter3);
}

static inline void log_remove_filter(std::string filter1, std::string filter2,
    std::string filter3, std::string filter4)
{
    vex_logger::remove_from_filter(filter1);
    vex_logger::remove_from_filter(filter2);
    vex_logger::remove_from_filter(filter3);
    vex_logger::remove_from_filter(filter4);
}
