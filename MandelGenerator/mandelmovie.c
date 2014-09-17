//
// Nicholas LaRosa
// CSE30341 Project 3
//
// usage: ./mandelmovie <processes>
//

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ARG_X "0.286795"				// mandel arguments
#define ARG_Y "0.01430"
#define INIT_S 2.00000000000000
#define FINAL_S .000000001
#define ARG_M "2000"
#define ARG_W "750"
#define ARG_H "750"

#define MANDEL_PATH "./mandel_old"
#define FRAMES 50

void newProcess( char ** argList, int frameNumber );
int completeProcess( void );

int main( int argc, char *argv[] )
{
	if( argc != 2 )						// mandelmovie <processes>
	{
		printf( "Usage: ./mandelmovie <max_running_processes>\n" );
		exit( 0 );
	}

	int pid;

	int activeProcess = 0;				// number of running processes
	int compProcess = 0;				// number of completed processes
	int currProcess = 1;
	int maxProcess = atoi( argv[ 1 ] );	// max running processes

	if( maxProcess > FRAMES )
	{
		maxProcess = FRAMES;
	}
	else if( maxProcess <= 0 )
	{
		maxProcess = 1;
	}

	double currS = INIT_S;										// initial s value
	double sInc = pow( FINAL_S / INIT_S, 1.0 / FRAMES );		// and increment multiplier ( frame-th root of init/final )

	char * argList[ 16 ];				// 14 arguments passed to mandel
										// with two varying (-s and -o)
	char sValue[ 15 ];
	char fileName[ 15 ];
	
	argList[ 0 ] = MANDEL_PATH;

	argList[ 1 ] = "-x";
	argList[ 2 ] = ARG_X;
										// establish consistent arguments
	argList[ 3 ] = "-y";
	argList[ 4 ] = ARG_Y;

	argList[ 5 ] = "-s";
	argList[ 6 ] = sValue;				// reserve 15 bytes for s value
	
	argList[ 7 ] = "-m";
	argList[ 8 ] = ARG_M;

	argList[ 9 ] = "-W";
	argList[ 10 ] = ARG_W;

	argList[ 11 ] = "-H";
	argList[ 12 ] = ARG_H;

	argList[ 13 ] = "-o";
	argList[ 14 ] = fileName;			// and 15 bytes for filename

	//printf( "%lf\n", sInc );
	//printf( "%d\n", maxProcess );

	while( currProcess <= FRAMES )		// more frames to be generated!
	{
		if( activeProcess < maxProcess )		// room for another process
		{
			sprintf( argList[ 6 ], "%1.9lf", currS );					// update mandel arguments
			sprintf( argList[ 14 ], "/tmp/mandel%d.bmp", currProcess );
			argList[ 15 ] = ( char * ) NULL;

			printf( "%1.12lf\n", currS );

			//printf( "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", argList[0], argList[1], argList[2], argList[3], argList[4], argList[5], argList[6], argList[7], argList[8], argList[9], argList[10], argList[11], argList[12], argList[13], argList[14] );

			pid = fork();						// prepare for new process

			if( pid == 0 )						// child - begin new process
			{
				newProcess( ( char ** ) argList, currProcess );
			}
			else if( pid < 0 )					// fork failed
			{
				printf( "mandelmovie: fork failed at frame %d: %s\n", currProcess, strerror( errno ) );
			
				if( completeProcess() )			// resources likely unavailable, so try to complete a process
				{
					activeProcess--;
					compProcess++;
				}

				continue;						// either way we should continue here, and try frame again
			}
			else								// we are at parent process (fork succeeded)
			{
				printf( "mandelmovie: frame %d generating with pid %d.\n", currProcess, pid );
			
				currS = currS * sInc;
				activeProcess++;
				currProcess++;
			}
		}

		if( activeProcess >= maxProcess )		// reached maximum, must wait for process to finish (before even trying to start another)
		{
			if( completeProcess() )				// wait for next process to complete
			{
				activeProcess--;
				compProcess++;
			}
		}

		printf( "Active: %d, Max: %d\n", activeProcess, maxProcess );
	}

	while( compProcess < FRAMES || activeProcess > 0 )				// still have processes to wait on
	{
		if( completeProcess() )
		{
			activeProcess--;
			compProcess++;
		}
	}

	return 1;
}

void newProcess( char ** argList, int frameNumber )		// spawn a new process
{
	int retVal;

	if( ( retVal = execvp( MANDEL_PATH, argList ) ) == -1 )			// run mandel and check for return error
	{
		printf( "mandelmovie: frame %d generation failed: %s\n", frameNumber, strerror( errno ) );
	}
}

int completeProcess( void )
{
	int pid;
	int signal;

	if( ( pid = waitpid( -1, &signal, 0 ) ) == -1 )		// wait specifically for the process we just started
	{
		printf( "mandelmovie: wait error: %s\n", strerror( errno ) );
	
		return 0;
	}
	else
	{
		if( WIFSIGNALED( signal ) != 0 )			// abnormal exit
		{
			printf( "mandelmovie: process %d exited abnormally with signal %d: %s\n", pid, WTERMSIG( signal ), strsignal( signal ) );
		}
		else if( WIFEXITED( signal ) != 0 )			// normal exit
		{
			printf( "mandelmovie: process %d exited normally with status %d\n", pid, WEXITSTATUS( signal ) );
		}
		else if( WIFSTOPPED( signal ) != 0 )		// process was stopped
		{
			printf( "mandelmovie: process %d was stopped with signal %d\n", pid, WSTOPSIG( signal ) );
		}
		else if( WIFCONTINUED( signal ) != 0 )		// process was continued
		{
			printf( "mandelmovie: process %d was continued\n", pid );
		}
		else										// no idea what happened
		{
			printf( "mandelmovie: process %d exited abnormally\n", pid );
		}

		return 1;
	}
}

