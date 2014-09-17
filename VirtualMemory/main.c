/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEBUG 0

#define RAND 0
#define FIFO 1
#define CUSTOM 2

#define AGE 5

int algorithmInt = 0;

struct disk * disk;
char * physmem;

int currentFrame;					// index of next frame to replace
int freeFrames;						// number of free frames
struct frameInfo * frameTable;		// frame table to keep track of whether frames are in use and frame-to-page mappings
int nframes;

int pageFaults = 0;					// STATISTICS
int diskReads = 0;
int diskWrites = 0;

struct frameInfo					// frame info struct contains frame-to-page mapping and free boolean
{
	int givenSecondChance;			// used for custom algorithm - the number of page faults this frame has survived without replacement
	int pageNumber;
	int free;
};

void page_fault_handler( struct page_table *pt, int page );

int getFrameRand();
int getFrameFifo();
int getFrameCustom();

void updateSurvivors( int frameToBeReplaced );
int getFreeFrameArray( int * freeFrameTable );
void initializeFrameTable( int nframes );
int isDigit( char * argument );

void page_fault_handler( struct page_table *pt, int page )
{
	int frameNumber;
	int bits = 0;

//	printf( "Page fault on page #%d\n", page );
	page_table_get_entry( pt, page, &frameNumber, &bits );	// check if page exists

	if( bits == 0 )											// page needs to be read from disk, frame chosen according to algorithm
	{
		if( algorithmInt == RAND )			frameNumber = getFrameRand();		// retrieve frame number according to algorithm
		else if( algorithmInt == FIFO )		frameNumber = getFrameFifo();
		else if( algorithmInt == CUSTOM )	frameNumber = getFrameCustom();

		if( frameTable[ frameNumber ].free == 1 )		// just need to read from disk and link
		{
			if( DEBUG ) printf( "Frame %d is unoccupied.\n", frameNumber );		

			disk_read( disk, page, &physmem[ frameNumber * BLOCK_SIZE ] );			// read from disk into physical
			page_table_set_entry( pt, page, frameNumber, PROT_READ );				// link physical and virtual
			
			frameTable[ frameNumber ].free = 0;
			frameTable[ frameNumber ].pageNumber = page;							// set frame-to-page mapping
		
			diskReads++;
		}
		else											// frame to be replaced is populated. check if page linked to frame was written before replacing
		{
			if( DEBUG ) printf( "Frame %d is occupied and maps to page number %d.\n", frameNumber, frameTable[ frameNumber ].pageNumber );

			bits = 0;
			page_table_get_entry( pt, frameTable[ frameNumber ].pageNumber, &frameNumber, &bits );

			if( bits == ( PROT_READ | PROT_WRITE ) )	// page and frame modified
			{
				if( DEBUG ) printf( "Frame %d has been modified.\n", frameNumber );

				disk_write( disk, frameTable[ frameNumber ].pageNumber, &physmem[ frameNumber * BLOCK_SIZE ] );		// write the change from the frame to disk
				disk_read( disk, page, &physmem[ frameNumber * BLOCK_SIZE ] );										// read the disk into the frame
				
				if( DEBUG ) printf( "Writing contents of frame %d to disk at %d.\n", frameNumber, frameTable[ frameNumber ].pageNumber );

				page_table_set_entry( pt, page, frameNumber, PROT_READ );											// map page to frame
				page_table_set_entry( pt, frameTable[ frameNumber ].pageNumber, 0, 0 );								// invalidate this page
		
				if( DEBUG ) printf( "Mapping frame %d to page %d.\n", frameNumber, page );
	
				frameTable[ frameNumber ].pageNumber = page;			// modify mapping
			
				diskReads++;
				diskWrites++;
			}
			else										// no modifications made, just replace page/frame as if unoccupied
			{
				if( DEBUG ) printf( "Frame %d has not been modified.\n", frameNumber );
			
				disk_read( disk, page, &physmem[ frameNumber * BLOCK_SIZE ] );			// read from disk into physical
				page_table_set_entry( pt, page, frameNumber, PROT_READ );				// link physical and virtual
				page_table_set_entry( pt, frameTable[ frameNumber ].pageNumber, 0, 0 );	// unlink		
	
				frameTable[ frameNumber ].free = 0;
				frameTable[ frameNumber ].pageNumber = page;							// set frame-to-page mapping
			
				diskReads++;
			}
		}
	}
	else												// page needs its bits changed to R/W (ie. page and frame modified)
	{
		page_table_set_entry( pt, page, frameNumber, PROT_READ | PROT_WRITE );

		if( DEBUG ) printf( "Page %d being written to frame %d.\n", page, frameNumber );

		if( algorithmInt == CUSTOM )
		{
			if( DEBUG ) printf( "Frame %d given yet another chance.\n", frameNumber );

			frameTable[ frameNumber ].givenSecondChance = 0;		// writing to frame implies this frame is used more, so let's keep it longer
		}
	}	

	pageFaults++;

	if( DEBUG ) printf( "\n" );
}

