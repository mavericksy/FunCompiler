#include "allheaders.h"

void print_usage(char **argv) {
  printf("Usage: %s <path_to_source>\n", argv[0]);
}

long file_size(FILE *file) {
  if (!file) {
    return 0;
  }
  fpos_t orig;
  if (fgetpos(file, &orig) != 0) {
    printf("Could not get file position. fgetpos() failed: %i", errno);
    return 0;
  }
  fseek(file, 0, SEEK_END);
  long out = ftell(file);
  if (fsetpos(file, &orig) != 0) {
    printf("fsetpos() failed: %i", errno);
    return 0;
  }
  return out;
}

char *file_contents(char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    printf("Could not open file at %s\n", path);
    return NULL;
  }
  long size = file_size(file);
  char *contents = malloc(size + 1);
  char *write_it = contents;
  size_t bytes_read = 0;
  while (bytes_read < size) {
    size_t bytes_read_this_iter = fread(contents, 1, size - bytes_read, file);

    if (ferror(file)) {
      printf("Error while reading file: %i", errno);
      free(contents);
      return NULL;
    }

    bytes_read += bytes_read_this_iter;

    if (feof(file)) {
      break;
    }
  }
  if (bytes_read < size) {
    free(contents);
    return NULL;
  }
  contents[size] = '\0';
  return contents;
}
/*
 *
 */
int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv);
    exit(1);
  }
  char *path = argv[1];
  char *contents = file_contents(path);
  if (contents) {
    printf("Contents of %s\n---\n%s---\n", path, contents);
    free(contents);
  }
  return 0;
}
