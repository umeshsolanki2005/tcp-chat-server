#include "logger.h"

static FILE *g_log_file = NULL;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

void logger_init(const char *path) {
    if (g_log_file != NULL) {
        return;
    }

    g_log_file = fopen(path, "a");
    if (g_log_file == NULL) {
        perror("fopen");
    }
}

void logger_close(void) {
    if (g_log_file != NULL) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

void logger_log(const char *level, const char *fmt, ...) {
    va_list args;
    char timestamp[64];
    struct tm *tm_info;
    time_t now;

    if (g_log_file == NULL) {
        return;
    }

    pthread_mutex_lock(&g_log_mutex);
    time(&now);
    tm_info = localtime(&now);
    if (tm_info != NULL) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(timestamp, sizeof(timestamp), "unknown");
    }

    fprintf(g_log_file, "[%s] [%s] ", timestamp, level);
    va_start(args, fmt);
    vfprintf(g_log_file, fmt, args);
    va_end(args);
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
    pthread_mutex_unlock(&g_log_mutex);
}
