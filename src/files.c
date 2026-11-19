#include "files.h"
#include <stdio.h>
#include <assert.h>


void get_file(const wchar_t *filename, FileMap *fm, char mode, bool open_w_nullterm) {
    assert(mode == 'r' || mode == 'w');

    DWORD file_flags;
    // file_flags = mode == 'r' ? GENERIC_READ : GENERIC_WRITE;
    // if (open_w_nullterm) { file_flags |= GENERIC_WRITE; }
    file_flags = GENERIC_READ | GENERIC_WRITE;

    HANDLE fd = CreateFileW(
        filename,
        file_flags,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (fd == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        wprintf(L"Could not open file. Error code: 0x%X\n", dwError);
        return;
    }
    
    LARGE_INTEGER filesize;
    GetFileSizeEx(fd, &filesize);

    char *map = NULL;
    if (open_w_nullterm) { 
        filesize.QuadPart++;
        map = malloc(filesize.QuadPart);
        map[filesize.QuadPart-1] = '\0';
    }

    DWORD read;
    ReadFile(fd, map, filesize.QuadPart, &read, NULL);

    fm->fd = fd; fm->map = map; fm->mapsize = filesize.QuadPart;
}



void close_file(FileMap *fm) {
    free(fm->map);
    CloseHandle(fm->fd);
}