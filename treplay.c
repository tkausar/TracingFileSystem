#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <asm/unistd_64.h>

#define BUFFER_SIZE 4096
//#define tfile "/tmp/tfile"
#define umode_t unsigned short
#define MAX_SIZE 100

//d-s to maintain open-close consistency among multiple files
struct file_map{
  unsigned int fd;
  size_t fp;
  struct file_map *next;
}fp_fd[MAX_SIZE];

//read_disp_exe reads one record at a time from the tfile and returns the read bytes 
int read_disp_exe(char *cur_p,char flag)          
{
  unsigned int rec_ID,flags;
  unsigned short path_len = 0, rec_size, rec_type, rec_len,buf_len=0;
  umode_t mode=0;
  char *path=NULL,*p=cur_p,*buf=NULL;
  int ret_val,rc=0,fd,bufsize,arg_int,k;
  size_t filep = 0,count;
  loff_t ppos;
  dev_t dev;
  void *arg3;
  struct stat stat_u;
  struct file_map *newnode,*temp;

  memcpy((void*)&rec_ID,(void*)p,sizeof(rec_ID));
  p+=sizeof(rec_ID);
  printf("Record ID =%u | ",rec_ID);

  memcpy((void*)&rec_size,(void*)p,sizeof(rec_size));
  p+=sizeof(rec_size);
  printf("Record Size=%hu | ",rec_size);

  memcpy((void*)&rec_type,(void*)p,sizeof(rec_type));
  p+=sizeof(rec_type);
  printf("Record type(syscall no)=%hu | ",rec_type);

  if ((rec_type == __NR_read) || (rec_type == __NR_write)) {
    memcpy((void *)&count,(void *)p,sizeof(count));
    memcpy((void *)&ppos,(void *)p,sizeof(ppos));
    p+= sizeof(count) + sizeof(ppos);
    printf("Count=%lu | Ppos=%llu | ",count,ppos);
  }
  else if ((rec_type == __NR_open) || (rec_type == __NR_creat) || (rec_type == __NR_mknod)
      || (rec_type == __NR_chmod) || (rec_type == __NR_mkdir)) {
    memcpy((void *)&mode,(void *)p,sizeof(mode));
    p+= sizeof(mode);
    printf("Mode=%hu | ",mode);
  }
  else if ((rec_type == __NR_lseek)) {
    memcpy((void *)&ppos,(void *)p,sizeof(ppos));
    p+= sizeof(ppos);
    printf("Ppos=%llu | ",ppos);
  }
  else if ((rec_type == __NR_setxattr) ||(rec_type == __NR_getxattr) || (rec_type == __NR_listxattr)) {
    memcpy((void *)&count,(void *)p,sizeof(count));
    p+= sizeof(count) ;
    printf("Count=%lu | ",count);

  }

  if (rec_type == __NR_open){
    memcpy((void *)&flags,(void *)p,sizeof(flags));
    p+= sizeof(flags);
    printf("Flags=%u | ",flags);
  }

  else if (rec_type == __NR_mknod) {
    memcpy((void *)&dev,(void *)p,sizeof(dev));
    p+= sizeof(dev);
    printf("Dev=%d | ",dev);
  }

  else if ((rec_type ==__NR_readlink) || (rec_type == __NR_lseek) || (rec_type == __NR_setxattr)) {
    memcpy((void *)&arg_int,(void *)p,sizeof(arg_int));
    p+= sizeof(arg_int);
    if(rec_type==__NR_readlink)
      printf("Buffer Size=%d | ",arg_int);
    else if(rec_type==__NR_setxattr)
      printf("Flags=%d | ",arg_int);

  }

  if ((rec_type == __NR_setxattr) ||(rec_type == __NR_getxattr)) {
    memcpy(arg3,(void*)p,count);    //count has the void ptr size
    //  printf("File ptr=%lx | ",filep);
    p+=count;

  }
  else if (rec_type == __NR_stat) {
    memcpy((void*)&stat_u,(void*)p,sizeof(struct stat));
    // printf("Size of struct stat=%d | ",sizeof(struct stat));
    p+=sizeof(struct stat);


  }
  else if (rec_type == __NR_read || rec_type == __NR_write || rec_type == __NR_open || rec_type == __NR_close || rec_type == __NR_fsync)
  {
    memcpy((void*)&filep,(void*)p,sizeof(filep));
    printf("File ptr=%lx | ",filep);
    p+=sizeof(filep);

  }
  if (rec_type != __NR_close){
    memcpy((void*)&path_len,(void*)p,sizeof(path_len));
    p+=sizeof(path_len);
    path=calloc(1,path_len+1);  // For '\0' byte
    memcpy((void*)path,(void*)p,path_len);
    p+=path_len;
    printf("Path_len=%u | ",path_len);
    printf("Filepath=%s | ",path);
  }

  if(rec_type==__NR_read || rec_type == __NR_write || rec_type==__NR_link ||rec_type== __NR_symlink
      || rec_type== __NR_rename || rec_type==__NR_readlink || rec_type==__NR_setxattr || rec_type== __NR_getxattr 
      || rec_type==__NR_listxattr || rec_type==__NR_removexattr)
  {
    memcpy((void*)&buf_len,(void*)p,sizeof(buf_len));
    p+=sizeof(buf_len);
    buf=calloc(1,buf_len+1);  // For '\0' byte
    memcpy((void*)buf,(void*)p,buf_len);
    p+=buf_len;
    printf("Buffer length=%u | ",buf_len);
    printf("\nBuffer=%s\n",buf);
  }


  memcpy((void*)&ret_val,(void*)p,sizeof(ret_val));
  p+=sizeof(ret_val);
  printf("Return value=%d \n\n ",ret_val);

  //execute syscalls if flag is not 'n'
  if(flag!='n')
  {
    switch(rec_type)
    {
      case __NR_removexattr:
        printf("\nReplaying __NR_removexattr\n");
        rc = syscall(rec_type,path,buf);
printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);

        if(rc!=ret_val && flag=='s')
        {
                    return -1;
        }

        break;

      case __NR_listxattr:
        printf("\nReplaying __NR_listxattr\n");

        rc = syscall(rec_type,path,buf,count);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
         // printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }

        break;


      case __NR_setxattr:
        printf("\nReplaying __NR_setxattr\n");

        rc = syscall(rec_type,path,buf,arg3,count,arg_int);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }

        break;
      case __NR_getxattr:
        printf("\nReplaying __NR_getxattr\n");

        rc = syscall(rec_type,path,buf,arg3,count);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }

        break;


      case __NR_mknod:
        printf("\nReplaying mknod\n");

        rc = syscall(rec_type,path,mode,dev);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }

        break;
      case __NR_chmod:
        printf("\nReplaying chmod\n");
        rc = syscall(rec_type,path,mode);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }

        break;

      case __NR_readlink:
        printf("\nReplaying readlink\n");
        rc = syscall(rec_type,path,buf,buf_len);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }

        break;

      case __NR_symlink:
        printf("\nReplaying symlink\n");
        rc = syscall(rec_type,path,buf);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }

        break;

      case __NR_fsync:
        for(k=0;k<MAX_SIZE;++k)
        {fd=-1;
          if(fp_fd[k].fp==filep)
          {
            fd=fp_fd[k].fd;
            break;
          }

        }

        rc = syscall(rec_type,fd);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }

        break;

      case __NR_rename:
        rc = syscall(rec_type,path, buf);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }

        break;
      case __NR_link:
        rc = syscall(rec_type,path, buf);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }


        break;

      case __NR_mkdir:  
        rc = syscall(rec_type,path, mode);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }


        break;

      case __NR_creat:  
       break;

      case __NR_rmdir:  
        rc = syscall(rec_type,path);
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }


        break;

      case __NR_open:  
        fd = syscall(rec_type,path,flags, mode);
        if(fd>=0)
          rc=0;
        else
          rc=-1;
          printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
        printf("\nOpen file %s | fd = %d\n",path,fd);
        if(rc!=ret_val && flag=='s')
        {
          printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
          return -1;
        }
        for(k=0;k<MAX_SIZE;++k)
        {
          if(fp_fd[k].fp==-1)
          {

            fp_fd[k].fp=filep;
            fp_fd[k].fd=fd;
            printf("\nTANWEE=> Added in file_map fp %llx, fd %d",filep,fd);
            break;
          }
        }
        break; 

      case __NR_close:  
        //search fd corresponding to filepointer to be closed
        for(k=0;k<MAX_SIZE;++k)
        {fd=-1;
          if(fp_fd[k].fp==filep)
          {
            fd=fp_fd[k].fd;
            break;
          }

        }
        rc = syscall(rec_type,fd);   
        printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
