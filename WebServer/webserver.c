
/*
This is the main program for the simple webserver.
Your job is to modify it to take additional command line
arguments to specify the number of threads and scheduling mode,
and then, using a pthread monitor, add support for a thread pool.
All of your changes should be made to this file, with the possible
exception that you may add items to struct request in request.h
*/

// Nicholas LaRosa
// CSE 30341
// Project 4

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include <time.h>
#include <sys/stat.h>

#include "tcp.h"
#include "request.h"

#define BUFFER_SIZE 10
#define DEBUG 0

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER; 

struct requestNode
{
	struct request * req;						// contains request
	struct requestNode * nextReq;				// and a pointer to the next
};

struct threadArgs
{
	int threadID;
	struct requestNode * requestHead;			// each thread recieves the request buffer (head of linked list)
};

void * handleRequest( struct threadArgs * args );
int bufferEmpty( struct requestNode * head );
int bufferFull( struct requestNode * head );
int addRequestSFF( struct requestNode * head, struct request * new );
int addRequestFCFS( struct requestNode * head, struct request * new );
struct request * getRequest( struct requestNode * head );
void printBuffer( struct requestNode * head );

void * handleRequest( struct threadArgs * args )			// handleRequest receives the buffer of requests and mode as its argument
{
	struct request * req = NULL;							// copy request to thread to allow for earlier mutex unlock

	while( 1 )
	{
		if( DEBUG ) printf( "Inside thread %d.\n", args->threadID );

		pthread_mutex_lock( &mutex );

		if( DEBUG ) printf( "Thread %d granted mutex access.\n", args->threadID );
			
		while( bufferEmpty( args->requestHead ) )				// block on empty buffer
		{
			if( DEBUG ) printf( "Thread %d blocking on empty buffer.\n", args->threadID );
			pthread_cond_wait(&cond,&mutex);			
		}														// upon exit of while loop, this thread has access to request buffer

		req = getRequest( args->requestHead );
		if( DEBUG ) printf( "Thread %d getting first request.\n", args->threadID );

		pthread_cond_broadcast( &cond );						// this thread is done copying from the request buffer
		pthread_mutex_unlock( &mutex );		

		if( req )
		{
			printf("webserver thread %d: got request for %s\n", args->threadID, req->filename);
			request_handle( req );

			printf("webserver thread %d: done sending %s\n", args->threadID, req->filename); 
			request_delete( req );
		}
	}
}

