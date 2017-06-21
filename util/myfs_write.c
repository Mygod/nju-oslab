#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "kernel/fs.h"

int imageFd;

int main(int argc, char * const *argv) {
  assert(argc == 3);
  imageFd = open(argv[1], O_RDWR);
  if (imageFd < 0) {
    perror("open failed");
    assert(0);
  }
  fs_init();
  int fd = fs_open(argv[2], O_RDWR | O_CREAT);
  if (fd < 0) {
    fprintf(stderr, "fs_open failed: %d\n", fd);
    assert(0);
  }
  uint8_t buffer[SECTSIZE];
  ssize_t result;
  while ((result = read(STDIN_FILENO, buffer, SECTSIZE)) > 0) {
    int r = fs_write(fd, buffer, (int) result);
    if (r < 0) {
      fprintf(stderr, "fs_write failed: %d\n", r);
      assert(0);
    }
    assert(r == result);
  }
  if (result < 0) {
    perror("read failed");
    assert(0);
  }
  fs_close(fd);
  close(imageFd);
  return 0;
}
