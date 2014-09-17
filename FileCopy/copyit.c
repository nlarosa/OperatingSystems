//
// Nicholas LaRosa
// CSE 30341, Project 1
// 
// Usage: copyit <file_source> <file_dest>
//

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER 4096
#define DEBUG 0

void displayMessage( int s )
{
	alarm( 1 );
	printf("copyit: still copying...\n");
}

int main( int argc, char *argv[] )
{	
	alarm( 1 );												// first second set
	signal( SIGALRM, displayMessage );						// prepare alarm to set off signal

	printf( "\n" );

	if( argc != 3 )
	{
		printf( "copyit: incorrect argument number\n" );
		printf( "copyit: usage: copyit <file_orig> <file_dest>\n\n" );
		exit( 1 );
	}

	char buffer[ BUFFER ];					// 4 KB transfer at a time	

	int origFile, destFile;					// origin and destination file descriptors
	int bytesRead, bytesWritten;

	char * origFileName = argv[1];
	char * destFileName = argv[2];

	origFile = open( origFileName, O_RDONLY, 0 );
	
	if( origFile < 0 )
	{
		printf( "copyit: unable to open file %s: %s\n\n", origFileName, strerror( errno ) );
		exit( 1 );
	}

	destFile = open( destFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );	// open file for writing, set permissions to -rw-r--r--

	if( destFile < 0 )
	{
		printf( "copyit: unable to open file %s: %s\n\n", destFileName, strerror( errno ) );
		exit(1);
	}

	while( 1 )
	{
		if( DEBUG ) printf( "Reading...\n" );
		bytesRead = read( origFile, buffer, BUFFER );		// read up to 4KB
		if( DEBUG ) printf( "Done reading...\n" );

		if( bytesRead < 0 )
		{
			if( errno == EINTR )							// read interrupted, continue
			{
				continue;
			}
			else											// actual bad error
			{
				printf( "copyit: error reading from %s: %s\n\n", origFileName, strerror( errno ) );
				exit( 1 );
			}
		}
		else if( bytesRead == 0 )							// no more file to read
		{
			break;											// done with while loop
		}

		if( DEBUG ) printf( "%d bytes read...\n", bytesRead );
		bytesWritten = write( destFile, buffer, bytesRead );

		if( bytesWritten < 0 )
		{
			if( errno == EINTR )							// write interrupted
			{
				continue;
			}
			else											// actual bad error
			{
				printf( "copyit: error writing to %s: %s\n\n", destFileName, strerror( errno ) );
				exit( 1 );
			}
		}
		else if( bytesWritten != bytesRead )				// some bytes missing
		{
			printf( "copyit: error writing to %s: Bytes lost in transit\n\n", destFileName );
			exit( 1 );
		}

		if( DEBUG ) printf( "%d bytes written...\n", bytesWritten );
	}

	if( close( origFile ) < 0 )
	{
		printf( "copyit: error closing %s: %s\n\n", origFileName, strerror( errno ) );
		exit( 1 );
	}

	if( close( destFile ) < 0 )
	{
		printf( "copyit: error closing %s: %s\n\n", destFileName, strerror( errno ) );
		exit( 1 );
	}

	printf( "copyit: write complete!\n\n" );

	return 0;
}

