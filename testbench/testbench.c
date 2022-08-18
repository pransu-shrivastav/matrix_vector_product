#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//
// AHIR release utilities
//
#include <pthreadUtils.h>
#include <Pipes.h>
#include <pipeHandler.h>

// These will wait.
#ifndef AA2C
	#include "vhdlCStubs.h"
#else
	#include "aa_c_model.h"
#endif

#define MATRIX_ORDER  	32
#define BASE_VAL	((MATRIX_ORDER * (MATRIX_ORDER - 1))/2)

int __err_flag__ = 0;
// send vectors to the device
//  we will send 211..1, 1211..1, 11211...1,  ... etc.
//
void Sender()
{
	int I;
	uint32_t x[MATRIX_ORDER];
	

	for(I = 0; I < MATRIX_ORDER; I++)
	{
		int J;
		for(J = 0; J < MATRIX_ORDER; J++)
		{
			if(J == I)
				x[J] = 2;
			else
				x[J] = 1;
		}

		write_uint32_n("in_data", x, MATRIX_ORDER);
		fprintf(stderr,"Info: Sender: sent vector number %d\n", I);
	}
}
DEFINE_THREAD(Sender);

//  The matrix is setup as shown below, and we
//  are computing A.1 + A.ei, where 1 is the vector
//  of all 1's and e is a unit vector with 1 in the
//  i'th position.
//
//  The expected results are 
//     [496 497 498 ...  527]
//     [497 498 .... 527 496]
//  etc.
void Receiver()
{
	uint32_t expected_y[MATRIX_ORDER];
	uint32_t y[MATRIX_ORDER];
	int I;
	for(I = 0; I < MATRIX_ORDER; I++)
	{
		int J;

		for(J = 0; J < MATRIX_ORDER; J++)
		{
			uint32_t column_correction = (J + I) % MATRIX_ORDER;
			expected_y[J] = BASE_VAL + column_correction;
		}
		read_uint32_n("out_data",y, MATRIX_ORDER);
		
		for(J = 0; J < MATRIX_ORDER; J++)
		{
			if( y[J] != expected_y[J])
			{
				fprintf(stderr,"Error I=%d, y[%d], expected_y[%d]\n",
						I, y[J], expected_y[J]);
			}
		}
		fprintf(stderr,"Info: Receiver: received vector number %d\n", I);
	}
}
DEFINE_THREAD(Receiver);


// Matrix rows are identical
//   0th row  is  0  1  2 ....    31	
//   1st row  is  1  2  3 ...  31  0
//   2nd row  is  2  3  4 .. 31 0  1
//     each row is the previous row
//     rotated left.
void sendMatrix()
{
	int I, J;
	for(I = 0; I < MATRIX_ORDER; I++)
	{
		for(J = 0; J < MATRIX_ORDER; J++)
		{
			uint32_t aij = (J + I) % MATRIX_ORDER;
			write_uint32("in_data", aij);
		}
	}
}


int main(int argc, char* argv[])
{

	if(argc < 2)
	{
		fprintf(stderr,"Usage: %s [trace-file]\n trace-file=null for no trace, stdout for stdout\n",
				argv[0]);
		return(1);
	}

	FILE* fp = NULL;
	if(strcmp(argv[1],"stdout") == 0)
	{
		fp = stdout;
	}
	else if(strcmp(argv[1], "null") != 0)
	{
		fp = fopen(argv[1],"w");
		if(fp == NULL)
		{
			fprintf(stderr,"Error: could not open trace file %s\n", argv[1]);
			return(1);
		}
	}

#ifdef AA2C
	init_pipe_handler();
	start_daemons (fp,0);
#endif
	// send the matrix.
	sendMatrix();

	// return(0);
	// start the receiver and sender.
	PTHREAD_DECL(Receiver);
	PTHREAD_CREATE(Receiver);

	PTHREAD_DECL(Sender);
	PTHREAD_CREATE(Sender);


	// wait on the two output threads
	PTHREAD_JOIN(Sender);
	PTHREAD_JOIN(Receiver);

	if(__err_flag__)
	{
		fprintf(stderr,"\nFAILURE.. there were errors\n");
	}
	else
	{
		fprintf(stderr,"\nSUCCESS!\n");
	}
	return(0);
}

