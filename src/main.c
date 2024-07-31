/*
 * This compiler compiles a language know as Funky.
 * That is all.
 */
#include "allheaders.h"

#define DEBUG 1
/*
 */
void print_usage(char **argv) {
  printf("Usage: %s <path_to_source>\n", argv[0]);
}
/*
 * TODO implement ignore comments
 */
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

void free_tokens(Token *root) {
  while (root) {
    Token *token_free = root;
    root = root->next;
    free(token_free);
  }
}

void print_token(Token node) {
  printf("%.*s", node.end - node.beginning, node.beginning);
}
/*
 */
void print_tokens(Token *root) {
  if(DEBUG){
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
  struct Node *children;
  struct Node *next_child;
} Node;

int node_compare(Node *a, Node *b){
  if(!a || !b) {
    if(!a && !b){
      return 1;
    }
    return 0;
  }
  assert(NODE_TYPE_MAX == 3 && "Node compare must handle all types");
  if(a->type != b->type) {
    return 0;
  }
  switch(a->type) {
    case NODE_TYPE_NONE:
      if(nonep(*b)){
        return 1;
      }
      return 0;
      break;
    case NODE_TYPE_INTEGER:
      if(a->value.integer == b->value.integer) {
        return 1;
      }
      return 0;
      break;
    case NODE_TYPE_PROGRAM:
      break;
    default:
      break;
  }
  return 0;
}

void print_node_impl(Node *node){
  assert(NODE_TYPE_MAX == 3 && "Print node must handle all types");
  switch (node->type){
    default:
      printf("UNKNOWN NODE");
    case NODE_TYPE_NONE:
      printf("NONE");
      break;
    case NODE_TYPE_INTEGER:
      printf(" INT: %lld", node->value.integer);
      break;
    case NODE_TYPE_PROGRAM:
      printf("PROGRAM");
      break;
  }
}

void print_node(Node *node, size_t indent_lvl){
  if(!node) {return;}
  for (size_t i = 0; i < indent_lvl; ++i) {
    putchar(' ');
  }
  print_node_impl(node);
  putchar('\n');
  Node *child = node->children;
  while(child) {
    for (size_t i = 0; i < indent_lvl; ++i) {
      putchar(' ');
    }
    printf("|-- ");
    print_node(child, indent_lvl + 4);
    child = child->next_child;
  }
}

void free_node(Node *root) {
  if(!root){ return; }
  Node *child = root->children;
  Node *next_child = NULL;
  while(child){
    next_child = child->next_child;
    free_node(child);
    child = next_child;
  }
  free(root);
}

/*
 */
typedef struct Binding {
  Node *id;
  Node *value;
  struct Binding *next;
} Binding;
/*
 */
typedef struct Environment {
  struct Environment *parent;
  Binding *bind;
} Environment;

Environment *env_create(Environment *parent) {
  Environment *env = malloc(sizeof(Environment));
  assert(env && "Could not allocate Environment");
  env->parent = parent;
  env->bind = NULL;
  return env;
}

void env_set(Environment env, Node id, Node val) {
  Binding *binding = malloc(sizeof(Binding));
  assert(binding && "Could not allocate binding");
  binding->id = &id;
  binding->value = &val;
  binding->next = env.bind;
  env.bind = binding;
}
Node env_get(Environment env, Node id){
  Binding *bind_it = env.bind;
  while(bind_it) {
    if(node_compare(bind_it->id, &id)){
      return *bind_it->value;
    }
    bind_it = bind_it->next;
  }
  Node val;
  val.type = NODE_TYPE_NONE;
  val.children = NULL;
  val.next_child = NULL;
  val.value.integer = 0;

  return val;
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
  // get to beginning of token beyond whitespace
  token->beginning += strspn(token->beginning, whitespace);
  token->end = token->beginning;
  if (*(token->end) == '\0'){ return err; }
  // send the cursor to the end of this token
  token->end += strcspn(token->beginning, delimiter);
  // shift end if something went wrong
  if(token->end == token->beginning) {
    token->end += 1;
  }

  return err;
}
/*
 */
int token_string_equalp(char *string, Token *token) {
  if(!string || !token){
    return 0;
  }
  char *beg = token->beginning;
  while(*string && token->beginning < token->end){
    if(*string != *beg){
      return 0;
    }
    string++;
    beg++;
  }
  return 1;
}
/*
 */
int parse_integer(Token *token, Node *node){
  if(!token || !node) {return 0;}
  if (token->end - token->beginning == 1 && *(token->beginning) == '0') {
      node->type = NODE_TYPE_INTEGER;
      node->value.integer = 0;
  } else if ((node->value.integer = strtoll(token->beginning, NULL, 10)) != 0) {
      node->type = NODE_TYPE_INTEGER;
  } else {
    return 0;
  }
  return 1;
}
/*
 */
Error parse_source(char *source, Node *environment) {
  Error err = ok;
  size_t token_count = 0;
  Token *tokens = NULL;
  Token *tokens_iter = tokens;
  Token current_token;
  current_token.next = NULL;
  current_token.beginning = source;
  current_token.end = source;
  Node *root = calloc(1, sizeof(Node));
  assert(root && "Could not allocate Root Node.");
  root->type = NODE_TYPE_PROGRAM;
  Node working_node;
  // This is cursed C.. beware
  while ((err = lex(current_token.end, &current_token)).type == ERROR_NONE) {
    working_node.children = NULL;
    working_node.next_child = NULL;
    working_node.type = NODE_TYPE_NONE;
    working_node.value.integer = 0;
    size_t token_length = current_token.end - current_token.beginning; 
    if (token_length == 0) { break; }
    if(parse_integer(&current_token, &working_node)) {
      Token integer;
      memcpy(&integer, &current_token, sizeof(Token));
      if (err.type != ERROR_NONE) return err;
    } else {
      printf("Unrecognised Token: ");
      print_token(current_token);
      putchar('\n');
    }
    print_node(&working_node, 0);
    putchar('\n');
  }
  free_tokens(tokens);
  free_node(root);
  return err;
}
/*
 * Go Baby, Go!
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
