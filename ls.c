#include	<stdlib.h>
#include	<stdio.h>
#include  <string.h>
#include  <sys/stat.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <dirent.h>
#include  <grp.h>
#include  <time.h>
#include  <limits.h>
#include  <pwd.h>

//COLORS
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
#define RESET       "\033[0m"

//MODES
int modeDisplaySymbol = 0;
int modeExceptDot = 0;
int modeFileTime = 0;
int modeFormat = 0;
int modeIncludeDot = 0;
int modeNonPrintableCharacter = 0;
int modeNumberOfBlocks = 0;
int modePlainFile = 0;
int modeRecursive = 0;
int modeReverse = 0;
int modeSerialNumber = 0;
int modeSize = 0;
int modeSort = 0;

struct stat elemStat;

int permissions[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};    //known permissions

const char *all_permissions = "rwxrwxrwx";    //permissions as letters

const char *file_permissions(int mode)    //parsing file permissions
{
  static char result[20];
  int i;
  for (i=0; i<9; i++){
    if (mode & permissions[i]){
      result[i] = all_permissions[i];
    }
    else{
      result[i] = '-';    //if file does not have current permission
    }
  }
  result[10] = '\0';

  return result;
}

void printColor(int mode, char* dir_name){    //printing filenames in specific colors
  if (S_ISDIR(mode))                    printf(BOLDBLUE    "%s" RESET, dir_name);
  else if(access(dir_name, X_OK) != -1) printf(BOLDGREEN   "%s" RESET, dir_name);
  else if (S_ISLNK(mode))               printf(BOLDCYAN    "%s" RESET, dir_name);
  else if (S_ISSOCK(mode))              printf(BOLDMAGENTA "%s" RESET, dir_name);
  else if ( S_ISFIFO (mode))            printf("%s", dir_name);
  else if (S_ISREG (mode))              printf("%s", dir_name);
}

void printList(char* entries[], int n){   //printing file list
  int mode;
  int i;
  int total;
  for(i = 0; i < n; i++){
    lstat(entries[i], &elemStat);   //stat info for each element (file)
    if ((!strcmp(entries[i], ".") || !strcmp(entries[i], "..")) && (modeExceptDot == 1 || (modeIncludeDot == 0))){
      ;
    }
    else{
      long int links        = (long)elemStat.st_nlink;
      struct passwd *uid    = getpwuid((long)elemStat.st_uid);  //user struct from given ID
      struct group *gid     = getgrgid((long)elemStat.st_gid);  //group struct from given ID
      long file_size        = elemStat.st_size;                 //file size
      struct tm *time_stamp = localtime(&elemStat.st_mtime);    //for parsing date and time
      char buffer[80];
      strftime(buffer, 10, "%b", time_stamp);   //date and time parsing

      if(modeFormat == 0){
        printColor(elemStat.st_mode, entries[i]);
        printf("  ");
      }
      if(modeFormat == 2 || modeFormat == 3){
        if(modeSerialNumber == 1){
          printf("%ld ", elemStat.st_ino);    //if we need to print inode number of file
        }
        printf("%s %2ld", file_permissions(elemStat.st_mode), links);
        if(modeFormat == 2){
          printf(" %4s %4s ", uid->pw_name, gid->gr_name);    //if we need to print the name of user and the name of group
        }
        else if(modeFormat == 3){
          printf(" %d %d ", elemStat.st_uid, elemStat.st_gid);    //if we need to print the ID of user and the ID of group
        }
        printf("%5ld %s %2d %2d:%2d ", file_size, buffer, time_stamp->tm_mday, time_stamp->tm_hour, time_stamp->tm_min);    //parse and print date and time
        printColor(elemStat.st_mode, entries[i]);   //colorful filenames
        printf("\n");
      }
    }
  }
  printf("\n");
}

int sortByName (const struct dirent **d1, const struct dirent **d2){
  if (modeReverse == 0) return (strcasecmp((*d1)->d_name, (*d2)->d_name));    //if reverse mode is off
  if (modeReverse == 1) return (!strcasecmp((*d1)->d_name, (*d2)->d_name));   //if reverse mode is on
}

