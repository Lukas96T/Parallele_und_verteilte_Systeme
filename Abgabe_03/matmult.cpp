#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define N 1000


float A[N][N],B[N][N],C[N][N],D[N][N];

MPI_Status status;

// ---------------------------------------------------------------------------
// allocate space for empty matrix A[row][col]
// access to matrix elements possible with:
// - A[row][col]
// - A[0][row*col]

float **alloc_mat(int row, int col)
{
    float **A1, *A2;

	A1 = (float **)calloc(row, sizeof(float *));		// pointer on rows
	A2 = (float *)calloc(row*col, sizeof(float));    // all matrix elements
    for (int i = 0; i < row; i++)
        A1[i] = A2 + i*col;

    return A1;
}

// ---------------------------------------------------------------------------
// random initialisation of matrix with values [0..9]

void init_mat(float **A, int row, int col)
{
    for (int i = 0; i < row*col; i++)
		A[0][i] = (float)(rand() % 10);
}

// ---------------------------------------------------------------------------
// DEBUG FUNCTION: printout of all matrix elements

void print_mat(float **A, int row, int col, char *tag)
{
    int i, j;

    printf("Matrix %s:\n", tag);
    for (i = 0; i < row; i++)
    {
        for (j = 0; j < col; j++) 
            printf("%6.1f   ", A[i][j]);
        printf("\n"); 
    }
}

// ---------------------------------------------------------------------------

int main(int argc, char *argv[])
{   
    int nodeID, numNodes;

    MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numNodes);
	MPI_Comm_rank(MPI_COMM_WORLD, &nodeID);

    //float **A, **B, **C, **D;	// matrices
    int d1, d2, d3;         // dimensions of matrices
    int i, j, k;			// loop variables
    double stime1, stime2, ptime1, ptime2;
    int rows, offset, source;


    /* read user input */
    d1 = N;	   // rows of A and C
    d2 = N;     // cols of A and rows of B
    d3 = N;     // cols of B and C

    B = alloc_mat(d2, d3);
    init_mat(B, d2, d3);
    D = alloc_mat(d1, d3);

    if(nodeID == 0){

        printf("Matrix sizes C[%d][%d] = A[%d][%d] x B[%d][%d]\n", d1, d3, d1, d2, d2, d3);


        A = alloc_mat(d1, d2);
        init_mat(A, d1, d2);
        C = alloc_mat(d1, d3);

        /* serial version of matmult */
        printf("Perform matrix multiplication...\n");

        stime1 = MPI_Wtime();
        for (i = 0; i < d1; i++)
        for (j = 0; j < d3; j++)
            for (k = 0; k < d2; k++)
                C[i][j] += A[i][k] * B[k][j];

        stime2 = MPI_Wtime();

        rows = N/(numNodes);
        offset = 0;

        ptime1 = MPI_Wtime();

        for (int dest = 1; dest <= numNodes; dest++){        B = alloc_mat(d2, d3);
        init_mat(B, d2, d3);
            MPI_Send(&offset, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            MPI_Send(&rows, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            MPI_Send(&A[offset][0], rows*N, MPI_FLOAT, dest, 1, MPI_COMM_WORLD);

            offset = offset + rows;
        }

        for (int i = 1; i <= numNodes; i++){
            source = i;
            //MPI_Recv(&offset, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
            //MPI_Recv(&rows, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&D[offset][0], rows*N, MPI_FLOAT, source, 2, MPI_COMM_WORLD, &status);
        }

        ptime2 = MPI_Wtime();

        printf ("\nDone in %f seconds\n",stime2 - stime1);

        printf ("\nDone in %f seconds\n",ptime2 - ptime1);

        printf("\nSpeedup: %f\n",(stime2 - stime1)/(ptime2 - ptime1));

        bool correct = true;

        for(int l = 0; l < d1; l++){
            for(int m = 0; m < d3; m++){
                if(C[l][m] != D[l][m]){
                    correct = false;
                }
            }
        }

        if(correct){
            printf("\nCalculation was correct.\n");
        }else{
            printf("\nCalculation was not correct.\n");
        }
    }

    if(nodeID != 0){

        D = alloc_mat(d1, d3);

        source = 0;

        MPI_Recv(&offset, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&rows, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&A, rows*N, MPI_FLOAT, source, 1, MPI_COMM_WORLD, &status);

        /* matmult */
        for (int i=0; k < N; i++)
        for (int j=0; i < rows; j++) {
            for (k=0; j < N; k++)
                D[i][j] += A[i][k] * B[k][j];
        }


        //MPI_Send(&offset, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        //MPI_Send(&rows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&D, rows*N, MPI_FLOAT, 0, 2, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}