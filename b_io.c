/**************************************************************
 * Class::  CSC-415-0# Spring 2024
 * Name:: Mos Kullathon
 * Student ID:: 921425216
 * GitHub-Name:: mosguinz
 * Project:: Assignment 5 – Buffered I/O read
 *
 * File:: b_io.c
 *
 * Description:: This assignment involves writing a C program designed
 * 				 to copy a specified number of bytes from a provided file.
 *
 **************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>

#include "b_io.h"
#include "fsLowSmall.h"

#define MAXFCBS 20 // The maximum number of files open at one time

// This structure is all the information needed to maintain an open file
// It contains a pointer to a fileInfo strucutre and any other information
// that you need to maintain your open file.
typedef struct b_fcb
{
	fileInfo *fi; // holds the low level systems file info

	// Add any other needed variables here to track the individual open file
	b_io_fd fd;
	char *buffer;
	size_t start;
	size_t remaining;
	size_t block;
	size_t bytes_read;

} b_fcb;

// static array of file control blocks
b_fcb fcbArray[MAXFCBS];

// Indicates that the file control block array has not been initialized
int startup = 0;

// Method to initialize our file system / file control blocks
// Anything else that needs one time initialization can go in this routine
void b_init()
{
	if (startup)
		return; // already initialized

	// init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
	{
		fcbArray[i].fi = NULL; // indicates a free fcbArray
	}

	startup = 1;
}

// Method to get a free File Control Block FCB element
b_io_fd b_getFCB()
{
	for (int i = 0; i < MAXFCBS; i++)
	{
		if (fcbArray[i].fi == NULL)
		{
			fcbArray[i].fi = (fileInfo *)-2; // used but not assigned
			return i;						 // Not thread safe but okay for this project
		}
	}

	return (-1); // all in use
}

// b_open is called by the "user application" to open a file.  This routine is
// similar to the Linux open function.
// You will create your own file descriptor which is just an integer index into an
// array of file control blocks (fcbArray) that you maintain for each open file.
// For this assignment the flags will be read only and can be ignored.

b_io_fd b_open(char *filename, int flags)
{
	if (startup == 0)
		b_init(); // Initialize our system

	//*** TODO ***//  Write open function to return your file descriptor
	//				  You may want to allocate the buffer here as well
	//				  But make sure every file has its own buffer

	// This is where you are going to want to call GetFileInfo and b_getFCB
	fileInfo *fi = GetFileInfo(filename);
	int fd = b_getFCB();

	// Abort if file not found.
	if (fi == NULL)
		return -1;

	// Initialize FCB and place it into `fcbArray`.
	b_fcb fcb;
	fcb.fi = fi;
	fcb.fd = fd;
	fcb.buffer = malloc(B_CHUNK_SIZE);
	fcb.remaining = B_CHUNK_SIZE;
	fcb.start = 0;
	fcb.bytes_read = 0;
	fcb.block = LBAread(fcb.buffer, 1, fi->location);
	fcbArray[fd] = fcb;

	return fd;
}

// b_read functions just like its Linux counterpart read.  The user passes in
// the file descriptor (index into fcbArray), a buffer where thay want you to
// place the data, and a count of how many bytes they want from the file.
// The return value is the number of bytes you have copied into their buffer.
// The return value can never be greater then the requested count, but it can
// be less only when you have run out of bytes to read.  i.e. End of File
int b_read(b_io_fd fd, char *buffer, int count)
{
	//*** TODO ***//
	// Write buffered read function to return the data and # bytes read
	// You must use LBAread and you must buffer the data in B_CHUNK_SIZE byte chunks.

	if (startup == 0)
		b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); // invalid file descriptor
	}

	// and check that the specified FCB is actually in use
	if (fcbArray[fd].fi == NULL) // File not open for this descriptor
	{
		return -1;
	}

	// Your Read code here - the only function you call to get data is LBAread.
	// Track which byte in the buffer you are at, and which block in the file
	int total_read = 0;

	// Adjust count if reading beyond end of file.
	if (fcbArray[fd].bytes_read + count >= fcbArray[fd].fi->fileSize)
	{
		count = fcbArray[fd].fi->fileSize - fcbArray[fd].bytes_read;
	}

	// Read from buffer.
	while (total_read < count && fcbArray[fd].remaining)
	{
		int to_copy = (count - total_read < fcbArray[fd].remaining)
						  ? count - total_read
						  : fcbArray[fd].remaining;

		// Copy the remaining bytes into the buffer and update positions.
		memcpy(buffer + total_read, fcbArray[fd].buffer + fcbArray[fd].start, to_copy);
		total_read += to_copy;
		fcbArray[fd].start += to_copy;
		fcbArray[fd].remaining -= to_copy;

		// If there are no remaining space in our buffer, get a new block.
		if (fcbArray[fd].remaining == 0)
		{
			fcbArray[fd].start = 0;
			fcbArray[fd].block += LBAread(fcbArray[fd].buffer, 1, fcbArray[fd].block);
			fcbArray[fd].remaining = B_CHUNK_SIZE;
		}
	}

	// Update number of bytes read.
	fcbArray[fd].bytes_read += total_read;
	return total_read;
}

// b_close frees and allocated memory and places the file control block back
// into the unused pool of file control blocks.
int b_close(b_io_fd fd)
{
	//*** TODO ***//  Release any resources
	if (fcbArray[fd].buffer)
		free(fcbArray[fd].buffer);
	fcbArray[fd].fi = NULL;
}
