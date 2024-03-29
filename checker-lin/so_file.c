#include "so_stdio.h"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFSIZ 4096 // as specified in the problem statement

// operations type ( read or write )
#define NO_OPERATION 0
#define WRITE_OPERATION 1
#define READ_OPERATION 2

typedef struct _so_file {
	int fp, error, eof, index, sz, parent;
	char buffer[BUFFSIZ];
	char op;
} SO_FILE;

// Helper function to initialise a SO_FILE structure
SO_FILE *structInit(void)
{
	SO_FILE *str = malloc(sizeof(struct _so_file));

	str->eof = 0;
	str->error = 0;
	str->op = NO_OPERATION;
	str->sz = BUFFSIZ;
	str->index = 0;
	memset(str->buffer, 0, BUFFSIZ);

	return str;
}

// free strcture
void freeStructure(SO_FILE *structure)
{
	free(structure);
}

// reset the buffer
void resetBuffer(SO_FILE *structure)
{
	memset(structure->buffer, 0, BUFFSIZ);
	structure->index = 0;
	structure->sz = BUFFSIZ;
}

// open file.
// Setting flags depending on mode and then opening file and setting
// file pointer.
FUNC_DECL_PREFIX SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	SO_FILE *openedFile = structInit();
	int fp;
	int flags;

	if (strcmp(mode, "r") == 0)
		flags = O_RDONLY;
	else if (strcmp(mode, "r+") == 0)
		flags = O_RDWR;
	else if (strcmp(mode, "w") == 0)
		flags = O_WRONLY | O_CREAT | O_TRUNC;
	else if (strcmp(mode, "w+") == 0)
		flags = O_RDWR | O_CREAT | O_TRUNC;
	else if (strcmp(mode, "a") == 0)
		flags = O_WRONLY | O_APPEND | O_CREAT;
	else if (strcmp(mode, "a+") == 0)
		flags = O_RDWR | O_APPEND | O_CREAT;
	else {
		freeStructure(openedFile);
		return NULL;
	}

	fp = open(pathname, flags, 0644);

	if (fp < 0) {
		freeStructure(openedFile);
		return NULL;
	}

	openedFile->fp = fp;

	return openedFile;
}

// if the last operation is write, the status
// depends on flushing the buffer.
// otherwise  return the status of closing.
FUNC_DECL_PREFIX int so_fclose(SO_FILE *stream)
{
	int writing;

	if (stream->op == WRITE_OPERATION) {
		writing = so_fflush(stream);

		if (writing == SO_EOF) {
			freeStructure(stream);
			return SO_EOF;
		}
	}

	int rc = close(stream->fp);

	freeStructure(stream);
	return rc;
}

FUNC_DECL_PREFIX int so_fileno(SO_FILE *stream)
{
	return stream->fp;
}

