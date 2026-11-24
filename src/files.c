#include "files.h"
#include <stdio.h>
#include <assert.h>


void get_file(const wchar_t *filename, FileMap *fm, char mode, bool open_w_nullterm) {
    assert(mode == 'r' || mode == 'w');

    DWORD file_flags = 0;
    DWORD open_flags = 0;

    if (mode == 'w') {
        open_flags |= CREATE_ALWAYS;
        file_flags |= GENERIC_WRITE;
    }
    else {
        open_flags |= OPEN_EXISTING;
        file_flags |= GENERIC_READ;
    }

    HANDLE fd = CreateFileW(
        filename,
        file_flags,
        FILE_SHARE_READ,
        NULL,
        open_flags,
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
    if (mode == 'r') {
        if (open_w_nullterm) { filesize.QuadPart++; }
        map = malloc(filesize.QuadPart);
        if (open_w_nullterm) { map[filesize.QuadPart-1] = '\0'; }

        DWORD read;
        ReadFile(fd, map, filesize.QuadPart, &read, NULL);
    }

    fm->fd = fd; fm->map = map; fm->mapsize = filesize.QuadPart;
}


void write_to_file(FileMap *file, char *data, int size) {
    DWORD bytes_written = 0;

    BOOL res = WriteFile(
        file->fd,
        data,
        (DWORD)size,
        &bytes_written,
        NULL
    );

    if (!res) {
        DWORD dwError = GetLastError();
        wprintf(L"Could not write to file. Error code: 0x%X\n", dwError);
        return;
    }
}


void close_file(FileMap *fm) {
    free(fm->map);
    CloseHandle(fm->fd);
}