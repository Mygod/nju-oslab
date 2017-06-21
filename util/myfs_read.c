#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "kernel/fs.h"

int imageFd;

int main(int argc, char * const *argv) {
  assert(argc == 3);
  imageFd = open(argv[1], O_RDONLY);
  if (imageFd < 0) {
    perror("open failed");
    assert(0);
  }
  fs_init();
  int fd = fs_open(argv[2], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "fs_open failed: %d\n", fd);
    assert(0);
  }
  uint8_t buffer[SECTSIZE];
  int result;
  while ((result = fs_read(fd, buffer, SECTSIZE)) > 0) {
    ssize_t r = write(STDOUT_FILENO, buffer, (size_t) result);
    if (r < 0) {
      perror("write failed");
      assert(0);
    }
    assert(r == result);
  }
  if (result < 0) {
    fprintf(stderr, "fs_read failed: %d\n", result);
    assert(0);
  }
  fs_close(fd);
  close(imageFd);
  return 0;
}
