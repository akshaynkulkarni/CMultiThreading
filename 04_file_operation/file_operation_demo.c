#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <dirent.h>

#define FILE_NAME_LEN 25

mode_t getumask(void) {
  mode_t mask = umask(0);
  umask(mask);
  return mask;
}

void print_file_permissions(char *file_name, mode_t xmode) {
  struct stat file_info;

  if (!stat(file_name, &file_info)) {
    printf("File permissions for file %s are: %o ", file_name,
           file_info.st_mode);
    printf(" (%s: ", xmode & S_IRWXU   ? "user"
                     : xmode & S_IRWXG ? "Group"
                     : xmode & S_IRWXO ? "Other"
                                       : "Unknown");
    mode_t file_mode = file_info.st_mode & xmode;
    if (file_mode & (S_IWOTH | S_IWUSR | S_IWGRP))
      printf("Write ");
    if (file_mode & (S_IROTH | S_IRUSR | S_IRGRP))
      printf("Read ");
    if (file_mode & (S_IXOTH | S_IXUSR | S_IXGRP))
      printf("Execute");
    printf(")\n");
  }
}

void populate_file_name(char *file_name, size_t len) {
  snprintf(file_name, len, "%d-%ld.txt", getpid(), time(NULL));
}

int main() {

  char file_name[2][FILE_NAME_LEN] = {0};
  int fd[2];

  populate_file_name(file_name[0], FILE_NAME_LEN);
  printf("creating a file named \"%s\"\n", file_name[0]);
  fd[0] = open(file_name[0], O_CREAT | O_RDWR, 0666);
  getchar();
  populate_file_name(file_name[1], FILE_NAME_LEN);
  // tmpnam(file_name); // create a file with unique name under /tmp/

  printf("Enter a text to save in %s file:", file_name[0]);

  char input[100] = {0};

  fgets(input, 100, stdin);

  printf("Writing to the file %s..\n", file_name[0]);

  size_t wsize = write(fd[0], (const void *)&input, sizeof(input));

  getchar();

  char read_buff[100] = {0};

  lseek(fd[0], 0, SEEK_SET); // reset file offset to SOF
                             // SEEK_SET: set position to offset
                             // SEEK_CUR: curr location + offset
                             // SEEK_END: EOF + offset

  size_t rsize = read(fd[0], (void *)&read_buff, wsize);
  read_buff[99] = '\0'; // to avoid SIGSEGV
  printf("Read from file: %s\n", read_buff);

  getchar();

  printf("creating a file named \"%s\"\n", file_name[1]);
  // file permissions has 4 digits: giving 666 is not interpreted as it is
  // supposed to be. it should be 0666 (octal). or use macros: S_IRGRP | S_IWGRP
  // : Individual:  S_I<R/W/X><USR/GRP/OTH> RWX for : S_IRWX<U/G/O>

  // The opened file will always have permissions = file permissions - umask;
  // in case of my system: it is 0002; Therefore effective permissions are 0664
  // to evade this, one can use umask(0000); But ensure, we backup the previous
  // umask!

  mode_t o_umask = umask(0000);
  printf("Old umask = %u\n", o_umask);
  printf("New umask = %u\n", getumask());

  fd[1] = open(
      file_name[1],
      O_CREAT |
          O_RDWR, // use O_APPEND incase if you want to append existing file
      S_IRGRP | S_IWGRP | S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH); // = 0666
  umask(o_umask); // reset to old
  printf("reset umask = %u\n", getumask());
  mode_t perm = 0444; // must be octal!!!
  print_file_permissions(file_name[1], S_IRWXU);
  if (!chmod(file_name[1], perm)) { // Give Read-Only permissions
    printf("Updated permissions: ");
    print_file_permissions(file_name[1], S_IRWXU);
    // we can use ret = access(path, <RWXF>_OK) to individual RWX permission, F:
    // file exists or not
  }
  getchar();

  char softlink_file[FILE_NAME_LEN];

  populate_file_name(softlink_file, FILE_NAME_LEN);
  // setup softlink to file_name[0]
  symlink(file_name[0], softlink_file);
  // It is pointer to original file, but it has differnet inode number
  // and permissions for that link; The changes to either in permissions
  // are synced; Shell: ln -s <src> <trg>
  if (access(softlink_file, F_OK) == -1) {
    printf("Symbolic link creation failed !\n");
  } else {
    print_file_permissions(softlink_file, S_IRWXU);
    print_file_permissions(file_name[0], S_IRWXU);
  }
  sleep(1);
  getchar();

  char hardlink_file[FILE_NAME_LEN];
  // setup hardlink to file_name[1]
  populate_file_name(hardlink_file, FILE_NAME_LEN);
  link(file_name[1], hardlink_file);

  if (access(hardlink_file, F_OK) == -1) {
    printf("Symbolic link creation failed !\n");
  } else {
    print_file_permissions(hardlink_file, S_IRWXU);
    print_file_permissions(file_name[1], S_IRWXU);
  }


  // for directory, we have 
  // mkdir(path, mode): create dir
  // rmdir(path)      : del dir
  // getcwd(buff, size): PWD in buff
  // chdir(path)      : cd
  // opendir, closedir: needs <dirent.h>

  DIR *PDIR = NULL, *TMP = NULL;
  TMP = PDIR = opendir("./..");
  if(!PDIR) {
    printf("Can't open parent dir\n");
    exit(1);
  }

  struct dirent *PDIR_DETAILS = readdir(PDIR);
  printf("Details of PDIR\n");

  printf("d_ino: %d\n", PDIR_DETAILS->d_ino);
  printf("d_name: %s\n", PDIR_DETAILS->d_name);
  printf("d_off: %ld\n", PDIR_DETAILS->d_off);
  printf("d_reclen: %d\n", PDIR_DETAILS->d_reclen);
  printf("d_type: %c\n", PDIR_DETAILS->d_type);
  
  
  printf("Listing all the files in the %s\n", PDIR_DETAILS->d_name);

  while((PDIR_DETAILS = readdir(PDIR)) != NULL)
    printf("%s\n", PDIR_DETAILS->d_name);



  if(closedir(TMP)) {
    printf("Failed to close dir: %s\n", PDIR_DETAILS->d_name);
  }


  close(fd[0]);
  close(fd[1]);
  // delete the files (remove() calls this incase of file)
  // shell: unlink <trg>; unlinks the softlink
  if (!(!unlink(file_name[0]) && !unlink(file_name[1]) &&
      !unlink(softlink_file) && !unlink(hardlink_file))) {
    printf("unlink/ delete failed!\n");
  }

  return 0;
}