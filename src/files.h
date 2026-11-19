#include <windows.h>



typedef struct {
    HANDLE fd;
    char* map;
    long long mapsize;
} FileMap;


void get_file(const wchar_t *filename, FileMap *fm, char mode, bool open_w_nullterm);

void close_file(FileMap *fm);