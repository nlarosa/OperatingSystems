//
// Nicholas LaRosa
// CSE30341 Project 3
//
// Modified from original
//
// -n option added
//

#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#include <pthread.h>

#define MAX_THREADS 25

struct thread_args
{
	struct bitmap *bm;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int max;
	int threadNum;				// passing thread number to set height limit in compute_image
	int totalThreads;
};

int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void * compute_image( struct thread_args *args );

void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=500)\n");
	printf("-H <pixels> Height of the image in pixels. (default=500)\n");
	printf("-n <num>    Number of threads used to generate plot. (default=1)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

int main( int argc, char *argv[] )
{
	char c;

	struct thread_args * args;			// array of arguments to thread function (dynamic)
	pthread_t * threadID;				// array of thread IDs (dynamic)

	int completedThreads = 0;

	// These are the default configuration values used
	// if no command line arguments are given.

	const char *outfile = "mandel.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 500;
	int    image_height = 500;
	int    max = 1000;
	int    threads = 1;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:n:o:h"))!=-1) {
		switch(c) {
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				scale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'n':
				threads = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}

	// Display the configuration of the image.
	printf("mandel: x=%lf y=%lf scale=%lf max=%d threads=%d outfile=%s\n",xcenter,ycenter,scale,max,threads,outfile);

	// Create a bitmap of the appropriate size.
	struct bitmap *bm = bitmap_create(image_width,image_height);

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	// Compute the Mandelbrot image with threads
	int i = 0;
	int retVal;
	
	args = malloc( sizeof( struct thread_args ) * threads );		// dynamically create array of thread argument structures	
	threadID = malloc( sizeof( pthread_t ) * threads );				// dynamically create array of thread IDs

//	while( completedThreads < threads )								// thread program in batches of MAX_THREADS
//	{
		for( i = 0; i < threads; i++ )
		{
			if( i >= threads )										// no more threads needed
			{
				break;
			}

			args[ i ].bm = bm;
			args[ i ].xmin = xcenter-scale;
			args[ i ].xmax = xcenter+scale;
			args[ i ].ymin = ycenter-scale;
			args[ i ].ymax = ycenter+scale;
			args[ i ].max = max;
		
			args[ i ].threadNum = i + completedThreads;				// only variables that vary each thread
			args[ i ].totalThreads = threads;

			if( ( retVal = pthread_create( &threadID[ i ], NULL, ( void * )&compute_image, ( void * )&args[ i ] ) ) < 0 )
			{
				printf( "pthread_create: error: %s\n", strerror( errno ) );
				i--;												// retry this thread
			}

			//printf( "Thread %d spawned.\n", i );	
		}

		for( i = 0; i < threads; i++ )
		{
			if( i >= threads )										// no more threads needed
			{
				break;
			}

			if( ( retVal = pthread_join( threadID[ i ], NULL ) ) < 0 )
			{
				printf( "pthread_join: error: %s\n", strerror( errno ) );
			}
			else
			{
				//printf( "Thread %d successfully joined.\n", i );

				completedThreads++;
				//printf( "%d total threads completed.\n", completedThreads );
			}
		}

	free( threadID );
	free( args );
	
//	}

	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}

	return 0;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void * compute_image( struct thread_args *args )
{
	int i,j;

	int width = bitmap_width( args->bm );
	int height = bitmap_height( args->bm );

	// For every pixel in the image...

	int startHeight = ( int )( ( ( double )args->threadNum / ( double )args->totalThreads ) * height );
	int endHeight;

	if( args->threadNum == args->totalThreads - 1 )			// we are on the last thread
	{
		endHeight = height;
	}
	else
	{
		endHeight = startHeight + ( height / args->totalThreads );
	}

	printf( "Thread %d drawing height pixels %d to %d.\n", args->threadNum, startHeight, endHeight );

	for(j=startHeight;j<endHeight;j++) {

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = (args->xmin) + i*((args->xmax)-(args->xmin))/width;
			double y = (args->ymin) + j*((args->ymax)-(args->ymin))/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,(args->max));

			// Set the pixel in the bitmap.
			bitmap_set((args->bm),i,j,iters);
		}
	}

	return NULL;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color( int i, int max )
{
	int gray = 255*i/max;
	return MAKE_RGBA(gray,gray,gray,0);
}




