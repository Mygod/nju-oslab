#include "assert.h"
#include "error.h"
#include "string.h"
#include "kernel/fs.h"
#include "pcb.h"

#define FDPOOL_SIZE (ENTRY_COUNT * PROCESS_POOL_SIZE)
#define TRY(x) do { if ((r = (x)) < 0) return r; } while (false)
#define DATA_READ(secno, dest) ide_read((secno) + FSOFFSET_DATA, dest, 1)
#define DATA_WRITE(secno, dest) ide_write((secno) + FSOFFSET_DATA, dest, 1)

#define STATE_UNINITIALIZED 0
#define STATE_READY         1
#define STATE_DIRTY         2

Bitmap bitmap;
Dir dir;
typedef uint8_t SyncState;
SyncState state_bitmap, state_dir;

const char filename_kernel[] = "kernel.bin";

struct {
  int index;
  size_t offset;
  uint8_t buffer[SECTSIZE];
  INode inode;
  uint32_t index_buffer, index_inode;
  bool open, writable;
  SyncState state_buffer, state_inode;
} fdPool[FDPOOL_SIZE];

void fs_init() {
  assert(ide_read(FSOFFSET_BITMAP, &bitmap, FSLENGTH_BITMAP) >= 0);
  assert(ide_read(FSOFFSET_DIR, &dir, 1) >= 0);
  state_bitmap = state_dir = STATE_READY;
}
int fs_flush() {
  int r;
  if (state_bitmap == STATE_DIRTY) TRY(ide_write(FSOFFSET_BITMAP, &bitmap, FSLENGTH_BITMAP));
  if (state_dir == STATE_DIRTY) TRY(ide_write(FSOFFSET_DIR, &dir, 1));
  state_bitmap = state_dir = STATE_READY;
  return 0;
}

int inode_occupy() {
  assert(state_bitmap != STATE_UNINITIALIZED);
  for (int i = 0; i < BITMAP_SIZE; ++i) {
    int j = ~bitmap[i];
    if (j) {
      asm volatile("rep; bsf %1, %0"
      : "=r" (j)
      : "rm" (j));
      bitmap[i] |= 1 << j;
      state_bitmap = STATE_DIRTY;
      return (i << 5) + j;
    }
  }
  return E_DQUOT;
}

int fd_inode_flush(int fd) {
  if (fdPool[fd].state_inode == STATE_DIRTY) {
    int r;
    TRY(DATA_WRITE(fdPool[fd].index_inode, &fdPool[fd].inode));
    fdPool[fd].state_inode = STATE_READY;
  }
  return 0;
}
int fd_inode_fetch(int fd) {
  assert(!(fdPool[fd].offset % INODE_SIZE));
  int r;
  if (fdPool[fd].state_inode == STATE_UNINITIALIZED) {
    if (fdPool[fd].offset == 0) {
      if (dir[fdPool[fd].index].inodeOffset == -1) {
        TRY(inode_occupy());
        fdPool[fd].index_inode = dir[fdPool[fd].index].inodeOffset = (uint32_t) r;
        state_dir = STATE_DIRTY;
        memset(&fdPool[fd].inode, 0, SECTSIZE);
        fdPool[fd].state_inode = STATE_DIRTY;
      } else {
        TRY(DATA_READ(fdPool[fd].index_inode = dir[fdPool[fd].index].inodeOffset, &fdPool[fd].inode));
        fdPool[fd].state_inode = STATE_READY;
      }
    } else {
      if (fdPool[fd].inode.next == 0) {
        TRY(inode_occupy());
        fdPool[fd].inode.next = (uint32_t) r;
        fdPool[fd].state_inode = STATE_DIRTY;
        TRY(fd_inode_flush(fd));
        fdPool[fd].index_inode = fdPool[fd].inode.next;
        memset(&fdPool[fd].inode, 0, SECTSIZE);
        fdPool[fd].state_inode = STATE_DIRTY;
      } else {
        TRY(DATA_READ(fdPool[fd].index_inode = fdPool[fd].inode.next, &fdPool[fd].inode));
        fdPool[fd].state_inode = STATE_READY;
      }
    }
  }
  return 0;
}

int fd_buffer_flush(int fd) {
  if (fdPool[fd].state_buffer == STATE_DIRTY) {
    int r;
    TRY(DATA_WRITE(fdPool[fd].index_buffer, &fdPool[fd].buffer));
    fdPool[fd].state_buffer = STATE_READY;
  }
  return 0;
}
int fd_buffer_fetch(int fd) {
  if (fdPool[fd].state_buffer == STATE_UNINITIALIZED) {
    assert(!(fdPool[fd].offset % SECTSIZE));
    int r;
    if (!(fdPool[fd].offset % INODE_SIZE)) {
      TRY(fd_inode_flush(fd));
      fdPool[fd].state_inode = STATE_UNINITIALIZED;
      TRY(fd_inode_fetch(fd));
    }
    int index = (int) (fdPool[fd].offset / SECTSIZE % INODE_DATA_COUNT);
    if (fdPool[fd].inode.dataBlocks[index] == 0) {
      TRY(inode_occupy());
      fdPool[fd].index_buffer = fdPool[fd].inode.dataBlocks[index] = (uint32_t) r;
      fdPool[fd].state_inode = STATE_DIRTY;
    } else TRY(DATA_READ(fdPool[fd].index_buffer = fdPool[fd].inode.dataBlocks[index], &fdPool[fd].buffer));
    fdPool[fd].state_buffer = STATE_READY;
  }
  return 0;
}

