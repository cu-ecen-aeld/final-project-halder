#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>

#include "sysmon.h"

#define SYSMON_FILE "/dev/sysmon"
#define SYSMON_UPDATE_INTERVAL_MS 1000 /* 1 second */


static volatile sig_atomic_t keep_running = 1;

static void signal_handler(int signal)
{
    (void)signal;
    keep_running = 0;
}

static int setup_signal_handlers(void)
{
    struct sigaction sa;
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Failed to set up signal handler: %s", strerror(errno));
        return -1;
    }

    return 0;
}

static void daemonize()
{
    pid_t pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0)
        exit(EXIT_FAILURE);
    
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int devnull = open("/dev/null", O_RDWR);
    if (devnull < 0)
        exit(EXIT_FAILURE);
    
    if (dup2(devnull, STDIN_FILENO) < 0 ||
        dup2(devnull, STDOUT_FILENO) < 0 ||
        dup2(devnull, STDERR_FILENO) < 0) {
            exit(EXIT_FAILURE);
    }
    
    if (devnull > 2)
        close(devnull);
}

static int monitoring_loop(int sysmon_fd, int csv_fd) {
    struct pollfd pfd = {
        .fd = sysmon_fd,
        .events = POLLIN
    };

    struct sysmon_data data;
    char csv_string[256];

    while (keep_running) {
        int ret = poll(&pfd, 1, -1);

        if (ret < 0) {
            if (errno == EINTR) {
                if (!keep_running)
                    break;
                continue;
            }

            syslog(LOG_ERR, "poll failed: %s", strerror(errno));
            return -1;
        }

        if (pfd.revents & POLLNVAL) {
            syslog(LOG_ERR, "Invalid sysmon file descriptor (POLLNVAL)");
            return -1;
        }
        
        if (pfd.revents & (POLLERR | POLLHUP)) {
            syslog(LOG_ERR, "sysmon device reported poll error/hangup");
            return -1;
        }

        if (pfd.revents & POLLIN) {
            ssize_t bytes_read = read(sysmon_fd, &data, sizeof(data));

            if (bytes_read == 0) {
                syslog(LOG_ERR, "sysmon device returned EOF");
                return -1;
            }

            if (bytes_read < 0) {
                if (errno == EINTR) {
                    if (!keep_running)
                        break;
                    continue;
                }
                
                syslog(LOG_ERR, "read failed: %s", strerror(errno));
                return -1;
            }

            if (bytes_read != sizeof(data)) {
                syslog(LOG_ERR, "Unexpected read size: %zd", bytes_read);
                return -1;
            }

            time_t now = time(NULL);
            struct tm tm_now;
            char timestamp[32];

            if (localtime_r(&now, &tm_now) == NULL) {
                syslog(LOG_ERR, "monitoring_loop.localtime_r failed: %s", strerror(errno));
                return -1;
            }

            if (strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &tm_now) == 0) {
                syslog(LOG_ERR, "Failed to format timestamp");
                return -1;
            }

            int len = snprintf(csv_string, sizeof(csv_string), "%s,%lu,%lu,%lu,%d,%lu,%s\n",
                     timestamp,
                     data.uptime_seconds,
                     data.free_memory_bytes,
                     data.total_memory_bytes,
                     data.cpu_temperature_millicelsius,
                     data.cpu_frequency_khz,
                     data.hostname);
            
            if (len < 0) {
                syslog(LOG_ERR, "Failed to format CSV data");
                return -1;
            }

            if ((size_t)len >= sizeof(csv_string)) {
                syslog(LOG_ERR, "CSV buffer too small");
                return -1;
            }
            
            size_t csv_len = strlen(csv_string);
            ssize_t bytes_written = write(csv_fd, csv_string, csv_len);

            if (bytes_written < 0) {
                syslog(LOG_ERR, "Failed to write to csv file: %s", strerror(errno));
                return -1;
            }

            if ((size_t)bytes_written != csv_len) {
                syslog(LOG_ERR, "Short write to csv file");
                return -1;
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    (void)argc;

    daemonize();

    openlog(argv[0], LOG_PID, LOG_USER);

    if (setup_signal_handlers() < 0) {
        closelog();
        return EXIT_FAILURE;
    }
   
    int interval = SYSMON_UPDATE_INTERVAL_MS;
    time_t now = time(NULL);
    struct tm tm_now;

    if (localtime_r(&now, &tm_now) == NULL) {
        syslog(LOG_ERR, "localtime_r failed: %s", strerror(errno));
        closelog();
        return -1;
    }

    char date_str[9], csv_path[64];
    
    if (strftime(date_str, sizeof(date_str), "%Y%m%d", &tm_now) == 0) {
        syslog(LOG_ERR, "Failed to format date_str");
        return -1;
    }
    snprintf(csv_path, sizeof(csv_path), "/var/lib/sysmon/%s.csv", date_str);
    
    int sysmon_fd, csv_fd, status;
    ssize_t bytes_written;

    csv_fd = open(csv_path, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
    if (csv_fd == -1) {
        syslog(LOG_ERR, "Could not open %s: %s", csv_path, strerror(errno));
        return EXIT_FAILURE;
    }
    
    char header_row[] = "timestamp,uptime_seconds,free_memory_bytes,total_memory_bytes,cpu_temperature_millicelsius,cpu_frequency_khz,hostname\n";

    bytes_written = write(csv_fd, header_row, strlen(header_row));
    if (bytes_written < 0) {
        syslog(LOG_ERR, "Failed to write to %s: %s", csv_path, strerror(errno));
        close(csv_fd);
        return EXIT_FAILURE;
    }

    if ((size_t)bytes_written != strlen(header_row)) {
        syslog(LOG_ERR, "Short write to: %s", csv_path);
        close(csv_fd);
        return EXIT_FAILURE;
    }

    sysmon_fd = open(SYSMON_FILE, O_RDWR | O_CLOEXEC);
    if (sysmon_fd == -1) {
        syslog(LOG_ERR, "Could not open %s: %s", SYSMON_FILE, strerror(errno));
        close(csv_fd);
        return EXIT_FAILURE;
    }
    
    status = ioctl(sysmon_fd, SYSMON_SET_INTERVAL, &interval);
    if (status < 0) {
        syslog(LOG_ERR, "Setting sysmon update interval failed (ioctl): %s", strerror(errno));
        close(csv_fd);
        close(sysmon_fd);
        return EXIT_FAILURE;
    }

    if (monitoring_loop(sysmon_fd, csv_fd) < 0) {
        syslog(LOG_ERR, "Monitoring loop failed: %s", strerror(errno));
        close(csv_fd);
        close(sysmon_fd);
        closelog();

        return 1;
    }

    close(csv_fd);
    close(sysmon_fd);
    closelog();

    return 0;
}