int sortByTime(const struct dirent **d1, const struct dirent **d2){
  struct stat stat1, stat2;

  stat((*d1)->d_name, &stat1);
  stat((*d2)->d_name, &stat2);

  time_t ts1, ts2;

  if(modeFileTime == 0)       ts1 = stat1.st_mtime, ts2 = stat2.st_mtime;   //compare last modified times
  else if(modeFileTime == 1)  ts1 = stat1.st_ctime, ts2 = stat2.st_ctime;   //compare last status change times
  else if(modeFileTime == 2)  ts1 = stat1.st_atime, ts2 = stat2.st_atime;   //compare last access times

  if(ts1 == ts2){
    return strcasecmp((*d1)->d_name,(*d2)->d_name);
  }
  else if(ts1 < ts2){
    if (modeReverse == 0) return 1;     //if reverse mode is off
    if (modeReverse == 1) return -1;    //if reverse mode is on
  }
  else{
    if (modeReverse == 0) return -1;    //if reverse mode is off
    if (modeReverse == 1) return 1;     //if reverse mode is on
  }
}

int sortBySize(const struct dirent **d1, const struct dirent **d2){
  struct stat stat1, stat2;
  lstat((*d1)->d_name, &stat1);
  lstat((*d2)->d_name, &stat2);

  if(stat1.st_size == stat2.st_size){
    return (strcasecmp((*d1)->d_name, (*d2)->d_name));
  }
  else if(stat1.st_size < stat2.st_size){
    if (modeReverse == 0) return 1;     //if reverse mode is off
    if (modeReverse == 1) return -1;    //if reverse mode is on
  }
  else{
    if (modeReverse == 0) return -1;    //if reverse mode is off
    if (modeReverse == 1) return 1;     //if reverse mode is on
  }
}

int main(int argc, char* argv[]) {
  char *filename = NULL;
  struct stat dirStat;
  DIR *dir;
  struct dirent **dp;
  char* entries[256];
  int n = 0;

  int i;
  for (i = 1; i < argc; i++) {
    if(argv[i][0] == '-'){    //parse `ls` options
      int j;
      int len = strlen(argv[i]);
      for (j = 1; j < len; j++) {
        if(argv[i][j] == 'A') modeExceptDot = 1;
        if(argv[i][j] == 'a') modeIncludeDot = 1;
        if(argv[i][j] == 'C') modeFormat = 1;
        if(argv[i][j] == 'c') modeSort = 2, modeFileTime = 1;
        if(argv[i][j] == 'd') modePlainFile = 1;
        if(argv[i][j] == 'F') modeDisplaySymbol = 1;
        if(argv[i][j] == 'f') modeSort = 3;
        if(argv[i][j] == 'h') modeSize = 1;
        if(argv[i][j] == 'i') modeSerialNumber = 1;
        if(argv[i][j] == 'k') modeSize = 2;
        if(argv[i][j] == 'l') modeFormat = 2;
        if(argv[i][j] == 'n') modeFormat = 3;
        if(argv[i][j] == 'q') modeNonPrintableCharacter = 1;
        if(argv[i][j] == 'R') modeRecursive = 1;
        if(argv[i][j] == 'r') modeReverse = 1;
        if(argv[i][j] == 'S') modeSort = 1;
        if(argv[i][j] == 's') modeNumberOfBlocks = 1;
        if(argv[i][j] == 't') modeSort = 2;
        if(argv[i][j] == 'u') modeSort = 2, modeFileTime = 2;
        if(argv[i][j] == 'w') modeNonPrintableCharacter = 2;
        if(argv[i][j] == 'x') modeFormat = 4;
        if(argv[i][j] == '1') modeFormat = 5;
      }
    }
    else{
      filename = argv[i];   //get filename from argument
    }
  }

  if(filename == NULL) filename = ".";    //if filename is NULL, set current folder as default
  lstat(filename, &dirStat);    //get the stat info for filename

  i = 0;

  if(access(filename, F_OK) != -1){   //if it is accessible (actually, exists)
    if(S_ISDIR(dirStat.st_mode)){   //if our file is directory
      if(modeSort == 0){
        n = scandir(filename, &dp, 0, sortByName);
      }
      else if(modeSort == 1){
        n = scandir(filename, &dp, 0, sortBySize);
      }
      else if(modeSort == 2){
        n = scandir(filename, &dp, 0, sortByTime);
      }
      else if(modeSort == 3){
        n = scandir(filename, &dp, 0, NULL);
      }

      for(i = 0; i < n; i++){
        entries[i] = malloc(sizeof(dp[i]->d_name));
        strcpy(entries[i], dp[i]->d_name);
      }
    }
    else{   //for only one file (e.g. regular file)
      entries[0] = malloc(sizeof(filename));
      strcpy(entries[0], filename);
      n = 1;
    }
  }
  else{
    printf("ls: cannot access %s: No such file or directory\n", filename);    //if we cannot access the given file
  }

  /////////////////////////////////////////////////////////////////////////////

  printList(entries, n);    //print the List in Terminal as a final operation

  return 0;
}
