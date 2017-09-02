#ifndef LOG_H
#define LOG_H

#define LOGFILE "../log.txt"

void log_error(const char *message, ...);
void log_info(const char *message, ...);
void log_warning(const char *message, ...);

#endif /* LOG_H */
