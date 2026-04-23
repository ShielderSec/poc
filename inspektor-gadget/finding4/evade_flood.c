#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char path[64];
    snprintf(path, sizeof(path), "/tmp/flood");
    while(1) {
        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd >= 0)
            close(fd);
    }
    return 0;
}
