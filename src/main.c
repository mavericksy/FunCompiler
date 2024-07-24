#include "allheaders.h"
#include <string.h>

const char *comment = "#";
const char *whitespace = " \r\n";
const char *delimiter = " \n\r";

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
  size_t bytes_read = 0;
  while (bytes_read < size) {
    printf("Reading %ld bytes\n", size - bytes_read);
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
    printf("read: %zu, size: %zu", bytes_read, size);
    free(contents);
    return NULL;
  }
  contents[bytes_read] = '\0';
  return contents;
}
/*
 *
 */
typedef struct Error {
  enum ErrorType {
    ERROR_NONE = 0,
    ERROR_ARGUMENTS,
    ERROR_TYPE,
    ERROR_SYNTAX,
    ERROR_GENERIC,
    ERROR_TODO,
  } type;
  char *message;
} Error;

Error ok = {ERROR_NONE, NULL};

void print_error(Error err) {
  if (err.type == ERROR_NONE) {
    return;
  }
  printf("ERROR: ");
  switch (err.type) {
  case ERROR_TODO:
    printf("TODO (not implemented)");
    break;
  case ERROR_ARGUMENTS:
    printf("Invalid arguments");
    break;
  case ERROR_SYNTAX:
    printf("Invalid Syntax");
    break;
  case ERROR_TYPE:
    printf("Mismatched types");
    break;
  case ERROR_GENERIC:
    printf("ERROR");
    break;
  default:
    printf("Unknown error type...");
    break;
  }
  putchar('\n');
  if (err.message) {
    printf("     : %s", err.message);
  }
}

#define ERROR_CREATE(n, t, msg) Error(n) = {(t), (msg)}
#define ERROR_PREP(n, t, msg)                                                  \
  (n).type = (t);                                                              \
  (n).message = (msg)

Error lex(char *source, char **beg, char **end) {
  Error err = ok;
  if (!source || !beg || !end) {
    ERROR_PREP(err, ERROR_ARGUMENTS, "Source cannot be empty");
    return err;
  }
  //
  *beg = source;
  *beg += strspn(*beg, whitespace);
  *end = *beg;
  *end += strcspn(*beg, delimiter);
  return err;
}

Error parse_source(char *source) {
  Error err = ok;
  char *beg = source;
  char *end = source;
  // This is cursed C.. beware
  while ((err = lex(end, &beg, &end)).type == ERROR_NONE) {
    if (end - beg == 0) {
      break;
    }
    printf("lexed: %.*s\n", end - beg, beg);
  }
  return err;
}

/*
 *
 */
int main(int argc, char **argv) {
  Error err = ok;
  if (argc < 2) {
    print_usage(argv);
    exit(1);
  }
  char *path = argv[1];
  char *contents = file_contents(path);
  if (contents) {
    // printf("Contents of %s\n---\n%s---\n", path, contents);
    Error err = parse_source(contents);
    free(contents);
  }
  print_error(err);
  return 0;
}
