/*
 * This compiler compiles a language know as Funky.
 * That is all.
 */
#include "allheaders.h"

const char *comment = "#";
const char *whitespace = " \r\n";
const char *delimiter = " \n\r,():";
/*
 */
#define nonep(node) ((node).type == NODE_TYPE_NONE)
#define integerp(node) ((node).type == NODE_TYPE_INTEGER)
/*
 */
typedef struct Token {
  char *beginning;
  char *end;
  struct Token *next;
} Token;
/*
 */
Token *token_create() {
  Token *token = malloc(sizeof(Token));
  assert(token && "Could not allocate Token");
  memset(token, 0, sizeof(Token));
  return token;
}
/*
 */
void print_tokens(Token *root) {
  size_t count = 1;
  while(root){
    // FIXME remove inf loop checker
    if(count > 10000) { break; }
    printf("Token %zu: ", count);
    if (root->beginning && root->end){
      printf("%.*s", root->end - root->beginning, root->beginning);
    }
    putchar('\n');
    root = root->next;
    count++;
  }
}
/*
 */
typedef long long integer_t;
/*
 */
typedef struct Node {
  enum NodeType {
    NODE_TYPE_NONE = 0,
    NODE_TYPE_INTEGER,
    NODE_TYPE_PROGRAM,
    NODE_TYPE_MAX,
  } type;
  union NodeValue {
    integer_t integer;
  } value;
  struct Node **children;
} Node;
/*
 */
typedef struct Binding {
  char *id;
  Node *value;
  struct Binding *next;
} Binding;
/*
 */
typedef struct Environment {
  struct Environment *parent;
  Binding *bind;
} Environment;
/*
 */
void print_usage(char **argv) {
  printf("Usage: %s <path_to_source>\n", argv[0]);
}
/*
 */
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
/*
 */
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
 */
typedef struct Error {
  enum ErrorType {
    ERROR_NONE = 0,
    ERROR_ARGUMENTS,
    ERROR_TYPE,
    ERROR_SYNTAX,
    ERROR_GENERIC,
    ERROR_TODO,
    ERROR_MAX,
  } type;
  char *message;
} Error;
/*
 */
Error ok = {ERROR_NONE, NULL};
/*
 */
void print_error(Error err) {
  if (err.type == ERROR_NONE) {
    return;
  }
  printf("ERROR: ");
  assert(ERROR_MAX == 6);
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
/*
 */
#define ERROR_PREP(n, t, msg)                                                  \
  (n).type = (t);                                                              \
  (n).message = (msg)
/*
 */
Error lex(char *source, Token *token) {
  Error err = ok;
  if (!source || !token) {
    ERROR_PREP(err, ERROR_ARGUMENTS, "Source cannot be empty");
    return err;
  }

  token->beginning = source;
  token->beginning += strspn(token->beginning, whitespace);
  token->end = token->beginning;
  if (*(token->end) == '\0'){ return err; }
  token->end += strcspn(token->beginning, delimiter);
  if(token->end == token->beginning) {
    token->end += 1;
  }

  return err;
}
/*
 */
Error parse_source(char *source, Node *environment) {
  Error err = ok;
  Token *tokens = NULL;
  Token *tokens_iter = tokens;

  Token current_token;
  current_token.next = NULL;
  current_token.beginning = source;
  current_token.end = source;

  // This is cursed C.. beware
  while ((err = lex(current_token.end, &current_token)).type == ERROR_NONE) {
    if (current_token.end - current_token.beginning == 0) {
      break;
    }
    // Prepend to linked list
    if(tokens){
      tokens_iter->next = token_create();
      memcpy(tokens_iter->next, &current_token, sizeof(Token));
      tokens_iter = tokens_iter->next;
    } else {
      tokens = token_create();
      memcpy(tokens, &current_token, sizeof(Token));
      tokens_iter = tokens;
    }
  }

  print_tokens(tokens);

  return err;
}
/*
 *
 * Go Baby, Go!
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
    Node env;
    Error err = parse_source(contents, &env);
    free(contents);
  }
  print_error(err);
  return 0;
}
