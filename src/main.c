/*
 * This compiler compiles a language know as Funky.
 * That is all.
 */
#include "allheaders.h"
/*
 *
*/
#define DEBUG 1
/*
 *
 */
#define nonep(node) ((node).type == NODE_TYPE_NONE)
#define integerp(node) ((node).type == NODE_TYPE_INTEGER)
#define symbolp(node) ((node).type == NODE_TYPE_SYMBOL)
/*
 *
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
const char *newline = "\n\r";
/*
 *
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
 *
 */
char *file_contents(char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    printf("Could not open file at %s\n", path);
    return NULL;
  }
  long size = file_size(file);
  char *contents = malloc(size + 1);
  assert(contents && "Could not allocate buffer Contents");
  size_t bytes_read = 0;
  // FIXME how to compare long to size_t safely
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
  // FIXME how to compare long to size_t safely
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
typedef struct Token {
  char *beginning;
  char *end;
} Token;
/*
 *
 */
Token *token_create() {
  Token *token = malloc(sizeof(Token));
  assert(token && "Could not allocate Token");
  memset(token, 0, sizeof(Token));
  return token;
}
/*
 *
 */
void print_token(Token node) {
  printf("%.*s", node.end - node.beginning, node.beginning);
}
/*
 *
 */
typedef struct Node {
  enum NodeType {
    NODE_TYPE_NONE = 0,
    NODE_TYPE_INTEGER,
    NODE_TYPE_SYMBOL,
    NODE_TYPE_VAR_DECLARATION,
    NODE_TYPE_VAR_DECLARATION_INIT,
    NODE_TYPE_PROGRAM,
    NODE_TYPE_BINARY_OP,
    NODE_TYPE_MAX,
  } type;
  union NodeValue {
    long long integer;
    char *symbol;
  } value;
  struct Node *children;
  struct Node *next_child;
} Node;
/*
 *
 */
void node_add_child(Node *parent, Node *new_child) {
  if(!parent || !new_child) return;
  Node *alloc_child = malloc(sizeof(Node));
  assert(alloc_child && "Cannot allocate for new child");
  *alloc_child = *new_child;
  if(parent->children) {
    Node *child = parent->children;
    while(child->next_child){
      child = child->next_child;
    }
    child->next_child = alloc_child;
  } else {
    parent->children = alloc_child;
  }
}
/*
 *
 */
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
/*
 *
 */
void print_node_impl(Node *node){
  assert(NODE_TYPE_MAX == 7 && "Print node must handle all types");
  switch (node->type){
    default:
      printf("UNKNOWN NODE\n");
      break;
    case NODE_TYPE_NONE:
      printf("NONE");
      break;
    case NODE_TYPE_INTEGER:
      printf("INT:%lld", node->value.integer);
      break;
    case NODE_TYPE_SYMBOL:
      if(node->value.symbol)
        printf("SYM:%s", node->value.symbol);
      break;
    case NODE_TYPE_BINARY_OP:
      break;
    case NODE_TYPE_VAR_DECLARATION:
      printf("VAR DCL");
      break;
    case NODE_TYPE_VAR_DECLARATION_INIT:
      break;
    case NODE_TYPE_PROGRAM:
      printf("PROGRAM");
      break;
  }
}
/*
 *
 */
void print_node(Node *node, size_t indent_lvl){
  if(!node) {return;}
  for (size_t i = 0; i < indent_lvl; ++i) {
    putchar(' ');
  }
  print_node_impl(node);
  putchar('\n');
  Node *child = node->children;
  while(child) {
    print_node(child, indent_lvl + 4);
    child = child->next_child;
  }
}
/*
 *
 */
void free_node(Node *root) {
  if(!root){ return; }
  Node *child = root->children;
  Node *next_child = NULL;
  while(child){
    next_child = child->next_child;
    free_node(child);
    child = next_child;
  }
  if(symbolp(*root) && root->value.symbol) {
    free(root->value.symbol);
  }
  free(root);
}
/*
 *
 */
typedef struct Binding {
  Node *id;
  Node *value;
  struct Binding *next;
} Binding;
/*
 *
 */
typedef struct Environment {
  struct Environment *parent;
  Binding *bind;
} Environment;
/*
 *
 */
Environment *env_create(Environment *parent) {
  Environment *env = malloc(sizeof(Environment));
  assert(env && "Could not allocate Environment");
  env->parent = parent;
  env->bind = NULL;
  return env;
}
/*
 *
 */
