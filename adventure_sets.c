#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEBUG 0

const int SYMBOL_STRING = 10;
const int SYMBOL_SET = 12;
const int SYMBOL_CHOICE = 14;

typedef struct {
	int type; // STRING or SET or CHOICE
	int elements; // Number of elements in the set
	void *data; // a cstring if STRING or a symbol_t*
} symbol_t;

symbol_t* new_symbol(int type);
symbol_t* new_symbol_string(char *str);
void symbol_print(symbol_t *s, int indent_level);
void append_to_set(symbol_t *set, symbol_t *new_element);
int is_set_of_strings(symbol_t *sym);
symbol_t* flatten_set_of_strings(symbol_t *stringy_set);
symbol_t* reduce_choice(symbol_t* set);
int find_closing_brace(char* source, int start);
int find_closing_apostrophe(char *source, int start);

char* load_file(char *fname) {
	FILE *f = fopen(fname, "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  //same as rewind(f);

	char *string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = 0;
	return string;
}

symbol_t* new_symbol(int type) {
	symbol_t *s = (symbol_t*) malloc(sizeof(symbol_t));
	s->type = type;
	s->elements = 0;
	s->data = NULL;
	return s;
}

symbol_t* new_symbol_string(char *str) {
	symbol_t* s = new_symbol(SYMBOL_STRING);
	s->data = malloc(strlen(str) + 1);
	strcpy(s->data, str);
	return s;
}

void symbol_print(symbol_t *s, int indent_level) {
	for(int i=0; i<indent_level; i+=1)
		printf("\t");
	
	printf("Symbol(");
	if( s->type == SYMBOL_SET ) {
		printf("SET, %d elements, ", s->elements);
		
		set_process:
		printf(")\n");
		
		for(int i=0; i<s->elements; i+=1){
			symbol_t *new_s = ((symbol_t*)s->data) + i;
			symbol_print(new_s, indent_level + 1);
		}
		
		
	} else if ( s->type == SYMBOL_STRING ) {
		printf("STRING, strlen=%li, \"%s\", ", strlen((char*) s->data), (char*) s->data);
		printf(")\n");
		
	} else if ( s->type == SYMBOL_CHOICE ) {
		printf("CHOICE, %d elements", s->elements);
		goto set_process; // lol
		
	}
}

void append_to_set(symbol_t *set, symbol_t *new_element) {
	if( set->type == SYMBOL_STRING ) {
		printf("ATTEMPTED TO APPEND TO NON-SET\n");
		return;
	}
	
	set->elements += 1;
	
	symbol_t *new_data = (symbol_t*) malloc( sizeof(symbol_t) * set->elements );
	
	memcpy(new_data, set->data, sizeof(symbol_t) * (set->elements - 1));
	new_data[set->elements-1] = *new_element;
	if(set->data != NULL) free(set->data);
	set->data = new_data;
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
	
	char* full_string = (char*) malloc(total_length + 1);
	int position = 0;
	
	for(int i=0; i<stringy_set->elements; i+=1){
		strcpy(full_string + position, sym_array[i].data);
		position += strlen(sym_array[i].data);
	}
	
	symbol_t *output = new_symbol_string(full_string);
	return output;
}

symbol_t* reduce_choice(symbol_t* set) {
	if( set->elements == 0 ) {
		printf("Error - attempted to reduce an empty choice");
		return NULL;
	}
	
	symbol_t *array = (symbol_t*) set->data;
	return &array[rand() % set->elements];
}

int find_closing_brace(char* source, int start) {
	int position = start;
	int counter = 0;
	
	do {
		if(source[position] == '{') counter += 1;
		if(source[position] == '}') counter -= 1;
		position += 1;
		
	}while(counter != 0);
	
	return position - 1;
}

int find_closing_apostrophe(char *source, int start) {
	if(source[start] != '"') {
		printf("STARTING APOSTROPHE NOT IDENTIFIED");
	}
	
	do{
		start += 1;
	}while(source[start] != '"');
	
	return start;
}

symbol_t* prase_symbol(char* source, int *start) {
	if(source[*start] == '{') {
		
		int closing = find_closing_brace(source, *start);
		*start = *start + 1;
		symbol_t *set = new_symbol(SYMBOL_SET);
		
		while(*start != closing) {
			if(source[*start] == '"') {
				symbol_t *string = prase_symbol(source, start);
				append_to_set(set, string);
			}
			
			if(source[*start] == '{') {
				symbol_t *subset = prase_symbol(source, start);
				append_to_set(set, subset);
			}
			
			if(source[*start] == '|'){
				set->type = SYMBOL_CHOICE;
			}
			
			*start += 1;
		}
		
		return set;
		
	}else if(source[*start] == '"') {
		
		int closing = find_closing_apostrophe(source, *start);
		
		char *temp_string = malloc(closing - *start);
		temp_string[closing-*start-1] = 0;
		memcpy(temp_string, source+*start+1, closing-*start-1);
		
		symbol_t *string = new_symbol_string(temp_string);
		free(temp_string);
		*start = closing;
		
		return string;
		
	}else{
		printf("Failed to parse symbol: %c\n", source[*start]);
		return NULL;
	}
}

/* REDUCTION */

symbol_t* reduce_symbol(symbol_t *root) {
	if(root->type == SYMBOL_STRING){
		printf("Symbol is already a string\n");
		return root;
	}
	
	if(DEBUG) {
		printf("reducing type: ");
		symbol_print(root, 0);
	}

	symbol_t* children = (symbol_t*) root->data;

	while( !is_set_of_strings(root) ) {
		for(int i=0; i<root->elements; i+=1){
			
			symbol_t *child = &children[i];
			if( child->type != SYMBOL_STRING )
				*child = *reduce_symbol(child);
		}
	}
	
	
	if( root->type == SYMBOL_SET ){
		return flatten_set_of_strings(root);
	}else if( root->type == SYMBOL_CHOICE ){
		return &((symbol_t*) root->data)[rand() % root->elements];
	}
	
	return NULL;
}


int main(int argc, char **argv) {
	if(argc != 2){
		printf("Usage: %s <input file>\n", argv[0]);
		return 1;
	}
	srand(time(0));
	
	char *source = load_file(argv[1]);
	int start = 0;
	symbol_t *root = prase_symbol(source, &start);
	if(DEBUG) symbol_print(root, 0);
	
	symbol_t *red = reduce_symbol(root);
	if(DEBUG) symbol_print(red, 0);
	
	if(DEBUG) printf("\n\nFinal output:\n");
	printf("%s\n", (char*) red->data);

	return 0;
}
