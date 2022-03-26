#include "so_stdio.h"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define BUFFSIZ 4096 // as specified in the problem statement

#define NO_OPERATION 0
#define WRITE_OPERATION 1
#define READ_OPERATION 2

typedef struct _so_file {
    int fp;
    int flags;
    int error;
    int eof;
    int index;
    int sz;

    char buffer[BUFFSIZ];
    char op;
} SO_FILE;


SO_FILE *structInit() {
    SO_FILE *str = malloc(sizeof(struct _so_file));
    
    str->eof = 0;
    str->error = 0;
    str->op = NO_OPERATION;
    str->sz = BUFFSIZ;
    str->index = 0;

    return str;
}

void freeStructure(SO_FILE *structure) {
    free(structure);
}

void resetBuffer(SO_FILE *structure) {
    memset(structure->buffer, 0, BUFFSIZ);
    structure->index = 0;
    structure->sz = BUFFSIZ;
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

FUNC_DECL_PREFIX int so_fflush(SO_FILE *stream) {
    if (stream->op != WRITE_OPERATION) {
        return SO_EOF;
    }

    int written = 0, totalWritten = 0;

    while (totalWritten < stream->index) {
        written = write (stream->fp,
            stream->buffer + totalWritten,
            stream->index - totalWritten);

        if (written < 0) {
            stream->error = SO_EOF;
            return SO_EOF;
        }

        totalWritten += written;
    }

    resetBuffer(stream);

    return 0;
}

FUNC_DECL_PREFIX int so_fseek(SO_FILE *stream, long offset, int whence) {
    if (stream->op == READ_OPERATION) {
        resetBuffer(stream);
    } else if (stream->op == WRITE_OPERATION) {
        so_fflush(stream);
    }

    int rc = lseek(stream->fp, offset, whence);

    if (rc < 0) {
        return SO_EOF;
    }

    return 0;
}

FUNC_DECL_PREFIX long so_ftell(SO_FILE *stream) {
    return 0;
}

FUNC_DECL_PREFIX int so_feof(SO_FILE *stream) {
    return stream->eof;
}

FUNC_DECL_PREFIX int so_ferror(SO_FILE *stream) {
    return stream->error;
}

FUNC_DECL_PREFIX int so_fgetc(SO_FILE *stream) {
    stream->op = READ_OPERATION;

    if (stream->index > 0 &&
        stream->index < stream->sz) { // if we still have chars in buffer
        return (int) ((unsigned char) stream->buffer[stream->index++]);
    }

    stream->sz = read(stream->fp, stream->buffer, BUFFSIZ);

    if (stream->sz <= 0) {
        stream->eof = 1;

        if (stream->sz == 0) {
            stream->error = SO_EOF;
        }

        return SO_EOF;
    }

    stream->index = 0;

    return (int) ((unsigned char) stream->buffer[stream->index++]);
}

FUNC_DECL_PREFIX int so_fputc(int c, SO_FILE *stream) {
    stream->op = WRITE_OPERATION;

    return SO_EOF;
}

