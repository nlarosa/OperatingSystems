//
// Nicholas LaRosa
// CSE 30341, Project 1 EC
// 
// Usage: copyit_extracredit <source> <dest>
//

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER 4096
#define DEBUG 0
#define SIZE 1024

void displayMessage( int s );
void copyFile( int origFile, int destFile );
void copyDirectory( char * origDirName, char * destDirName );

void displayMessage( int s )
{
	alarm( 1 );
	printf("copyit: still copying...\n");
}

void copyDirectory( char * origDirName, char * destDirName )	// copy all files/directories in a directory
{
	DIR * origDir = opendir( origDirName );
	DIR * destDir = opendir( destDirName );
	DIR * testDir;

	struct dirent * orig;										// struct to hold directory file names

	int origFile, destFile;

	char origFileName[ SIZE+1 ];
	char destFileName[ SIZE+1 ];
	char intermediate[ SIZE+1 ];							// intermediate string to hold concatenation

	if( origDir == NULL )
	{
		printf( "copyit_extracredit: cannot open origin directory %s\n\n", origDirName );
		exit( 1 );
	}
	else if( destDir == NULL )
	{
		printf( "copyit_extracredit: cannot open destination directory %s\n\n", destDirName );
		exit( 1 );
	}

	while( ( orig = readdir( origDir ) ) != NULL ) 
	{
		if( orig->d_name[0] != '.' )						// copy all files or recursively enter directories
		{
			strcpy( intermediate, orig->d_name );			// file to copy

			sprintf( destFileName, "%s%s", destDirName, intermediate );		// set destination to origin's relative name
			sprintf( origFileName, "%s%s", origDirName, intermediate );		// and create absolute path

			testDir = opendir( origFileName );		// test for directory

			if( testDir != NULL )					// another directory to copy
			{
				closedir( testDir );

				testDir = opendir( destFileName );	// test for directory

				if( origFileName[ strlen( origFileName ) - 1 ] != '/' )		// add forward dash for easier processing
				{
					sprintf( origFileName, "%s/", origFileName );
				}

				if( destFileName[ strlen( destFileName ) - 1 ] != '/' )		// add forward dash for easier processing
				{
					sprintf( destFileName, "%s/", destFileName );
				}

				if( testDir != NULL )				// another directory to copy to
				{
					closedir( testDir );
				}
				else
				{
					printf( "copyit_extracredit: creating new directory %s.\n", destFileName );
					destFile = mkdir( destFileName, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );		// creates a new directory to copy to
				}	
				
				copyDirectory( origFileName, destFileName );
			}
			else
			{
				origFile = open( origFileName, O_RDWR, 0 );		// enable writing to retrieve directory error (ie. if origin is directory)
				destFile = open( destFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );	// open file for writing, set permissions to -rw-r--r--

				if( origFile < 0 )
				{
					printf( "copyit_extracredit: unable to open file %s: %s\n\n", origFileName, strerror( errno ) );
					exit( 1 );
				}

				if( destFile < 0 )
				{
					printf( "copyit_extracredit: unable to open file %s: %s\n\n", destFileName, strerror( errno ) );
					exit(1);
				}

				printf( "copyit_extracredit: copying %s to %s.\n", origFileName, destFileName );
				copyFile( origFile, destFile );

				if( close( origFile ) < 0 )
				{
					printf( "copyit_extracredit: error closing %s: %s\n\n", origFileName, strerror( errno ) );
					exit( 1 );
				}

				if( close( destFile ) < 0 )
				{
					printf( "copyit_extracredit: error closing %s: %s\n\n", destFileName, strerror( errno ) );
					exit( 1 );
				}
			}
		}
	}

	if( closedir( origDir ) < 0 )
	{
		printf( "copyit_extracredit: error closing directory %s: %s\n\n", origDirName, strerror( errno ) );
		exit( 1 );
	}

	if( closedir( destDir ) < 0 )
	{
		printf( "copyit_extracredit: error closing directory %s: %s\n\n", destDirName, strerror( errno ) );
		exit( 1 );
	}
}

void copyFile( int origFile, int destFile )					// copy a single file
{
	char buffer[ BUFFER ];									// 4 KB transfer at a time	
	int bytesRead, bytesWritten;	

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
				printf( "copyit_extracredit: error reading: %s\n\n", strerror( errno ) );
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
				printf( "copyit_extracredit: error writing: %s\n\n", strerror( errno ) );
				exit( 1 );
			}
		}
		else if( bytesWritten != bytesRead )				// some bytes missing
		{
			printf( "copyit_extracredit: error writing: Bytes lost in transit\n\n" );
			exit( 1 );
		}

		if( DEBUG ) printf( "%d bytes written...\n", bytesWritten );
	}	
}

int main( int argc, char * argv[] )
{	
	alarm( 1 );												// first second set
	signal( SIGALRM, displayMessage );						// prepare alarm to set off signal

	printf( "\n" );

	if( argc != 3 )
	{
		printf( "copyit_extracredit: invalid argument number\n" ); 
		printf( "copyit_extracredit: usage: copyit_extracredit <file_orig> <file_dest>\n\n");
		exit( 1 );
	}

	int origFile, destFile;					// origin and destination file descriptors

	char origFileName[ SIZE+1 ];
	char destFileName[ SIZE+1 ];

	DIR * testDir;

	strcpy( origFileName, argv[1] );
	strcpy( destFileName, argv[2] );

	if( DEBUG ) printf( "First argument %s, second argument %s.\n", origFileName, destFileName );

	testDir = opendir( origFileName );		// test for directory

	if( testDir != NULL )					// user specified a directory to copy
	{
		if( DEBUG ) printf( "Origin file name is %d characters long.\n", (int) strlen( origFileName ) );
		
		if( origFileName[ strlen( origFileName ) - 1 ] != '/' )		// add forward dash for easier processing
		{
			sprintf( origFileName, "%s/", origFileName );
		}

		if( DEBUG ) printf( "Destination file name is %d characters long.\n", (int) strlen( destFileName ) );

		if( destFileName[ strlen( destFileName ) - 1 ] != '/' )		// add forward dash for easier processing
		{
			sprintf( destFileName, "%s/", destFileName );
		}

		testDir = opendir( destFileName );

		if( testDir != NULL )				// user specified a directory to copy to
		{
			copyDirectory( origFileName, destFileName );
		}
		else
		{
			printf( "copyit_extracredit: destination directory %s does not exist\n\n", destFileName );
			exit( 1 );
		}
	}
	else
	{
		origFile = open( origFileName, O_RDWR, 0 );		// enable writing to retrieve directory error (ie. if origin is directory)
		destFile = open( destFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );	// open file for writing, set permissions to -rw-r--r--

		if( origFile < 0 )
		{
			printf( "copyit_extracredit: unable to open file %s: %s\n\n", origFileName, strerror( errno ) );
			exit( 1 );
		}

		if( destFile < 0 )
		{
			printf( "copyit_extracredit: unable to open file %s: %s\n\n", destFileName, strerror( errno ) );
			exit( 1 );
		}

		copyFile( origFile, destFile );
	
		if( close( origFile ) < 0 )
		{
			printf( "copyit_extracredit: error closing %s: %s\n\n", origFileName, strerror( errno ) );
			exit( 1 );
		}
	
		if( close( destFile ) < 0 )
		{
			printf( "copyit_extracredit: error closing %s: %s\n\n", destFileName, strerror( errno ) );
			exit( 1 );
		}
	}
	
	printf( "copyit_extracredit: write complete!\n\n" );

	return 0;
}

