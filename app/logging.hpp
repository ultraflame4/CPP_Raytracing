#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/spdlog.h>
#include <string>

typedef std::shared_ptr<spdlog::logger> Logger;



class logging {
public:
  Logger static get(const std::string &name = "") {
    if (!initialised)
      init();

    std::string logger_name = name;

    return createLogger(logger_name);
  }

private:
  static inline std::vector<spdlog::sink_ptr> sinks{};
  static inline bool initialised = false;
  static void init() {
    initialised = true;

    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sinks.push_back(console);

#ifdef _WIN32
    ((std::shared_ptr<spdlog::sinks::wincolor_stdout_sink_mt>)console)
        ->set_color(spdlog::level::trace, 0x0008);
#else
    ((std::shared_ptr<spdlog::sinks::ansicolor_stdout_sink_mt>)console)
        ->set_color(spdlog::level::trace, "\033[100m");
#endif
    const std::chrono::zoned_time cur_time{std::chrono::current_zone(),
                                           std::chrono::system_clock::now()};

#ifdef LOG_FILENAME_INCLUDE_TIME
    const std::string logfile_name =
        std::format("./logs/{:%d-%m-%Y_%H-%M-%OS}_log.txt", cur_time);
#else
    const std::string logfile_name = "./logs/log.txt";
#endif

    sinks.push_back(
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(logfile_name));

    get("logging")->info("Initialised logging.");
  }

  static Logger createLogger(const std::string &logger_name = "") {
    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>(
        logger_name, sinks.begin(), sinks.end());
    logger->set_pattern("%T.%e %^[%n] - %l %$: %v");
    logger->set_level(spdlog::level::trace);
    return logger;
  }

private:
  static inline std::optional<Logger> _rootLogger;

public:
  static Logger getRootLogger() {
    if (!_rootLogger.has_value()) {
      _rootLogger = logging::get("*root");
    }
    return *_rootLogger;
  }
};

class WithLogger {
public:
  virtual std::string LOG_NAME() const = 0;

protected:
  std::optional<Logger> _logger;

  Logger getLogger() {
    if (!_logger.has_value()) {
      _logger = logging::get(LOG_NAME());
    }
    return *_logger;
  }

  inline Logger quartz_getLogger() { return getLogger(); }
};

/**
 * Literal class type that wraps a constant expression string.
 *
 * Uses implicit conversion to allow templates to *seemingly* accept constant
 * strings.
 */
template <size_t N> struct StringLiteral {
  constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }

  char value[N];
};

template <StringLiteral loggerName> class WithStaticLogger {
public:
  static std::string LOG_NAME() {
    constexpr auto size = sizeof(loggerName.value);
    constexpr auto contents = loggerName.value;
    std::string str(contents, size);
    return str;
  };

protected:
  static inline std::optional<Logger> _logger = std::nullopt;

  static Logger getLogger() {
    if (!_logger.has_value()) {
      _logger = logging::get(LOG_NAME());
    }
    return *_logger;
  }

  inline static Logger quartz_getLogger() { return getLogger(); }
};

#define CLS_LOG_NAME(s)                                                        \
  inline std::string LOG_NAME() const override { return s; };



// Macros for easy access to the logger

#define Log quartz_getLogger()
#define PrintLoggerAddress                                                     \
  quartz_getLogger()->debug("Logger located at mem address: {}",               \
                            static_cast<void *>(&*quartz_getLogger()));
#define RootLog logging::getRootLogger()

// Defa
inline Logger quartz_getLogger() { return logging::getRootLogger(); }