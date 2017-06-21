#ifndef OSLAB_IDE_H
#define OSLAB_IDE_H

#include "include/types.h"

#define SECTSIZE  512

int ide_read(uint32_t secno, void *dst, size_t nsecs);
int ide_write(uint32_t secno, const void *src, size_t nsecs);

#endif //OSLAB_IDE_H
