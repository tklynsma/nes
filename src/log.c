#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/log.h"

void put_timestamp(FILE *file) {
    char buffer[256];
    time_t t = time(NULL);
    char *time_ = strtok(ctime(&t), "\n");
    sprintf(buffer, "[%s] - ", time_);
    fputs(buffer, file);
}

void put_message(const char *type, const char *message, va_list args) {
    FILE *file = fopen(LOGFILE, "a");

    if (file != NULL) {
        put_timestamp(file);
        char type_[16];
        sprintf(type_, "%s: ", type);
        fputs(type_, file); printf(type_);

        char message_[256];
        vsnprintf(message_, 256, message, args);

        fputs(message_, file); printf(message_);
        fputs("\n", file); printf("\n");
        fclose(file);
    }
    else {
        perror("Error opening log file.");
    }
}

void log_info(const char *message, ...) {
    va_list args;
    va_start(args, message);
    put_message("INFO", message, args);
    va_end(args);
}

void log_warning(const char *message, ...) {
    va_list args;
    va_start(args, message);
    put_message("WARNING", message, args);
    va_end(args);
}

void log_error(const char *message, ...) {
    va_list args;
    va_start(args, message);
    put_message("ERROR", message, args);
    va_end(args);

    return exit(EXIT_FAILURE);
}

void log_cpu(int width, const char *message, ...) {
    FILE *file = fopen(CPU_LOGFILE, "a");

    if (file != NULL) {
        va_list args;
        va_start(args, message);
        char message_[256];
        vsnprintf(message_, 256, message, args);
        va_end(args);

        char padded_message[256];
        sprintf(padded_message, "%-*s", width, message_);

        fputs(padded_message, file);
        fclose(file);
    }
    else {
        perror("Error opening log file.");
    }
}
