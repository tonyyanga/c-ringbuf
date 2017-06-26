#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "ringbuf.h"

#define RINGBUF_CAPACITY 65536
#define MMAP_ADDRESS 0x800000

int main() {
    int shm = shm_open("spsc", O_CREAT | O_RDWR, 0666);

    // one byte for sync
    int length = sizeof(uint8_t) + get_buf_size(RINGBUF_CAPACITY);

    //ftruncate(shm, length);

    uint8_t* buf = (uint8_t*) mmap(MMAP_ADDRESS, length, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_FIXED, shm, 0);

    ringbuf_t r_buf = buf + sizeof(uint8_t);

    // status = 0 => nobody reading / writing
    // status = 1 => buffer is full, allow read only
    // status = 2 => buffer is empty, allow write only
    // status = 3 => buffer is being read / write
    uint8_t volatile * status = buf;

    // accept data in
    uint8_t data[5];

    // spinlock for write
    while (true) {
        if (__sync_bool_compare_and_swap(status, 0, 3)) {
            ringbuf_memcpy_from(data, r_buf, 5 * sizeof(uint8_t));
            if (ringbuf_is_empty(r_buf)) {
                *status = 2;
            } else {
                *status = 0;
            }
            printf("%d%d%d%d%d\n", data[0], data[1], data[2], data[3], data[4]);
        } else if (__sync_bool_compare_and_swap(status, 1, 3)) {
            ringbuf_memcpy_from(data, r_buf, 5 * sizeof(uint8_t));
            *status = 0;
            printf("%d%d%d%d%d\n", data[0], data[1], data[2], data[3], data[4]);
        } else {
            usleep(10);
        }
    }

    sleep(5);

    munmap(buf, length);

    shm_unlink("spsc");
}

