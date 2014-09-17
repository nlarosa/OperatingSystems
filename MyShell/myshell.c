// 
// Nicholas LaRosa
// CSE 30341 Project 2
//
// usage: 	./myshell
//
// commands:	start <command>	- begins a new command in background
// 				wait			- waits for next process to end 	
//				run <command>	- begins and waits for a new command
//				kill <p_ID>		- terminate given process
//				stop <p_ID>		- pause given process
//				continue <p_ID>	- continue given paused process
//

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER 4096
#define WORDS 100
#define MAX 512

int main( void )
{
	pid_t pid;					// keep track of parent or child

	char buffer[ BUFFER + 1 ];	// contains given user line (null terminator adds 1)
	char * words[ WORDS ];		// contains user line split into words
	char * currWord;			// use with strtok

	//int fileDesc;				// file descriptor of redirected output/input
	int numWords = 0;			// number of words in the current input line
	int retVal;					// return value of our child
	int signal;					// signal from wait()
	int printed = 0;			// boolean for printing
	int i;

	int stoppedProc[ MAX ];		// allow for a maximum of 512 stopped processes
	int numStopped = 0;			// keep track of stopped processes
	int numInBack = 0;			// keep track of processes in background (running)

	memset( stoppedProc, 0, sizeof( int ) * MAX );

	while( 1 )					// run shell prompt perpetually (until exit, quit or EOF)
	{
		fflush( stdout );
		printf( "myshell> " );

		memset( buffer, 0, BUFFER * sizeof( char ) );
		if( fgets( buffer, BUFFER, stdin ) == NULL )		// read error or EOF
		{
			if( ferror( stdin ) )		// read error
			{
				printf( "myshell: input error: %s\n", strerror( errno ) );
				exit( 0 );
			}
			else						// reached EOF
			{
				exit( 0 );
			}
		}

		numWords = 0;			// prepare to grab each word

		currWord = strtok( buffer, " \t\n" );
		while( currWord != NULL )
		{
			words[ numWords ] = currWord;		// store pointer in words array
			words[ ++numWords ] = NULL;			// preincrement set to NULL

			currWord = strtok( NULL, " \t\n" );
		}

		if( strcmp( words[ 0 ], "quit" ) == 0 || strcmp( words[ 0 ], "exit" ) == 0 )		// exit conditions - NOTE: EOF
		{
			exit( 0 );						// exit successfully
		}
		else if( strcmp( words[ 0 ], "clear" ) == 0 )					// custom clear command
		{
			printf("\033[2J\033[1;1H");
		}
		else if( strcmp( words[ 0 ], "help" ) == 0 )					// custom help command
		{
			printf("\n\tWelcome to the LaRosa Shell (lsh)\n\n");
			printf("\tstart <command <args...>>\t- begin a new background process\n");
			printf("\trun <command <args...>>\t\t- begin a new foreground process\n");
			printf("\twait\t\t\t\t- wait for background process to complete\n");
			printf("\tkill <pid>\t\t\t- terminate given process by ID\n");
			printf("\tstop <pid>\t\t\t- stop given process by ID\n");
			printf("\tcontinue <pid>\t\t\t- continue given process by ID\n\n");
		}
		else if( strcmp( words[ 0 ], "start" ) == 0 )					// start - iniates process in background
		{
			pid = fork();
			numInBack++;

			if( pid == 0 )								// As Neil Young would say, "I am a child"
			{
				/*
				if( numWords > 2 )						// possible for input/output direct
				{
					for( i = 2; i < numWords; i++ )			// iterate through words to see if redirect output/input
					{
						if( words[ i ][ 0 ] == '<' )		// provide input file
						{
							if( i == numWords - 2 )			// one more argument after (must be output)
							{
								if( words[ i + 1 ][ 0 ] == '>' )	// provide output file
								{
									fileDesc = open( words[ i ] + 1, O_RDONLY, 0 );		// open file for reading (after first character)
									dup2( fileDesc, 0 );								// send the file to stdin

									fileDesc = open( words[ i + 1 ] + 1, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
									dup2( fileDesc, 1 );			// send stdout to file
									dup2( fileDesc, 2 );			// send stderror to file

									close( fileDesc );
								}
							}
							else if( i == numWords - 1 )	// last argument (only an input)
							{
								fileDesc = open( words[ i ] + 1, O_RDONLY, 0 );		// open file for reading (after first character)
								dup2( fileDesc, 0 );								// send the file to stdin

								close( fileDesc );
							}
							else							// too many arguments!
							{
								printf( "myshell: error: invalid redirect\n" );
							}
						}
						else if( words[ i ][ 0 ] == '>' )	// provide output file
						{
							if( i == numWords - 2 )			// one more argument after (must be input)
							{
								if( words[ i + 1 ][ 0 ] == '<' )	// provide input file
								{
									fileDesc = open( words[ i + 1 ] + 1, O_RDONLY, 0 );	// open file for reading (after first character)
									dup2( fileDesc, 0 );								// send the file to stdin

									fileDesc = open( words[ i ] + 1, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
									dup2( fileDesc, 1 );			// send stdout to file
									dup2( fileDesc, 2 );			// send stderror to file

									close( fileDesc );
								}
							}
							else if( i == numWords - 1 )	// last argument (only an output)
							{
								fileDesc = open( words[ i + 1 ] + 1, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
								dup2( fileDesc, 1 );			// send stdout to file
								dup2( fileDesc, 2 );			// send stderror to file

								close( fileDesc );
							}
							else							// too many arguments!
							{
								printf( "myshell: error: invalid redirect\n" );
							}
						}
					}
				}
				*/

				if( ( retVal = execvp( words[ 1 ], words+1 ) ) == -1 )		// run command and check for return error
				{
					printf( "myshell: command error: %s\n", strerror( errno ) );
					numInBack--;
				}
			}
			else if( pid < 0 )							// fork failed
			{
				printf( "myshell: command %s failed: %s\n", words[ 1 ], strerror( errno ) );
			}
			else										// we are at parent process
			{
				printf( "myshell: command %d started\n", pid );

				continue;
			}
		}
		else if( strcmp( words[ 0 ], "run" ) == 0 )						// run - iniates process in foreground
		{
			pid = fork();

			if( pid == 0 )								// As Neil Young would say, "I am a child"
			{
				if( ( retVal = execvp( words[ 1 ], words+1 ) ) == -1 )	// run command and check for return error
				{
					printf( "myshell: command error: %s\n", strerror( errno ) );
				}
			}
			else if( pid < 0 )							// fork failed
			{
				printf( "myshell: command %s failed: %s\n", words[ 1 ], strerror( errno ) );
			}
			else										// we are at parent process
			{
				if( ( pid = waitpid( pid, &signal, 0 ) ) == -1 )		// wait specifically for the process we just started
				{
					printf( "myshell: wait error: %s\n", strerror( errno ) );
				}
				else
				{
					if( WIFSIGNALED( signal ) != 0 )		// abnormal exit
					{
						printf( "myshell: process %d exited abnormally with signal %d: %s\n", pid, WTERMSIG( signal ), strsignal( signal ) );
					}
					else if( WIFEXITED( signal ) != 0 )		// normal exit
					{
						printf( "myshell: process %d exited normally with status %d\n", pid, WEXITSTATUS( signal ) );
					}
					else if( WIFSTOPPED( signal ) != 0 )	// process was stopped
					{
						printf( "myshell: process %d was stopped with signal %d\n", pid, WSTOPSIG( signal ) );
					}
					else if( WIFCONTINUED( signal ) != 0 )	// process was continued
					{
						printf( "myshell: process %d was continued\n", pid );
					}
					else									// no idea what happened
					{
						printf( "myshell: process %d exited abnormally\n", pid );
					}
				}
			}
		}
		else if( strcmp( words[ 0 ], "wait" ) == 0 )					// wait - allows oldest process to complete
		{
			printed = 0;

			while( 1 )										// continually check if state of any children has changed
			{
				pid = waitpid( -1, &signal, WUNTRACED | WNOHANG | WCONTINUED );

				if( pid == -1 )								// waitpid error
				{
					printf( "myshell: wait error: %s\n", strerror( errno ) );
					break;
				}
				else if( pid == 0 )							// this means no child has changed state, implying there are stopped processes
				{
					if( printed == 0 )
					{
						for( i = 0; i < numStopped; i++ )
						{
							if( stoppedProc[ i ] != 0 )		printf( "myshell: process %d is still stopped\n", stoppedProc[ i ] );
						}
					}

					if( numInBack == 0 )	break;			// we don't want to hang when no running processes

					printed = 1;

					continue;
				}
				else
				{
					if( WIFSIGNALED( signal ) != 0 )		// abnormal exit
					{
						for( i = 0; i < numStopped; i++ )
						{
							if( stoppedProc[ i ] == pid )	// this process had been stopped
							{
								stoppedProc[ i ] = 0;						// "remove" it from stopped list
							}
						}

						numInBack--;
						printf( "myshell: process %d exited abnormally with signal %d: %s\n", pid, WTERMSIG( signal ), strsignal( signal ) );
					}
					else if( WIFEXITED( signal ) != 0 )		// normal exit
					{
						for( i = 0; i < numStopped; i++ )
						{
							if( stoppedProc[ i ] == pid )	// this process had been stopped
							{
								stoppedProc[ i ] = 0;						// "remove" it from stopped list
							}
						}

						numInBack--;
						printf( "myshell: process %d exited normally with status %d\n", pid, WEXITSTATUS( signal ) );
					}
					else if( WIFSTOPPED( signal ) != 0 )	// process was stopped
					{
						numInBack--;
						printf( "myshell: process %d was stopped with signal %d\n", pid, WSTOPSIG( signal ) );
					}
					else if( WIFCONTINUED( signal ) != 0 )	// process was continued
					{
						numInBack++;
						printf( "myshell: process %d was continued\n", pid );
					}
					else									// no idea what happened
					{
						numInBack--;
						printf( "myshell: process %d exited abnormally\n", pid );
					}

					break;
				}
			}
		}
		else if( strcmp( words[ 0 ], "kill" ) == 0 )					// kill - terminates given process by ID
		{
			if( numWords != 2 )											// kill <pid>
			{
				printf( "myshell: usage: kill <pid>\n" );
			}
			else
			{
				if( kill( atoi( words[ 1 ] ), SIGKILL ) == -1 )			// unsuccessful kill
				{
					printf( "myshell: kill error: %s\n", strerror( errno ) );
				}
				else
				{
					for( i = 0; i < numStopped; i++ )
					{
						if( stoppedProc[ i ] == atoi( words[ 1 ] ) )	// this process had been stopped
						{
							stoppedProc[ i ] = 0;						// "remove" it from stopped list
						}
					}

					printf( "myshell: process %d killed\n", atoi( words[ 1 ] ) );
				}
			}
		}
		else if( strcmp( words[ 0 ], "stop" ) == 0 )					// start - pauses given process by ID
		{
			if( numWords != 2 )											// stop <pid>
			{
				printf( "myshell: usage: stop <pid>\n" );
			}
			else
			{
				if( kill( atoi( words[ 1 ] ), SIGSTOP ) == -1 )			// unsuccessful stop
				{
					printf( "myshell: stop error: %s\n", strerror( errno ) );
				}
				else
				{
					numStopped = ( numStopped + 1 ) % MAX;

					for( i = 0; i < numStopped; i++ )
					{
						if( stoppedProc[ i ] == 0 )				// replace first 0 entries with stopped PIDS
						{
							stoppedProc[ i ] = atoi( words[ 1 ] );
							break;
						}
					}

					printf( "myshell: process %d stopped\n", atoi( words[ 1 ] ) );
				}
			}
		}	
		else if( strcmp( words[ 0 ], "continue" ) == 0 )				// continue - continue background process by ID
		{
			if( numWords != 2 )											// continue <pid>
			{
				printf( "myshell: usage: continue <pid>\n" );
			}
			else
			{
				if( kill( atoi( words[ 1 ] ), SIGCONT ) == -1 )			// unsuccessful continue
				{
					printf( "myshell: continue error: %s\n", strerror( errno ) );
				}
				else
				{
					for( i = 0; i < numStopped; i++ )
					{
						if( stoppedProc[ i ] == atoi( words[ 1 ] ) )	// this process had been stopped
						{
							stoppedProc[ i ] = 0;						// "remove" it from stopped list
						}
					}

					printf( "myshell: process %d continued\n", atoi( words[ 1 ] ) );
				}
			}
		}
		else
		{
			printf( "myshell: unknown command: %s\n", words[ 0 ] );
		}
	}

	return 0;
}

