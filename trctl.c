#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SET_MAP     0xFE
#define GET_MAP     0xFF
#define SEQ_NO       1
#define SET_FS_MAP  _IOW(SET_MAP, SEQ_NO, int)  
#define GET_FS_MAP  _IOR(GET_MAP, SEQ_NO, int)
#define ALL         "all"
#define NONE        "none"
#define MAP_ALL      0xFFFFFFFF
#define MAP_NONE      0x00

int main(int argc, char *argv[])
{

  int rc =0, fs_map = 0xFFFFFFFF, fd = 0, get_fs_map = 0;

  if (!(argc == 2) && !(argc == 3)) {
    printf("Invalid number of arguments\n");
    return -1;
  } 

  // set map request
  if ((argc == 3)) {
    fs_map = (int)strtol(argv[1], NULL, 16);

    if (!strcmp(argv[1], ALL))
      fs_map = MAP_ALL; 

    if (!strcmp(argv[1], NONE))
      fs_map = MAP_NONE; 

    fd = open(argv[2], O_DIRECTORY);
    if (fd == -1)
    {
      perror("Error");
      goto out;
    }

    rc = ioctl (fd, SET_FS_MAP, fs_map);

    printf ("Set map request sent\nMap value: %s\nMount point: %s\n", argv[1], argv[2]);

  }
  else if ( (argc == 2)) {
    // get map request
    fd = open(argv[1], O_DIRECTORY);
    if (fd == -1)
    {
      perror("Error");
      goto out;
    }

    rc = ioctl (fd, GET_FS_MAP, &get_fs_map);
    printf ("Get map request received\nMap value: %x\nMount point: %s\n", get_fs_map, argv[1]);
  }


out:
  if (fd)
    close(fd);

  exit (rc);
}