int bufferEmpty( struct requestNode * head )				// threads block while buffer is empty
{
	if( head->nextReq == NULL )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int bufferFull( struct requestNode * head )					// main blocks while buffer is full (size of 10)
{
	int count;
	struct requestNode * current;

	if( head->nextReq == NULL )								// empty buffer
	{
		return 0;
	}

	for( current = head->nextReq; current->nextReq != NULL; current = current->nextReq )
	{
		count++;
	}

	if( count == BUFFER_SIZE )	return 1;
	
	return 0;
}

int addRequestFCFS( struct requestNode * head, struct request * new )		// adds new request to end of linked list
{
	struct requestNode * current;
	struct requestNode * newNode;
	newNode = malloc( sizeof( struct requestNode ) );						// this new node will contain the new request

	if( DEBUG ) printf( "Adding request by FCFS.\n" );

	newNode->req = new;														// and set new node as tail pointer
	newNode->nextReq = NULL;

	if( head->nextReq == NULL )												// add first request
	{
		head->nextReq = newNode;
		if( DEBUG ) printf( "Added request at front of list.\n" );
		printBuffer( head );
		return 1;
	}

	for( current = head->nextReq; current->nextReq != NULL; current = current->nextReq )		// at end, currentNode = tail
	{}

	current->nextReq = newNode;												// point last node to new node

	printBuffer( head );

	return 1;
}

int addRequestSFF( struct requestNode * head, struct request * new )		// adds new request based on filesize
{
	struct requestNode * current;
	struct requestNode * previous = head;

	struct requestNode * newNode;
	newNode = malloc( sizeof( struct requestNode ) );
	
	if( DEBUG ) printf( "Adding request by SFF.\n" );

	newNode->req = new;
	newNode->nextReq = NULL;

	if( head->nextReq == NULL )												// if buffer is empty, just add to front
	{
		head->nextReq = newNode;
		printf( "Added request at front of list.\n" );
		printBuffer( head );
		return 1;
	}

	for( current = head->nextReq; current->nextReq != NULL; current = current->nextReq )
	{
		if( newNode->req->fileSize > current->req->fileSize )				// new filesize is larger than current, continue to search
		{
			printf( "Surpassing %d with %d.\n", current->req->fileSize, newNode->req->fileSize );
			previous = previous->nextReq;
			continue;
		}
		else 																// new filesize should be inserted before current
		{
			if( previous->req == NULL )										// previous was head (current is first)
			{
				printf( "Adding size: %d before %d.\n", newNode->req->fileSize, current->req->fileSize );
				newNode->nextReq = current;
				head->nextReq = newNode;
				printBuffer( head );
				return 1;
			}
			else
			{
				printf( "Adding size: %d in between %d and %d.\n", newNode->req->fileSize, previous->req->fileSize, current->req->fileSize );
				newNode->nextReq = current;
				previous->nextReq = newNode;
				printBuffer( head );
				return 1;
			}
		}
	}

	/*
	if( previous->req == NULL )										// previous was head (current is first)
	{
		printf( "Adding size: %d before %d.\n", newNode->req->fileSize, current->req->fileSize );
		newNode->nextReq = current;
		head->nextReq = newNode;
		printBuffer( head );
		return 1;
	}*/

	if( current->nextReq == NULL )									// we are at the last node. compare and add newNode
	{
		if( newNode->req->fileSize > current->req->fileSize )		// add after
		{
			printf( "Adding size: %d after %d.\n", newNode->req->fileSize, current->req->fileSize );
			current->nextReq = newNode;
			newNode->nextReq = NULL;
			return 1;
		}
		else														// add before
		{
			printf( "Adding size: %d before %d.\n", newNode->req->fileSize, current->req->fileSize );
			newNode->nextReq = current;
			head->nextReq = newNode;
			printBuffer( head );
			return 1;
		}	
	}

	printBuffer( head );

	return 1;
}	

struct request * getRequest( struct requestNode * head )				// pop linked list
{
	struct requestNode * current;
	current = malloc( sizeof( struct requestNode ) );					// make copy (transferred to threads)
	current = head->nextReq;

	head->nextReq = current->nextReq;									// head points to next pointer, deleting top request

	return current->req;
}

void printBuffer( struct requestNode * head )
{
	if( DEBUG ) printf( "Attempting to print buffer.\n" );

	struct requestNode * current = head;
	int count = 1;

	if( head->nextReq == NULL )
	{
		if( DEBUG ) printf( "This buffer is empty...\n" );
		return;
	}

	printf( "\n" );

	for( current = head->nextReq; current->nextReq != NULL; current = current->nextReq )
	{
		printf( "%d: FILE: %s SIZE: %d --> ", count, current->req->filename, current->req->fileSize );
	
		count++;
	}

	if( current->req == NULL )
	{
		current = current->nextReq;
	}

	printf( "%d: FILE: %s SIZE: %d\n\n", count, current->req->filename, current->req->fileSize );

	if( DEBUG ) printf( "Done printing buffer.\n" );
}

int main( int argc, char *argv[] )
{
	if(argc<4) 
	{
		fprintf(stderr,"use: %s <port> <nthreads> <mode>\n",argv[0]);
		return 1;
	}

	if(chdir("webdocs")!=0)
	{
		fprintf(stderr,"couldn't change to webdocs directory: %s\n",strerror(errno));
		return 1;
	}
	
	int i;

	int port = atoi(argv[1]);			// PORT
	int threads = atoi(argv[2]);		// NTHREADS
	char mode[5];
	strcpy( mode, argv[3] );			// MODE

	for( i = 0; mode[i]; i++ )			// check mode to ensure valid mode
	{
		mode[i] = tolower( mode[i] );
	}

	if( strcmp( mode, "fcfs" ) != 0 && strcmp( mode, "sff" ) != 0 )				// neither matches fcfs or sff
	{
		fprintf(stderr,"No such mode %s found. Specify FCFS or SFF.\n", mode);
		exit( 1 );
	}

	struct tcp *master = tcp_listen(port);
	if(!master) 
	{
		fprintf(stderr,"couldn't listen on port %d: %s\n",port,strerror(errno));
		return 1;
	}

	printf("webserver: waiting for requests..\n");

	pthread_t * threadID;									// array of thread IDs (dynamic)
	threadID = malloc( sizeof( pthread_t ) * threads );		// dynamically establish our thread pool

	struct threadArgs * args;
	args = malloc( sizeof( struct threadArgs ) * threads );	// dynamically establish thread arguments	

	struct requestNode * head;								// head of our linked list
	head = malloc( sizeof( struct requestNode ) );
	head->req = NULL;										// head is marked by a NULL request
	head->nextReq = NULL;

	/*
	struct request * requestBuffer[ BUFFER_SIZE ];			// buffer of requests to be fulfilled and its current index (where to append)
	for( i = 0; i < BUFFER_SIZE; i++ )
	{
		requestBuffer[i] = malloc( sizeof( struct request ) );
		requestBuffer[i]->valid = 0;						// initialize as invalid
	}
	*/

	struct stat st;						// used to generate requested filesizes
	int retVal;

	// before we wait for requests, we need to establish thread pool

	for( i = 0; i < threads; i++ )
	{
		args[i].threadID = i;			// initialize thread arguments
		args[i].requestHead = head;
	
		if( ( retVal = pthread_create( &threadID[i], NULL, ( void * )&handleRequest, ( void * )&args[i] ) ) < 0 )		
		{
			fprintf( stderr, "pthread_create error: %s", strerror( errno ) );
			i--;	
		}
		else
		{
			printf( "Thread %d created.\n", i );
		}
	}

	while(1)
	{
		struct tcp *conn = tcp_accept(master,time(0)+300);
	
		if(conn) 
		{
			printf("webserver: got new connection.\n");
			
			struct request * req = request_create(conn);
			stat( req->filename, &st );						// get file size via stat()
			req->fileSize = st.st_size;
			
			printf( "File size: %d, File path: %s\n", req->fileSize, req->filename );
			
			pthread_mutex_lock(&mutex);
		
			if( DEBUG ) printf( "Main granted mutex access.\n" );

			while( bufferFull( head )  )					// we need to add a request to the buffer, but first check if full and block if so
			{
				if( DEBUG ) printf( "Main blocking on full buffer.\n" );
				pthread_cond_wait(&cond,&mutex);	
			}												// at this point, main has access to buffer and can add request

			if( req )										// add request to buffer according to selection scheme (next select at top)
			{
				if( strcmp( mode, "fcfs" ) == 0 )
				{
					addRequestFCFS( head, req );
				}
				else if( strcmp( mode, "sff" ) == 0 )
				{
					addRequestSFF( head, req );
				}
			}
			else
			{
				tcp_close( conn );
			}

			pthread_cond_broadcast( &cond );				// release buffer	
			pthread_mutex_unlock( &mutex );		
		} 
		else
		{
			printf("webserver: shutting down because idle too long\n");
			break;
		}
	}
	
	free( threadID );
	free( args );

	return 0;
}