if(rc!=ret_val && flag=='s')
{
  printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
  return -1;
}


printf("Close file : %s ,fp = %llx ,file descriptor is %d\n",path,filep,fd);


break;

case __NR_read:
for(k=0;k<MAX_SIZE;++k)
{fd=-1;
  if(fp_fd[k].fp==filep)
  {
    fd=fp_fd[k].fd;
    break;
  }
 
}

rc = syscall(rec_type,fd,buf,count);   //TO-DO mapping of fds to handle multiple file open-close consistency
printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
if(rc!=ret_val && flag=='s')
{
  printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
  return -1;
}


printf("Read file : %s ,fp = %llx ,file descriptor is %d\n",path,filep,fd);

break;

case __NR_write:
for(k=0;k<MAX_SIZE;++k)
{fd=-1;
  if(fp_fd[k].fp==filep)
  {
    fd=fp_fd[k].fd;
    break;
  }
  
}

rc = syscall(rec_type,fd,buf,count);   //TO-DO mapping of fds to handle multiple file open-close consistency
printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
printf("Write file : %s ,fp = %llx ,file descriptor is %d\n",path,filep,fd);
if(rc!=ret_val && flag=='s')
{
  printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
  return -1;
}

break;

case __NR_unlink:
rc = syscall(rec_type,path);
printf("\nREPLAY RESULT :\nReplayed syscall's return value = %d | Original syscall's return value = %d \n",rc,ret_val);
if(rc!=ret_val && flag=='s')
{
  printf("Replayed syscall return value = %d | Original syscall return value = %d \n",rc,ret_val);
  return -1;
}


