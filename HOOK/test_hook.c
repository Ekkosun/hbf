#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef DEBUG
#define DEBUG_PRINT LIBC_FPRINTF
#else
#define DEBUG_PRINT(...)
#endif /* ifdef DEBUG */

#define MAP_SIZE (1 << 16)
#define MAX_READ_INFO 1024
/*
 * The macro to store information to shared memory
 *
 */
#define INSERT_INFO_BLOCK(data, a, b, c)                                       \
  do {                                                                         \
    int count = (data)[0];                                                     \
    if (count > MAX_READ_INFO) {                                               \
      count = 0;                                                               \
      (data)[0] = count;                                                       \
      DEBUG_PRINT(LOG_FILE, "Out of memory\n");                                \
    }                                                                          \
    (data)[count * 3 + 1] = (a);                                               \
    (data)[count * 3 + 2] = (b);                                               \
    (data)[count * 3 + 3] = (c);                                               \
    (data)[0]++;                                                               \
  } while (0)

void get_origin_function() __attribute__((constructor));
static FILE *LOG_FILE = NULL;

// the env to get shared memory
static char *SHM_ENV_VAR = "__AFL_SHM_ID";
// the shared memory pointer
static int *SHM_READ_INFO = NULL;
// env contians the name of file pass to target program
static char *FILE_NAME_ENV = "AFL_OUTPUT_FILE";
static char *FILE_NAME = NULL;
static int TEST_FILE = 0;
typedef int (*open_ptr)(const char *, int, ...);
typedef int (*openat_ptr)(int, const char *, int, ...);
typedef FILE *(*fopen_ptr)(const char *, const char *);
typedef FILE *(*fopen64_ptr)(const char *, const char *);
typedef ssize_t (*read_ptr)(int, void *, size_t);
typedef size_t (*fread_ptr)(void *, size_t, size_t, FILE *);
typedef int (*close_ptr)(int);
typedef int (*fclose_ptr)(FILE *);
typedef int (*fprintf_ptr)(FILE *, const char *, ...);
typedef int (*printf_ptr)(const char *, ...);
typedef int (*write_ptr)(int, const void *, int);

// the static origin function point
static open_ptr GLIBC_OPEN = NULL;
static openat_ptr GLIBC_OPENAT = NULL;
static fopen_ptr LIBC_FOPEN = NULL;
static fopen64_ptr LIBC_FOPEN64 = NULL;
static read_ptr GLIBC_READ = NULL;
static fread_ptr LIBC_FREAD = NULL;
static close_ptr GLIBC_CLOSE = NULL;
static fclose_ptr LIBC_FCLOSE = NULL;
static fprintf_ptr LIBC_FPRINTF = NULL;
static printf_ptr LIBC_PRINTF = NULL;
static write_ptr GLIBC_WRITE = NULL;
// local errno
static int ERRNO = 1;

/*
 * load init funciton
 */
void get_origin_function() {

  GLIBC_OPEN = (open_ptr)dlsym(RTLD_NEXT, "open");
  GLIBC_OPENAT = (openat_ptr)dlsym(RTLD_NEXT, "openat");
  LIBC_FOPEN = (fopen_ptr)dlsym(RTLD_NEXT, "fopen");
  LIBC_FOPEN64 = (fopen64_ptr)dlsym(RTLD_NEXT, "fopen64");
  GLIBC_READ = (read_ptr)dlsym(RTLD_NEXT, "read");
  LIBC_FREAD = (fread_ptr)dlsym(RTLD_NEXT, "fread");
  GLIBC_CLOSE = (close_ptr)dlsym(RTLD_NEXT, "close");
  LIBC_FCLOSE = (fclose_ptr)dlsym(RTLD_NEXT, "fclose");
  LIBC_FPRINTF = (fprintf_ptr)dlsym(RTLD_NEXT, "fprintf");
  LIBC_PRINTF = (printf_ptr)dlsym(RTLD_NEXT, "printf");
  GLIBC_WRITE = (write_ptr)dlsym(RTLD_NEXT, "write");
  usleep(10); // FIX: why need add usleep here , when  i don't add it , it
              // cause segmentation fault
}

/*
 * create the log file
 */
