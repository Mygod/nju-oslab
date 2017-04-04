#ifndef OSLAB_IDE_H
#define OSLAB_IDE_H

#include "types.h"

#define SECTSIZE  512

int ide_read(void *dst, uint32_t offset, uint32_t count);

#endif //OSLAB_IDE_H