// if the last oepration is not write then return SO_EOF
// write everything in file.
// using while loop to write for error checking ( writing everything )
FUNC_DECL_PREFIX int so_fflush(SO_FILE *stream)
{
	if (stream->op != WRITE_OPERATION)
		return SO_EOF;

	int written = 0, totalWritten = 0;

	while (totalWritten != stream->index) {
		written = write(stream->fp,
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

// if it is a read operation then we just need to reset the buffer.
// otherwise we need to write then to reset ( flush )
FUNC_DECL_PREFIX int so_fseek(SO_FILE *stream, long offset, int whence)
{
	if (stream->op == READ_OPERATION)
		resetBuffer(stream);
	else if (stream->op == WRITE_OPERATION)
		so_fflush(stream);

	int rc = lseek(stream->fp, offset, whence);

	if (rc < 0)
		return SO_EOF;

	return 0;
}

// if last operation was read the answer is currentPos - size of the buffer
// + the read elements until now.
// otherwise the answer is currentPos + what we have written.
FUNC_DECL_PREFIX long so_ftell(SO_FILE *stream)
{
	long currentPos = lseek(stream->fp, 0, SEEK_CUR);

	if (stream->op == READ_OPERATION)
		return currentPos - stream->sz + stream->index;
	else
		return currentPos + stream->index;

	return SO_EOF;
}

FUNC_DECL_PREFIX int so_feof(SO_FILE *stream)
{
	return stream->eof;
}

FUNC_DECL_PREFIX int so_ferror(SO_FILE *stream)
{
	return stream->error;
}

// read char by char.
// first of all, if the buffer is full or empty, we need to
// read from file.
// if we are sill reading from buffer, return the specific char.
// if the EOF is reached, update error status and eof flag.
FUNC_DECL_PREFIX int so_fgetc(SO_FILE *stream)
{
	stream->op = READ_OPERATION;

	// if we still have chars in buffer return next char
	if (stream->index > 0 &&
		stream->index < stream->sz) {
		return (int) ((unsigned char) stream->buffer[stream->index++]);
	}

	stream->sz = read(stream->fp, stream->buffer, BUFFSIZ);

	if (stream->sz <= 0) {
		stream->error = SO_EOF;

		if (stream->sz == 0)
			stream->eof = 1;

		return SO_EOF;
	}

	stream->index = 0;

	return (int) ((unsigned char) stream->buffer[stream->index++]);
}

// if buffer is full, flush it and the read into the new one.
FUNC_DECL_PREFIX int so_fputc(int c, SO_FILE *stream)
{
	stream->op = WRITE_OPERATION;

	if (stream->index == stream->sz && so_fflush(stream) == SO_EOF)
		return SO_EOF;

	stream->buffer[stream->index++] = c;

	return (int)((unsigned char) c);
}

// read char by char using fgetc
// if the EOF is reached, break the loop
FUNC_DECL_PREFIX
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	char *point = (char *) ptr;
	int it = 0, chr;

	for (; it < nmemb * size; ++it) { // reading char by char
		chr = so_fgetc(stream);

		if (chr == SO_EOF)
			break;

		point[it] = chr;
	}

	return it / size;
}

// same as above. write using fputc
FUNC_DECL_PREFIX
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	char *point = (char *) ptr;
	int it = 0, chr;

	for (; it < nmemb * size; ++it) { // reading char by char
		chr = so_fputc(point[it], stream);

		if (chr == SO_EOF)
			return 0;
	}

	return it / size;
}

// use pipes to communicate between processes.
// close pipes depending on the type specified and
// "redirected" the other one to STDIN/STDOUT
// on the parent process, use file descriptor as wanted pipe
FUNC_DECL_PREFIX SO_FILE *so_popen(const char *command, const char *type)
{
	SO_FILE *struc;
	pid_t pid;
	int err, pipes[2];
	const char *args[] = {"sh", "-c", command, NULL};

	err = pipe(pipes);

	if (err == SO_EOF)
		return NULL;

	pid = fork();

	if (pid == 0) {
		if (strcmp(type, "r") == 0) {
			close(pipes[0]);
			dup2(pipes[1], STDOUT_FILENO);
		} else if (strcmp(type, "w") == 0) {
			close(pipes[1]);
			dup2(pipes[0], STDIN_FILENO);
		}

		execvp("sh", (char * const*) args);
		exit(SO_EOF);
	} else if (pid > 0) {
		struc = structInit();
		int toClose = strcmp(type, "r") == 0 ? 1 : 0;

		struc->parent = pid;
		struc->fp = pipes[1 - toClose];
		close(pipes[toClose]);

		return struc;
	}

	return NULL;
}

// close stream and wait for the process to end.
FUNC_DECL_PREFIX int so_pclose(SO_FILE *stream)
{
	int status;
	int parent = stream->parent;

	so_fclose(stream);

	if (waitpid(parent, &status, 0) == SO_EOF)
		return SO_EOF;

	return status;
}
