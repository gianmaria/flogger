#ifndef FLOGGER_H
#define FLOGGER_H

#include <chrono>
#include <cstring> // strrchr
#include <ctime>
//#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#define FMT_HEADER_ONLY 1
#include "fmt/format.h"

class Flogger
{
public:
    
    enum Opt : uint16_t
    {
        none     = 0x0,
        time     = 0x1,
        tid      = 0x2,
        file     = 0x4,
        function = 0x8,
        all      = time | tid | file | function
    };

    Flogger(const Flogger&) = delete;
    void operator=(const Flogger&) = delete;

    virtual ~Flogger()
    {
        if (out_file.is_open())
        {
            flush_Buffer(); // Ensure the remaining buffer is flushed
            out_file.close();
        }
    }

    template<typename... Args>
    void log(uint16_t options, std::string_view time, std::string_view tid, 
             const char* file_, int line, const char* function,
             std::string_view fmt, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mtx); // Lock the mutex to ensure thread safety

        auto msg = fmt::vformat(fmt, fmt::make_format_args(args...));

#ifdef _WIN32
        int separator = '\\';
#else
        int separator = '/';
#endif
        const char* file = (strrchr(file_, separator) ? strrchr(file_, separator) + 1 : file_);
        
        if ((options & Opt::time) == Opt::time)
            buffer << time << "|";
        
        if ((options & Opt::tid) == Opt::tid)
            buffer << tid << "|";

        if ((options & Opt::file) == Opt::file)
            buffer << file << ":" << line << "|";

        if ((options & Opt::function) == Opt::function)
            buffer << "@" << function << "|";

        if (options != Opt::none)
            buffer << " ";
        
        buffer << msg << "\n";

        /*buffer << time << "|" << tid << "|"
            << file << ":" << line << "|"
            << "@" << function << "| "
            << msg << "\n";*/

        flush_Buffer();
    }

    static std::string get_Current_Time()
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&now_time);

        std::ostringstream oss;
        oss << std::put_time(local_time, "%H:%M:%S") 
            << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    static std::string get_TID()
    {
        std::thread::id this_id = std::this_thread::get_id();

        // Convert the thread ID to a numeric representation using a stringstream
        std::ostringstream oss;
        oss << std::hex 
            << std::setw(8) 
            << std::setfill('0') 
            << std::uppercase 
            << this_id;

        std::string thread_id_str = "0x";
        thread_id_str.append(oss.str());

        return thread_id_str;
    }

    static Flogger& instance(std::string_view log_file_path)
    {
        static auto flogger = Flogger(log_file_path);
        return flogger;
    }

protected:

    Flogger(std::string_view log_file_path) 
        : log_file_path(log_file_path)
    {
        out_file.open(log_file_path.data(), std::ios::binary | std::ios::app);
        if (!out_file.is_open())
        {
            std::cerr << "Error: Unable to open log file: " << log_file_path << std::endl;
        }
    }

private:
    std::ofstream out_file;          // file path where to log
    std::mutex mtx;                  // mutex to protect file access
    std::ostringstream buffer;       // buffer to accumulate log messages
    const std::string log_file_path; // default log file path
    uint16_t options;

    // Flush the buffer to the file with a timestamp
    void flush_Buffer()
    {
        if (!buffer.str().empty() && 
            out_file.is_open())
        {
            out_file << buffer.str();
            out_file.flush(); // flush to disk at every log operation

            buffer.str("");  // Clear the buffer
            buffer.clear();  // Clear any error state
        }
    }
};

extern Flogger& default_flog;

#define Flog(fmt, ...)  default_flog.log(Flogger::all,  Flogger::get_Current_Time(), Flogger::get_TID(), __FILE__, __LINE__, __func__, fmt, __VA_ARGS__)
#define Flogb(fmt, ...) default_flog.log(Flogger::none, Flogger::get_Current_Time(), Flogger::get_TID(), __FILE__, __LINE__, __func__, fmt, __VA_ARGS__)
#define Ftrace          default_flog.log(Flogger::file, Flogger::get_Current_Time(), Flogger::get_TID(), __FILE__, __LINE__, __func__, "{}", "*trace*")

#endif // FLOGGER_H


#ifdef FLOGGER_IMPLEMENTATION

Flogger& default_flog = Flogger::instance("debug_log.txt");

#endif