// Test-only Linux interposer. Delays reads and metadata lookups under one
// synthetic media root; does not alter bytes or the application's other I/O.
// Compile with: cc -shared -fPIC -O2 slow-storage.c -o slow-storage.so -ldl
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static unsigned long delayed_calls;

static int selected(const char* path) {
    const char* root = getenv("BITEDJ_SLOW_STORAGE_ROOT");
    if (!root || !*root || !path) return 0;
    const size_t size = strlen(root);
    return strncmp(path, root, size) == 0 && (path[size] == '/' || path[size] == '\0');
}

static int selected_fd(int fd) {
    char link[64], path[4096];
    snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
    const ssize_t size = readlink(link, path, sizeof(path) - 1);
    if (size < 0) return 0;
    path[size] = '\0';
    return selected(path);
}

static void delay(size_t bytes) {
    // 8 MiB/s plus 2 ms operation latency: a deliberately slow media profile,
    // not emulation/certification of the physical USB bus or power supply.
    const unsigned long ns = 2000000UL + bytes * 1000000000UL / (8UL * 1024 * 1024);
    struct timespec duration = {ns / 1000000000UL, ns % 1000000000UL};
    const unsigned long calls = __atomic_add_fetch(&delayed_calls, 1, __ATOMIC_RELAXED);
    if (calls == 1 || calls % 256 == 0) {
        fprintf(stderr, "Slow storage test: %lu delayed operations\n", calls);
    }
    while (nanosleep(&duration, &duration) != 0) {}
}

ssize_t read(int fd, void* buffer, size_t size) {
    ssize_t (*real_read)(int, void*, size_t) = dlsym(RTLD_NEXT, "read");
    const ssize_t count = real_read(fd, buffer, size);
    if (count > 0 && selected_fd(fd)) delay((size_t)count);
    return count;
}

ssize_t pread(int fd, void* buffer, size_t size, off_t offset) {
    ssize_t (*real_pread)(int, void*, size_t, off_t) = dlsym(RTLD_NEXT, "pread");
    const ssize_t count = real_pread(fd, buffer, size, offset);
    if (count > 0 && selected_fd(fd)) delay((size_t)count);
    return count;
}

ssize_t pread64(int fd, void* buffer, size_t size, off64_t offset) {
    ssize_t (*real_pread)(int, void*, size_t, off64_t) = dlsym(RTLD_NEXT, "pread64");
    const ssize_t count = real_pread(fd, buffer, size, offset);
    if (count > 0 && selected_fd(fd)) delay((size_t)count);
    return count;
}

int statx(int fd, const char* path, int flags, unsigned int mask, struct statx* buffer) {
    int (*real_statx)(int, const char*, int, unsigned int, struct statx*) = dlsym(RTLD_NEXT, "statx");
    if (selected(path) || (path && path[0] != '/' && selected_fd(fd))) delay(0);
    return real_statx(fd, path, flags, mask, buffer);
}

__attribute__((destructor)) static void report(void) {
    fprintf(stderr, "Slow storage test: %lu delayed operations\n",
            __atomic_load_n(&delayed_calls, __ATOMIC_RELAXED));
}