static inline int min(int a, int b) {
  return a < b ? a : b;
}

int fs_open(const char *pathname, int flags) {
  if (!pathname || !*pathname) return E_EMPTYPATH;
  int x = -1;
  for (int i = 0; i < ENTRY_COUNT; ++i) {
    if (x < 0 && !dir[i].fileName[0]) x = i;
    if (!strncmp(pathname, dir[i].fileName, FILENAME_LENGTH))
      for (int j = 0; j < FDPOOL_SIZE; ++j) if (!fdPool[j].open) {
        if (!(fdPool[j].index = i) && (fdPool[j].writable = (flags & O_RDWR) != 0)) return E_ACCESS;
        fdPool[j].open = true;
        fdPool[j].offset = 0;
        fdPool[j].state_buffer = fdPool[j].state_inode = STATE_UNINITIALIZED;
        return j;
      }
  }
  if (flags & O_CREAT) {
    if (x < 0) return E_DQUOT;
    assert(strncmp(pathname, filename_kernel, FILENAME_LENGTH) ? x > 0 : x == 0);
    for (int j = 0; j < FDPOOL_SIZE; ++j) if (!fdPool[j].open) {
      fdPool[j].index = x;
      fdPool[j].writable = (flags & O_RDWR) != 0;
      fdPool[j].open = true;
      fdPool[j].offset = 0;
      fdPool[j].state_buffer = fdPool[j].state_inode = STATE_UNINITIALIZED;
      strncpy(dir[x].fileName, pathname, FILENAME_LENGTH);
      dir[x].fileSize = 0;
      dir[x].inodeOffset = (uint32_t) -1;
      state_dir = STATE_DIRTY;
      return j;
    }
  }
  return E_NOENT;
}
int fs_read(int fd, void *buf, int len) {
  if (!fdPool[fd].open) return E_BADFD;
  len = min(len, (int) (dir[fdPool[fd].index].fileSize - fdPool[fd].offset));
  int result = 0, r;
  while (len > 0) {
    TRY(fd_buffer_fetch(fd));
    int offset = (int) (fdPool[fd].offset % SECTSIZE), count = SECTSIZE - offset;
    if (len < count) count = len;
    memcpy((uint8_t *) buf + result, fdPool[fd].buffer + offset, (size_t) count);
    result += count;
    fdPool[fd].offset += count;
    len -= count;
    if (offset + count == SECTSIZE) {
      TRY(fd_buffer_flush(fd));
      fdPool[fd].state_buffer = STATE_UNINITIALIZED;
    }
  }
  return result;
}
int fs_write(int fd, const void *buf, int len) {
  if (!fdPool[fd].open) return E_BADFD;
  if (!fdPool[fd].writable) return E_ACCESS;
  int result = 0, r;
  while (len > 0) {
    TRY(fd_buffer_fetch(fd));
    int offset = (int) (fdPool[fd].offset % SECTSIZE), count = SECTSIZE - offset;
    if (len < count) count = len;
    memcpy(fdPool[fd].buffer + offset, (uint8_t *) buf + result, (size_t) count);
    fdPool[fd].state_buffer = STATE_DIRTY;
    result += count;
    fdPool[fd].offset += count;
    if (dir[fdPool[fd].index].fileSize < fdPool[fd].offset) {
      dir[fdPool[fd].index].fileSize = (uint32_t) fdPool[fd].offset;
      state_dir = STATE_DIRTY;
    }
    len -= count;
    if (offset + count == SECTSIZE) {
      TRY(fd_buffer_flush(fd));
      fdPool[fd].state_buffer = STATE_UNINITIALIZED;
    }
  }
  return result;
}
int fs_lseek(int fd, int offset, int whence) {
  if (!fdPool[fd].open) return E_BADFD;
  switch (whence) {
    case SEEK_SET:
      if (offset) return E_NOTIMPLEMENTED;
      int r;
      TRY(fd_buffer_flush(fd));
      TRY(fd_inode_flush(fd));
      fdPool[fd].offset = 0;
      fdPool[fd].state_buffer = fdPool[fd].state_inode = STATE_UNINITIALIZED;
      return 0;
    default: return E_NOTIMPLEMENTED;
  }
}
int fs_close(int fd) {
  if (!fdPool[fd].open) return E_BADFD;
  int r;
  TRY(fd_buffer_flush(fd));
  TRY(fd_inode_flush(fd));
  fdPool[fd].open = false;
  TRY(fs_flush());
  return 0;
}
