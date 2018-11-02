#include "helper.h"


char* integer_to_string(char*string,int x) {
	string = malloc(10);
	if (string) {
		sprintf(string, "%d", x);
	}
	string=realloc(string,strlen(string)+1);
	return string; // caller is expected to invoke free() on this buffer to release memory
}

size_t getFileSize(const char* filename) {
    struct stat st;
    if(stat(filename, &st) != 0) {
        return 0;
    }
    return st.st_size;
}
