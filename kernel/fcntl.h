#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200



#define PROT_READ (1L << 0)
#define PROT_WRITE (1L << 1)

#define MAP_SHARED (1L << 0)
#define MAP_PRIVATE (1L << 1)