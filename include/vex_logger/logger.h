/**
 * include/vex_logger/logger.h
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

#define log_info(msg, ...) \
    vex_logger::log("INFO", __LINE__, __FILE__, msg, __VA_ARGS__)

#define log_warn(msg, ...) \
    vex_logger::log("WARN", __LINE__, __FILE__, msg, __VA_ARGS__)

#define log_debug(msg, ...) \
    vex_logger::log("DEBUG", __LINE__, __FILE__, msg, __VA_ARGS__)

#define log_error(msg, ...) \
    vex_logger::log("ERROR", __LINE__, __FILE__, msg, __VA_ARGS__)

#define log_filter(...) \
    vex_logger::filter(__VA_ARGS__)

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
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

const std::string log_path = "/tmp/vex_last_log.txt";

bool log_listener_started = false;
//lock
std::mutex buffer_mutex;
//max buffersize
const int buffer_size = 128;
Log_entry ring_buffer[buffer_size];
int read_index = 0;
int write_index = 0;

std::string get_file_name(std::string filePath, bool withExtension = true, char seperator = '/')
{
    // Get last dot position
    std::size_t dotPos = filePath.rfind('.');
    std::size_t sepPos = filePath.rfind(seperator);

    if (sepPos != std::string::npos) {
        return filePath.substr(sepPos + 1, filePath.size() - (withExtension || dotPos != std::string::npos ? 1 : dotPos));
    }
    return "";
}

void enqueue(Log_entry entry)
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

bool has_entry()
{

    if (read_index > write_index) {
        throw "THIS CANT BE HAPPENING!";
    }
    return read_index > write_index;
}

Log_entry dequeue()
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

std::string get_color_text(std::string text)
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

std::string get_text(std::string text)
{

    if (text == "WARN")
        return text + " ";
    if (text == "INFO")
        return text + " ";
    return text;
}

void write_file(std::string entry)
{
    std::ofstream outfile;
    outfile.open(log_path, std::ios_base::app); // append instead of overwrite
    outfile << entry;
}

void print_log_entry(Log_entry log_entry)
{
    std::string color_text = get_color_text(log_entry.log_severity);
    std::string log_str = log_entry.log_date + " " + color_text + " " + log_entry.current_file + ":" + log_entry.current_line + " | " + log_entry.log_message + "\n";
    std::string log_text = log_entry.log_date + " " + get_text(log_entry.log_severity) + " " + log_entry.current_file + ":" + log_entry.current_line + " | " + log_entry.log_message + "\n";
    std::cout << log_str;
    write_file(log_text);
}

void read_from_buffer()
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
    sprintf(buffer, "%02d:%02d:%02d.%03ld", local->tm_hour, local->tm_min, local->tm_sec, now.tv_usec / 1000);
    return buffer;
}

Log_entry create_log_entry(std::string log_severity, std::string message, std::string file, int line)
{
    Log_entry log_entry;
    log_entry.log_severity = log_severity;
    log_entry.log_message = message;
    log_entry.log_date = get_timestamp();
    log_entry.current_line = std::to_string(line);
    log_entry.current_file = get_file_name(file);
    return log_entry;
}

void run_log_listener()
{
    while (true) {
        if (has_entry()) {
            Log_entry entry = dequeue();
            print_log_entry(entry);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void start_logging()
{
    if (log_listener_started) {
        std::ofstream outfile(log_path);

        outfile << "";
        outfile.close();
        std::thread log_thread(run_log_listener);
        log_thread.join();
    }
}

void log(std::string log_severity, int line, const char* file, const char* msg, ...)
{
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
    std::thread enqueue_thread(enqueue,entry);
    enqueue_thread.join();
}
};