break;


default:
printf("\n%d syscall not found\n",rec_type);
rc=-1; // returning TODO

}

if (rc == 0)          
  printf("Syscall no %d returned %d\n",rec_type, rc); //rc stores the return value from syscall
  else
{
  perror("ERROR found");
  printf("syscall returned %d (errno=%d)\n", rc, errno);
}

}
// printf("\n----------------------------------------------------------------------\n");
return(p-cur_p);
// return rec_size;
}

int main(int argc,char *argv[])
{
  int  err, i = 0,read,ch;
  FILE *fp_read;
  char *buffer,*p=NULL,flag='d',*tfile;   //flag to store command line options
  buffer=calloc(1,BUFFER_SIZE);
  int len,k,file_len;

  /*reading flags from user; by default flag is d
   * ‐n: show what records will be replayed but don't replay them
   * ‐s: strict mode ‐‐ abort replaying as soon as a deviation occurs
   * default: continues replaying even if deviation occurs
   */
  while((ch=getopt(argc,argv,"ns"))!=-1)
  {
    switch(ch)
    {
      case 'n':
        flag='n';
        break;
      case 's':
        flag='s';
        break;
      default:
        return(-1);
    }

  }

printf("Treplay option = %c\n",flag);
if(argv[optind]==NULL)
{
printf("Please specify tfile\n");
return -1;
}
tfile=argv[optind];
printf("TFILE=%s\n",tfile);

  //fp_fd map initialisation
  for(k=0;k<MAX_SIZE;++k)
  {
    fp_fd[k].fp=-1;
    fp_fd[k].fd=-1;
  }

  //read from file to buffer
  fp_read=fopen(tfile,"r");
  if(fp_read<0)
  {
  printf("Cannot read the file");
  return -1;
  }
  read = fread(buffer,1,BUFFER_SIZE,fp_read); 
  p=buffer;
  //TO-DO handle case when tfile > PAGESIZE
  while((p-buffer) < read)
  {
    printf("Reading new record : \n");
    len = read_disp_exe(p,flag); //function that reads,displays and execute per record; len returns the 
    if(len==-1)
    {
      printf("Return values didn't match. ABORTING!!\n");
      break;
    }
    p+=len;
    printf("Record length read = %d | Total record length read till now = %d\n",len,(p-buffer));
    printf("\n----------------------------------------------------------------------------------------------------------------------\n");

  }
  //close file
  fclose(fp_read);
}