static int init_log() {
  if (LOG_FILE == NULL) {
    // load the origin function of `fopen`
    LOG_FILE = LIBC_FOPEN("./log.txt", "a+");
  }

  if (NULL == LOG_FILE)
    return 1;
  return 0;
}
/*
 * close the log file
 *
 */
static int finish_log() {
  if (LOG_FILE != NULL) {
    LIBC_FCLOSE(LOG_FILE);
  }
  LOG_FILE = NULL;
  return 0;
}
/*
 * get shared memory and map it to SHM
 *
 */
static int init_shm() {
  SHM_READ_INFO = NULL;

  char *shm_id = getenv(SHM_ENV_VAR);
  if (shm_id == NULL)
    return 1;
  DEBUG_PRINT(LOG_FILE, "shm_id:%s\n", shm_id);
  void *shm_ptr = shmat(atoi(shm_id), NULL, 0);
  shm_ptr = (char *)shm_ptr + MAP_SIZE;
  *(int *)shm_ptr = 0;
  SHM_READ_INFO = (int *)shm_ptr;
  return 0;
}
/*
 *
 * finish shm
 */
static int finish_shm() {
  SHM_READ_INFO = NULL;
  return 0;
}
/*
 * get the file name
 *
 */
static int init_file_name() {
  FILE_NAME = getenv(FILE_NAME_ENV);
  if (FILE_NAME == NULL) {
    return 1;
  }
  DEBUG_PRINT(LOG_FILE, "%s\n", FILE_NAME);
  return 0;
}
/*
 *
 *finish file name
 */
static int finish_file_name() {
  FILE_NAME = NULL;
  TEST_FILE = 0;
  return 0;
}

/*
 * the init process of read inormation.
 */
static int init() {
  int stage_errno = 0;
  // init log process
  stage_errno = 0 ; //init_log();
  if (0 != stage_errno) {
    ERRNO = 1;
    DEBUG_PRINT(LOG_FILE, "init stage:%d errno:%d\n", 1, stage_errno);
    goto end_init;
  }
  // init shm process
  stage_errno = init_shm();
  if (0 != stage_errno) {
    ERRNO = 2;
    DEBUG_PRINT(LOG_FILE, "init stage:%d errno:%d\n", 2, stage_errno);
    goto end_init;
  }

  // init file name process
  stage_errno = init_file_name();
  if (0 != stage_errno) {
    ERRNO = 3;
    DEBUG_PRINT(LOG_FILE, "init stage:%d errno:%d\n", 3, stage_errno);
    goto end_init;
  }
  ERRNO = 0;
end_init:
  return 0;
}

/*
 * is the  same file by fd
 */
static int is_same(int file0, int file1) {
  if (file0 == 0 || file1 == 0)
    return 0;
  struct stat stat0, stat1;
  if (fstat(file0, &stat0) < 0)
    return -1;
  if (fstat(file1, &stat1) < 0)
    return -1;

  return (stat0.st_dev == stat1.st_dev) && (stat0.st_ino == stat1.st_ino);
}

/*
 * The function to get the file size by FILE structure
 */
inline static long int get_file_size_file(FILE *stream) {
  int fd = fileno(stream);
  struct stat file_stat;
  if (fstat(fd, &file_stat) == -1)
    DEBUG_PRINT(LOG_FILE, "Can't find fstat\n");
  return file_stat.st_size;
}

/*
 * the function to get the file size by fd
 */
inline static long int get_file_size_fd(int fd) {
  struct stat file_stat;
  if (fstat(fd, &file_stat) == -1)
    DEBUG_PRINT(LOG_FILE, "Can't find fstat\n");
  return file_stat.st_size;
}

/*
 * A hook of `fopen()` function to log the opened file name.
 */
FILE *fopen(const char *filename, const char *mode) {

  // init
  init();

  FILE *stream = LIBC_FOPEN(filename, mode);

  if (ERRNO)
    goto init_failed;
  if (strstr(FILE_NAME, filename) || strstr(filename, FILE_NAME)) {
    TEST_FILE = fileno(stream);
  }
  // DEBUG_PRINT(LOG_FILE, "--- %ld \n", file_size);

init_failed:
  return stream;
}
/*
 * A hook of `fopen64()` function to log the opened file name.
 */
