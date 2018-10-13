#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEBUG 0

const int SYMBOL_STRING = 2;
const int SYMBOL_SET = 4;
const int SYMBOL_CHOICE = 8;

typedef struct {
	int type;
	int elements;
	void *data; // a cstring a symbol_t array
} symbol_t;


// --- Symbol Manipulation
symbol_t* new_symbol(int type);
symbol_t* new_symbol_string(char *str);
void      append_to_set(symbol_t *set, symbol_t *new_element);
symbol_t* flatten_set_of_strings(symbol_t *stringy_set);
int       is_set_of_strings(symbol_t *sym);

void      symbol_print(symbol_t *s, int indent_level);

// --- Parsing
symbol_t* parse_symbol(char *source, int *start);
int       find_closing_brace(char *source, int start);
int       find_closing_apostrophe(char *source, int start);

// --- Symbol Reduction
void      reduce_symbol_into(symbol_t *root, char **buffer, int *bsize);
char*     reduce_symbol(symbol_t *root);


char* load_file(char *fname) {
	FILE *f = fopen(fname, "r");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = 0;
	return string;
}

symbol_t* new_symbol(int type) {
	symbol_t *s = malloc(sizeof(symbol_t));
	s->type = type;
	s->elements = 0;
	s->data = NULL;
	return s;
}

symbol_t* new_symbol_string(char *str) {
	symbol_t *s = new_symbol(SYMBOL_STRING);
	s->data = malloc(strlen(str) + 1);
	strcpy(s->data, str);
	return s;
}

void symbol_print(symbol_t *s, int indent_level) {
	for(int i=0; i<indent_level; i+=1)
		printf("\t");
	
	printf("Symbol(");
	if( s->type & (SYMBOL_SET | SYMBOL_CHOICE) ) {
		printf("%s, %d elements)\n", (s->type == SYMBOL_SET) ? "SET" : "CHOICE", s->elements);
		
		for(int i=0; i<s->elements; i+=1){
			symbol_t *new_s = &((symbol_t*) s->data)[i];
			symbol_print(new_s, indent_level + 1);
		}
		
	} else if ( s->type == SYMBOL_STRING ) {
		printf("STRING, strlen=%li, \"%s\")\n", strlen((char*) s->data), (char*) s->data);
		
	}
}

void append_to_set(symbol_t *set, symbol_t *new_element) {
	if( set->type == SYMBOL_STRING ) {
		printf("ATTEMPTED TO APPEND TO NON-SET\n");
		return;
	}
	
	set->elements += 1;
	set->data = realloc(set->data, sizeof(symbol_t) * set->elements);
	((symbol_t*) set->data)[set->elements-1] = *new_element;
}

int is_set_of_strings(symbol_t *sym) {
	if( sym->type == SYMBOL_STRING) return 0; // symbol isn't a set or a choice
	if( sym->elements == 0 ) {
		printf("Zero length set\n");
		return 0;
	}
	
	symbol_t *data = (symbol_t*) sym->data;
	
	for( int i=0; i<sym->elements; i+=1 ){
		if( data[i].type != SYMBOL_STRING ) return 0; // contains a non-string symbol
	}
	
	return 1;
}

symbol_t* flatten_set_of_strings(symbol_t *stringy_set) {
	int total_length = 0;
	symbol_t* sym_array = (symbol_t*) stringy_set->data;
	
	for(int i=0; i<stringy_set->elements; i+=1){
		total_length += strlen(sym_array[i].data);
	}
	
	char* full_string = malloc(total_length + 1);
	int position = 0;
	
	for(int i=0; i<stringy_set->elements; i+=1){
		strcpy(full_string + position, sym_array[i].data);
		position += strlen(sym_array[i].data);
	}
	
	symbol_t *output = new_symbol_string(full_string);
	free(full_string);
	return output;
}

int find_closing_brace(char *source, int start) {
	int position = start;
	int counter = 0;
	
	do {
		if(source[position] == '{') counter += 1;
		if(source[position] == '}') counter -= 1;
		position += 1;
		
	} while(counter != 0);
	
	return position - 1;
}

int find_closing_apostrophe(char *source, int start) {
	if(source[start] != '"') {
		printf("STARTING APOSTROPHE NOT IDENTIFIED");
	}
	
	do{
		start += 1;
	} while(source[start] != '"');
	
	return start;
}

symbol_t* parse_symbol(char *source, int *start) {
	if(source[*start] == '{') {
		
		int closing = find_closing_brace(source, *start);
		*start += 1;
		symbol_t *set = new_symbol(SYMBOL_SET);
		
		while(*start != closing) {
			if(source[*start] == '"' || source[*start] == '{') {
				symbol_t *sub_symbol = parse_symbol(source, start);
				append_to_set(set, sub_symbol);
			}
			
			if(source[*start] == '|'){
				set->type = SYMBOL_CHOICE;
			}
			
			*start += 1;
		}
		
		return set;
		
	}else if(source[*start] == '"') {
		
		int closing = find_closing_apostrophe(source, *start);
		
		// -1 and +1 to get the string within the apostrophes
		symbol_t *string = new_symbol(SYMBOL_STRING);
		string->data = malloc(closing - *start);
		memcpy(string->data, source+*start+1, closing-*start-1);
		((char*) string->data)[closing-*start-1] = 0;
		
		*start = closing;
		
		return string;
		
	}else{
		printf("Failed to parse symbol: %c\n", source[*start]);
		return NULL;
	}
}

// Recursively traverses the input on a random path, adding what it finds to the buffer
void reduce_symbol_into(symbol_t *root, char **buffer, int *bsize) {
	if(root->type == SYMBOL_STRING) {
		int data_len = strlen(root->data);
		*buffer = realloc(*buffer, *bsize + data_len + 1);
		strcpy( *buffer + *bsize, root->data);
		*bsize += data_len;
		
	}else if(root->type == SYMBOL_SET){
		for(int i=0; i<root->elements; i+=1){
			reduce_symbol_into( &((symbol_t*) root->data)[i], buffer, bsize );
		}
		
	}else if(root->type == SYMBOL_CHOICE){
		int selected = rand() % root->elements;
		reduce_symbol_into( &((symbol_t*) root->data)[selected], buffer, bsize );
		
	}
}

char* reduce_symbol(symbol_t *root) {
	char *str = malloc(1);
	*str = 0;
	int bsize = 0;
	reduce_symbol_into(root, &str, &bsize);
	
	return str;
}


int main(int argc, char **argv) {
	if(argc < 2 || argc > 3){
		printf("Usage: %s <input file> [number of times to run]\n", argv[0]);
		return 1;
	}
	srand(time(0));
	
	char *source = load_file(argv[1]);
	int start = 0;
	symbol_t *root = parse_symbol(source, &start);
	if(DEBUG) symbol_print(root, 0);

	int repeat = 1;
	if(argc == 3) repeat = atoi(argv[2]);
	
	for(int i=0; i<repeat; i+=1){
		char* red = reduce_symbol(root);
		printf("%s\n\n", red);
		free(red);
	}
	
	return 0;
}
