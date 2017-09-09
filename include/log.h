#ifndef LOG_H
#define LOG_H

#define LOGFILE     "../log.txt"
#define CPU_LOGFILE "../cpu_log.txt"

#define LOGGING 3

#if defined(LOGGING) && LOGGING>2
# define LOG_INFO(...) log_info(__VA_ARGS__)
#else
# define LOG_INFO(...) do {} while (0)
#endif

#if defined(LOGGING) && LOGGING>1
# define LOG_WARNING(...) log_warning(__VA_ARGS__)
#else
# define LOG_WARNING(...) do {} while (0)
#endif

#if defined(LOGGING) && LOGGING>0
# define LOG_ERROR(...) log_error(__VA_ARGS__)
#else
# define LOG_ERROR(...) do {} while (0)
#endif

#ifdef CPU_LOGGING
# define LOG_CPU(...) log_cpu(__VA_ARGS__)
#else
# define LOG_CPU(...) do {} while (0)
#endif

void log_error(const char *message, ...);
void log_info(const char *message, ...);
void log_warning(const char *message, ...);
void log_cpu(int width, const char *message, ...);

#endif /* LOG_H */
