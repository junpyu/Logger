#ifndef LOGGER_LOGGER_H
#define LOGGER_LOGGER_H

#include <cstring>
#include <string>
#include <algorithm>

// To prevent the copy
class noncopyable
{
protected:
    noncopyable() {}
    ~noncopyable() {}

private:
    noncopyable(const noncopyable &);
    const noncopyable &operator=(const noncopyable &);
};

namespace detail
{
    const int kSmallBuffer = 4000;
    const int kLargeBuffer = 4000 * 1000;

    template <int SIZE>
    class FixedBuffer : noncopyable
    {
    public:
        FixedBuffer()
            : cur_(data_)
        {
            setCookie(cookieStart);
        }

        ~FixedBuffer()
        {
            setCookie(cookieEnd);
        }

        void append(const char * /*restrict*/ buf, size_t len)
        {
            // FIXME: append partially
            if ((size_t)avail() > len)
            {
                memcpy(cur_, buf, len);
                cur_ += len;
            }
        }

        const char *data() const { return data_; }
        int length() const { return static_cast<int>(cur_ - data_); }

        // write to data_ directly
        char *current() { return cur_; }
        int avail() const { return static_cast<int>(end() - cur_); }
        void add(size_t len) { cur_ += len; }

        void reset() { cur_ = data_; }
        void bzero() { memZero(data_, sizeof data_); }

        // for used by GDB
        const char *debugString();
        void setCookie(void (*cookie)()) { cookie_ = cookie; }
        // for used by unit test
        std::string toString() const { return std::string(data_, length()); }
        //StringPiece toStringPiece() const { return StringPiece(data_, length()); }

    private:
        const char *end() const { return data_ + sizeof data_; }
        // Must be outline function for cookies.
        static void cookieStart();
        static void cookieEnd();

        void (*cookie_)();
        char data_[SIZE];
        char *cur_;
    };
    template <int SIZE>
    const char *FixedBuffer<SIZE>::debugString()
    {
        *cur_ = '\0';
        return data_;
    }

    template <int SIZE>
    void FixedBuffer<SIZE>::cookieStart()
    {
    }
    template <int SIZE>
    void FixedBuffer<SIZE>::cookieEnd()
    {
    }
} // namespace detail

// Log
namespace Log
{
    class LogStream : noncopyable
    {
        typedef LogStream self;

    public:
        typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

        self &operator<<(bool v)
        {
            buffer_.append(v ? "1" : "0", 1);
            return *this;
        }

        self &operator<<(short);
        self &operator<<(unsigned short);
        self &operator<<(int);
        self &operator<<(unsigned int);
        self &operator<<(long);
        self &operator<<(unsigned long);
        self &operator<<(long long);
        self &operator<<(unsigned long long);

        self &operator<<(const void *);

        self &operator<<(float v)
        {
            *this << static_cast<double>(v);
            return *this;
        }
        self &operator<<(double);
        // self& operator<<(long double);

        self &operator<<(char v)
        {
            buffer_.append(&v, 1);
            return *this;
        }

        // self& operator<<(signed char);
        // self& operator<<(unsigned char);

        self &operator<<(const char *str)
        {
            if (str)
            {
                buffer_.append(str, strlen(str));
            }
            else
            {
                buffer_.append("(null)", 6);
            }
            return *this;
        }

        self &operator<<(const unsigned char *str)
        {
            return operator<<(reinterpret_cast<const char *>(str));
        }

        self &operator<<(const std::string &v)
        {
            buffer_.append(v.c_str(), v.size());
            return *this;
        }

        //   self& operator<<(const StringPiece& v)
        //   {
        //     buffer_.append(v.data(), v.size());
        //     return *this;
        //   }

        //   self& operator<<(const Buffer& v)
        //   {
        //     *this << v.toStringPiece();
        //     return *this;
        //   }

        void append(const char *data, int len) { buffer_.append(data, len); }
        const Buffer &buffer() const { return buffer_; }
        void resetBuffer() { buffer_.reset(); }

    private:
        void staticCheck();

        template <typename T>
        void formatInteger(T);

        Buffer buffer_;

        static const int kMaxNumericSize = 32;
    };

    class Logger
    {
    public:
        enum LogLevel
        {
            TRACE,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            NUM_LOG_LEVELS,
        };

        class SourceFile
        {
        public:
            template <int N>
            SourceFile(const char (&arr)[N])
                : data_(arr),
                  size_(N - 1)
            {
                const char *slash = strrchr(data_, '/'); // builtin function
                if (slash)
                {
                    data_ = slash + 1;
                    size_ -= static_cast<int>(data_ - arr);
                }
            }

            // explicit SourceFile(const char* filename)
            //     : data_(filename)
            // {
            //     const char* slash = strrchr(filename, '/');
            //     if (slash)
            //     {
            //     data_ = slash + 1;
            //     }
            //     size_ = static_cast<int>(strlen(data_));
            // }

            const char *data_;
            int size_;
        };

        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char *func);
        Logger(SourceFile file, int line, bool toAbort);
        ~Logger();

        LogStream &stream() { return impl_.stream_; }

        static LogLevel logLevel();
        static void setLogLevel(LogLevel level);

        typedef void (*OutputFunc)(const char *msg, int len);
        typedef void (*FlushFunc)();
        static void setOutput(OutputFunc);
        static void setFlush(FlushFunc);
        //static void setTimeZone(const TimeZone& tz);

    private:
        class Impl
        {
        public:
            typedef Logger::LogLevel LogLevel;
            Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
            // void formatTime();
            void finish();

            //Timestamp time_;
            LogStream stream_;
            LogLevel level_;
            int line_;
            SourceFile basename_;
        };

        Impl impl_;
    };

    // LogLevelName The head of each line of logs
    const char *const LogLevelName[Log::Logger::NUM_LOG_LEVELS] =
        {
            "\033[36mTRACE\033[0m  ",
            "\033[32mDEBUG\033[0m  ",
            "\033[34mINFO \033[0m  ", // blue
            "\033[33mWARN \033[0m  ", // yellow
            "\033[31mERROR\033[0m  ", // red
            "\033[31mFATAL\033[0m  ",
    };

#define LOG_TRACE                            \
    if (Logger::logLevel() <= Logger::TRACE) \
    Logger(__FILE__, __LINE__, Logger::TRACE, __func__).stream()
#define LOG_DEBUG                            \
    if (Logger::logLevel() <= Logger::DEBUG) \
    Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
#define LOG_INFO                            \
    if (Logger::logLevel() <= Logger::INFO) \
    Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()

} // namespace Log

#endif // LOGGER_LOGGER_H