FILE *fopen64(const char *filename, const char *mode) {

  // init
  init();

  FILE *stream = LIBC_FOPEN64(filename, mode);

  if (ERRNO)
    goto init_failed;
  if (strstr(FILE_NAME, filename) || strstr(filename, FILE_NAME)) {
    TEST_FILE = fileno(stream);
  }
  // DEBUG_PRINT(LOG_FILE, "--- %ld \n", file_size);

init_failed:
  return stream;
}

/*
 * A hook of `open()` function to log the opened file name.
 */
int open(const char *filename, int flags, ...) {

  init();
  DEBUG_PRINT(LOG_FILE, "Use open series\n");
  // handel variable argument
  mode_t mode = 0;
  va_list arg;
  va_start(arg, flags);
  if (flags & O_CREAT) {
    mode = va_arg(arg, mode_t);
  }
  va_end(arg);

  int fd = GLIBC_OPEN(filename, flags, mode);

  if (ERRNO)
    goto init_failed;
  if (strstr(FILE_NAME, filename) || strstr(filename, FILE_NAME)) {
    TEST_FILE = fd;
  }

  // DEBUG_PRINT(LOG_FILE, "--- %ld \n", file_size);
init_failed:
  return fd;
}

/*
 * A hook of `openat()` function to log the opened file name.
 */
int openat(int dirfd, const char *filename, int flags, ...) {

  init();
  DEBUG_PRINT(LOG_FILE, "Use openat series\n");
  // handel variable argument
  mode_t mode = 0;
  va_list arg;
  va_start(arg, flags);
  if (flags & O_CREAT) {
    mode = va_arg(arg, mode_t);
  }
  va_end(arg);

  int fd = GLIBC_OPENAT(dirfd, filename, flags, mode);

  if (ERRNO)
    goto init_failed;
  if (strstr(FILE_NAME, filename) || strstr(filename, FILE_NAME)) {
    TEST_FILE = fd;
  }

  // DEBUG_PRINT(LOG_FILE, "--- %ld \n", file_size);
init_failed:
  return fd;
}

/*
 * A hook of `fread()` function to log the pos and length the
 * program read.
 */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {

  long int cur_pos = ftell(stream);

  size_t read_length = LIBC_FREAD(ptr, size, nmemb, stream);

  if (ERRNO)
    goto init_failed;
  if (SHM_READ_INFO && is_same(fileno(stream), TEST_FILE) == 1) {
    INSERT_INFO_BLOCK(SHM_READ_INFO, cur_pos, read_length, size * nmemb);

    DEBUG_PRINT(LOG_FILE, "%ld %ld %ld\n", cur_pos, read_length * size,
                size * nmemb);
  }

init_failed:
  return read_length;
}

/*
 * A hook of `read()` funtion to log the length the program write
 */
ssize_t read(int fd, void *buf, size_t count) {

  long int cur_pos = lseek(fd, 0, SEEK_CUR);
  ssize_t read_length = GLIBC_READ(fd, buf, count);

  if (ERRNO)
    goto init_failed;
  if (SHM_READ_INFO && is_same(fd, TEST_FILE) == 1) {
    INSERT_INFO_BLOCK(SHM_READ_INFO, cur_pos, read_length, count);
    DEBUG_PRINT(LOG_FILE, "%ld %ld %ld\n", cur_pos, read_length, count);
  }
  DEBUG_PRINT(LOG_FILE, "read\n");
init_failed:
  return read_length;
}

/*
 * A hook of `fclose()` function to end log.
 */
int fclose(FILE *stream) {
  if (ERRNO)
    goto init_failed;
  if (is_same(TEST_FILE, fileno(stream)) == 1) {
    DEBUG_PRINT(LOG_FILE, "finish\n");
    finish_log();
    finish_shm();
    finish_file_name();
  }

init_failed:
  return LIBC_FCLOSE(stream);
}

/*
 * A hook of `close()` function to log the length the program write.
 */
int close(int fd) {
  if (ERRNO)
    goto init_failed;
  if (is_same(TEST_FILE, fd) == 1) {

    DEBUG_PRINT(LOG_FILE, "finish\n");
    finish_log();
    finish_shm();
    finish_file_name();
  }

init_failed:
  return GLIBC_CLOSE(fd);
}