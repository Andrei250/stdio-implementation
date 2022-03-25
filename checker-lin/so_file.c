#include "so_stdio.h"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define BUFFSIZ 4096 // as specified in the problem statement

typedef struct _so_file {
    int fp;
    int flags;

    char buffer[BUFFSIZ];

} SO_FILE;


SO_FILE *structInit() {
    SO_FILE *str = malloc(sizeof(struct _so_file));

    return str;
}

void freeStructure(SO_FILE *structure) {
    free(structure);
}

FUNC_DECL_PREFIX SO_FILE *so_fopen(const char *pathname, const char *mode) {
    SO_FILE *openedFile = structInit();
    int fp;
    int flags;

    if (strcmp(mode, "r") == 0) {
        flags = O_RDONLY;
    } else if (strcmp(mode, "r+")) {
        flags = O_RDWR;
    } else if (strcmp(mode, "w")) {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(mode, "w+")) {
        flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (strcmp(mode, "a")) {
        flags = O_WRONLY | O_APPEND | O_CREAT;
    } else if (strcmp(mode, "a+")) {
        flags = O_RDWR | O_APPEND | O_CREAT;
    }

    fp = open(pathname, flags, 0644);

    if (fp < 0) {
        freeStructure(openedFile);
        return NULL;
    }

    if (strcmp(mode, "a") || strcmp(mode, "a+")) {
        lseek(fp, SEEK_SET, SEEK_END);
    }

    openedFile->fp = fp;
    openedFile->flags = flags;

    return openedFile;
}

FUNC_DECL_PREFIX int so_fclose(SO_FILE *stream) {
    int rc = close(stream->fp);

    if (rc < 0) {
        return SO_EOF;
    }

    freeStructure(stream);

    return 0;
}

#if defined(__linux__)
FUNC_DECL_PREFIX int so_fileno(SO_FILE *stream) {
    return stream->fp;
}
#elif defined(_WIN32)
FUNC_DECL_PREFIX HANDLE so_fileno(SO_FILE *stream) {
    return stream->fp;
}
#endif


