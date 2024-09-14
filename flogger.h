#ifndef FLOGGER_H
#define FLOGGER_H

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <cstring> // strrchr

using std::endl;

namespace
{ // Anonymous namespace

class Flogger
{
private:
    std::ofstream outFile;
    std::mutex mtx;                // Mutex to protect file access
    std::ostringstream buffer;     // Buffer to accumulate log messages
    const std::string logFilePath; // Default log file path

    // Flush the buffer to the file with a timestamp
    void flushBuffer()
    {
        if (!buffer.str().empty() && outFile.is_open())
        {
            outFile << buffer.str();
            buffer.str("");  // Clear the buffer
            buffer.clear();  // Clear any error state
        }
    }

public:
    // Constructor: opens the file in append and binary mode with the default path
    Flogger(const std::string& logFilePath) : 
        logFilePath(logFilePath)
    {
        outFile.open(logFilePath, std::ios::binary | std::ios::app);
        if (!outFile.is_open())
        {
            std::cerr << "Error: Unable to open log file: " << logFilePath << std::endl;
        }
    }

    // Destructor: closes the file
    virtual ~Flogger()
    {
        if (outFile.is_open())
        {
            flushBuffer(); // Ensure the remaining buffer is flushed
            outFile.close();
        }
    }

    // Templated operator<< for logging various types of input
    template<typename T>
    Flogger& operator<<(const T& input)
    {
        std::lock_guard<std::mutex> lock(mtx); // Lock the mutex to ensure thread safety
        buffer << input;                       // Add input to the buffer
        return *this;
    }

    // Overload for adding std::endl functionality
    Flogger& operator<<(std::ostream& (*manip)(std::ostream&))
    {
        std::lock_guard<std::mutex> lock(mtx); // Lock the mutex to ensure thread safety
        if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
        {
            flushBuffer();    // Flush buffer when std::endl is encountered
            outFile << manip; // Also write the std::endl to the file
        }
        else
        {
            buffer << manip; // Add the manipulator to the buffer
        }
        return *this;
    }

    // Function to get the current time as a formatted string with milliseconds
    static std::string getCurrentTime()
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm* localTime = std::localtime(&nowTime);

        std::ostringstream oss;
        oss << std::put_time(localTime, "%H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    static std::string getTID()
    {
        std::thread::id this_id = std::this_thread::get_id();

        // Convert the thread ID to a numeric representation using a stringstream
        std::ostringstream oss;
        oss << std::hex << this_id;

        std::string thread_id_str = "0x";
        thread_id_str.append(oss.str());

        return thread_id_str;
    }

};

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define BFlog Flogger("/mnt/share/gc_continuous_testing/debug_log.txt") \
    << __FILENAME__ << ":" << __LINE__

#define Flog Flogger("/mnt/share/gc_continuous_testing/debug_log.txt") \
    << Flogger::getCurrentTime() << " | " << Flogger::getTID() << " | " \
    << __FILENAME__ << ":" << __LINE__ << " @" << __func__ << ": "

#define Ftrace Flogger("/mnt/share/gc_continuous_testing/debug_log.txt") \
    << Flogger::getCurrentTime() << " | " << Flogger::getTID() << " | " \
    << "* " << __FILENAME__ << ":" << __LINE__ << " @" << __PRETTY_FUNCTION__ << endl;

#define SMLCDLog Flogger("/mnt/share/gc_continuous_testing/SMLCD_SCREEN.txt")

} // End of anonymous namespace

#endif // FLOGGER_H