int getFrameRand()					// get frame number with rand algorithm
{
	int frameNumber;
	int * freeFrameTable;
	int freeFrames = 0;

	freeFrameTable = malloc( nframes * sizeof( int ) );	// where to store free frame indices
	freeFrames = getFreeFrameArray( freeFrameTable );

	if( freeFrames == 0 )
	{
		frameNumber = lrand48() % nframes;		// just pick a frame, all are already occupied
		if( DEBUG ) printf( "No free frames. Frame number %d chosen.\n", frameNumber );
	}
	else
	{
		frameNumber = lrand48() % freeFrames;			// pick a random free frame index
		frameNumber = freeFrameTable[ frameNumber ];	// and retrieve frame number
		if( DEBUG ) printf( "There are %d free frames. Frame number %d chosen.\n", freeFrames, frameNumber );
	}

	free( freeFrameTable );

	return frameNumber;
}

int getFrameFifo()					// get frame number with fifo algorithm
{
	int frameNumber;

	frameNumber = currentFrame;						// grab global frame counter

	currentFrame++;									// and increment / wraparound

	if( currentFrame >= nframes )	currentFrame = 0;

	return frameNumber;
}

int getFrameCustom()				// get frame number with second-chance algorithm
{
	int frameNumber;

	while( !frameTable[ currentFrame ].free && !frameTable[ currentFrame ].givenSecondChance )		// replace first encountered frame that is free or given second chance already
	{
		frameTable[ currentFrame ].givenSecondChance = 1;
		currentFrame++;

		if( currentFrame >= nframes ) 	currentFrame = 0;
	}

	frameNumber = currentFrame;
	frameTable[ frameNumber ].givenSecondChance = 0;			// give this frame a second chance at survival

	return frameNumber;
}

int getFreeFrameArray( int * freeFrameTable )	// populates freeFrameTable with indices of free frames, and returns number of free frames
{
	int freeFrames = 0;
	int i;

	for( i = 0; i < nframes; i++ )
	{
		if( frameTable[ i ].free == 1 )				// free frame!
		{
			freeFrameTable[ freeFrames ] = i;
			freeFrames++;
		}
	}

	return freeFrames;
}

void initializeFrameTable( int nframes )												// initialize frame table to empty
{
	int i = 0;

	for( i = 0; i < nframes; i++ )
	{
		frameTable[ i ].free = 1;					// all frames initially unoccupied
		frameTable[ i ].pageNumber = 0;				// so page number irrelevant
		frameTable[ i ].givenSecondChance = 0;
	}
}

int isDigit( char * argument )						// return 1 if valid, 0 if not
{
	int i;

	for( i = 0; i < strlen( argument ); i++ )		// for each digit
	{
		if( !isdigit( argument[ i ] ) )
		{
			return 0;
		}
	}

	return 1;
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: ./virtmem <numberOfPages> <numberOfFrames> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}
	
	if( !isDigit( argv[1] ) || !isDigit( argv[2] ) )	// check for non-numeric number of pages or frames
	{
		fprintf( stderr, "Non-numeric input for number of pages and/or frames.\n" );
		printf("use: ./virtmem <numberOfPages> <numberOfFrames> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	nframes = atoi(argv[2]);

	const char *algorithm = argv[3];
	const char *program = argv[4];

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	struct page_table *pt = NULL;

	if( !strcmp( algorithm, "rand" ) )
	{
		algorithmInt = RAND;
	}
	else if( !strcmp( algorithm, "fifo" ) )
	{
		algorithmInt = FIFO;
	}
	else if( !strcmp( algorithm, "custom" ) )
	{
		algorithmInt = CUSTOM;
	}
	else
	{
		fprintf( stderr, "Invalid page replacement algorithm" );
		return 1;
	}

	pt = page_table_create( npages, nframes, page_fault_handler );
	
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	physmem = page_table_get_physmem(pt);

	frameTable = malloc( nframes * ( sizeof( struct frameInfo ) ) );			// set size of our global frame table
	initializeFrameTable( nframes );
	freeFrames = nframes;
	currentFrame = 0;

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}

	printf( "%d,%d,%d\n", pageFaults, diskReads, diskWrites );

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