void env_set(Environment env, Node id, Node val) {
  Binding *binding = malloc(sizeof(Binding));
  assert(binding && "Could not allocate binding");
  binding->id = &id;
  binding->value = &val;
  binding->next = env.bind;
  env.bind = binding;
}
/*
 *
 */
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
    ERROR_MAX,
  } type;
  char *message;
} Error;
/*
 *
 */
Error ok = {ERROR_NONE, NULL};
/*
 *
 */
void print_error(Error err) {
  if (err.type == ERROR_NONE) {
    return;
  }
  printf("ERROR: ");
  assert(ERROR_MAX == 6 && "print error must handle all errors");
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
 *
 */
#define ERROR_PREP(n, t, msg)                                                  \
  (n).type = (t);                                                              \
  (n).message = (msg)
/*
 *
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
Error ignoreComment(char *source, Token *token) {
  Error err = ok;
  if (!source || !token) {
    ERROR_PREP(err, ERROR_ARGUMENTS, "Source cannot be empty");
    return err;
  }
  token->beginning = source;
  // get to beginning of token beyond whitespace
  token->end = token->beginning;
  if (*(token->end) == '\0'){ return err; }
  // send the cursor to the end of this token
  token->end += strcspn(token->beginning, newline);
  token->end += 1;
  return err;
}
/*
 *
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
 *
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
 *
 */
Error parse_source(Environment *env, char *source, char **end, Node *result) {
  Error err = ok;
  Token current_token;
  current_token.beginning = source;
  current_token.end = source;
  // This is cursed C.. beware
  while ((err = lex(current_token.end, &current_token)).type == ERROR_NONE) {
    size_t token_length = current_token.end - current_token.beginning;
    if (token_length == 0) { break; }
    *end = current_token.end;
    //
    if(parse_integer(&current_token, result)) {
      Node lhs_int = *result;
      printf("Integer: ");
      print_token(current_token);
      putchar('\n');

      continue;

    } else {
      //
      //
      if(token_string_equalp("=", &current_token)) continue;
      if(token_string_equalp("#", &current_token)) {
        err = ignoreComment(current_token.end, &current_token);
        if (err.type != ERROR_NONE)
          return err;
        continue;
      }

      Node symbol;
      symbol.type = NODE_TYPE_SYMBOL;
      symbol.children = NULL;
      symbol.next_child = NULL;
      symbol.value.symbol = NULL;

      char *symbol_string = malloc(token_length + 1);
      assert(symbol_string && "Could not allocate for symbol");
      memcpy(symbol_string, current_token.beginning, token_length);
      symbol_string[token_length] = '\0';
      symbol.value.symbol = symbol_string;

      *result = symbol;

      printf("Symbol: ");
      print_token(current_token);
      putchar('\n');
      //
      err = lex(current_token.end, &current_token);
      if(err.type != ERROR_NONE)
        return err;
      //
      *end = current_token.end;
      size_t token_length = current_token.end - current_token.beginning;
      if (token_length == 0) { break; }
      //
      if(token_string_equalp(":", &current_token)) {
        err = lex(current_token.end, &current_token);
        if(err.type != ERROR_NONE)
          return err;
        *end = current_token.end;
        size_t token_length = current_token.end - current_token.beginning;
        if (token_length == 0) { break; }
        //
        if (token_string_equalp("integer", &current_token)) {

          err = lex(current_token.end, &current_token);
          if (err.type != ERROR_NONE)
            return err;
          *end = current_token.end;
          size_t token_length = current_token.end - current_token.beginning;
          if (token_length == 0) {
            break;
          }

          err = lex(current_token.end, &current_token);
          if (err.type != ERROR_NONE)
            return err;
          *end = current_token.end;
          token_length = current_token.end - current_token.beginning;
          if (token_length == 0) {
            break;
          }

          printf("Type Integer: ");
          print_token(current_token);
          putchar('\n');

          Node var_decl;
          var_decl.children = NULL;
          var_decl.next_child = NULL;
          var_decl.type = NODE_TYPE_VAR_DECLARATION;

          Node type_node;
          memset(&type_node, 0, sizeof(Node));
          type_node.type = NODE_TYPE_INTEGER;
          type_node.value.integer = current_token.end - current_token.beginning;

          node_add_child(&var_decl, &type_node);
          node_add_child(&var_decl, &symbol);

          *result = var_decl;
          //
          return ok;
        }
      }
      printf("Unhandled: ");
      print_token(current_token);
      putchar('\n');
    }
  }
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
    Environment *env = env_create(NULL);
    Node expression;
    memset(&expression, 0, sizeof(Node));
    char *contents_iter = contents;
    Error err = parse_source(env, contents, &contents_iter, &expression);
    //
    print_node(&expression, 0);
    print_error(err);
    free(contents);
  }
  return 0;
}
