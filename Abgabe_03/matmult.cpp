#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define N 1000

MPI_Status status;

float A[N][N],B[N][N],C[N][N],D[N][N];


// ---------------------------------------------------------------------------

int main(int argc, char *argv[])
{   
    int nodeID, numNodes;

    MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numNodes);
	MPI_Comm_rank(MPI_COMM_WORLD, &nodeID);

    //float **A, **B, **C, **D;	// matrices
    int i, j, k;			// loop variables
    double stime1, stime2, ptime1, ptime2;
    int rows, offset, source;

    //printf ("\nWorker: %d\n",nodeID);


    if(nodeID == 0){

        //printf ("\nNumNodes %d\n",numNodes);

        printf("Matrix sizes C[%d][%d] = A[%d][%d] x B[%d][%d]\n", N, N, N, N, N, N);


        for (i=0; i<N; i++){
            for (j=0; j<N; j++){
                A[i][j]= (float)(rand() % 10);
                B[i][j]= (float)(rand() % 10);
            }
        }


        /* serial version of matmult */
        printf("Perform matrix multiplication...\n");

        stime1 = MPI_Wtime();
        for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            for (k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];

        stime2 = MPI_Wtime();

        rows = N/(numNodes-1);
        offset = 0;

        ptime1 = MPI_Wtime();

        for (int dest = 1; dest < numNodes; dest++){        
            //printf ("\nSending offset to worker %d\n",dest);
            MPI_Send(&offset, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            //printf ("\nSending rows to worker %d\n",dest);
            MPI_Send(&rows, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            //printf ("\nSending A to worker %d\n",dest);
            MPI_Send(&A[offset][0], rows*N, MPI_FLOAT, dest, 1, MPI_COMM_WORLD);
            MPI_Send(&B, N*N, MPI_FLOAT, dest, 1, MPI_COMM_WORLD);

            offset = offset + rows;
        }

        for (int i = 1; i < numNodes; i++){
            source = i;

            MPI_Recv(&offset, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&rows, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&D[offset][0], rows*N, MPI_FLOAT, source, 2, MPI_COMM_WORLD, &status);
        }

        ptime2 = MPI_Wtime();

        printf ("\nDone in %f seconds\n",stime2 - stime1);

        printf ("\nDone in %f seconds\n",ptime2 - ptime1);

        printf("\nSpeedup: %f\n",(stime2 - stime1)/(ptime2 - ptime1));

        bool correct = true;

        for(int l = 0; l < N; l++){
            for(int m = 0; m < N; m++){
                if(C[l][m] != D[l][m]){
                    //printf("\nC %f  D %f\n",C[l][m],D[l][m]);
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

        source = 0;
        //printf ("\nWorker %d recv offset\n",nodeID);
        MPI_Recv(&offset, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
        //printf ("\nWorker %d recv rows\n",nodeID);
        MPI_Recv(&rows, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
        //printf ("\nWorker %d recv A\n",nodeID);
        MPI_Recv(&A, rows*N, MPI_FLOAT, source, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&B, N*N, MPI_FLOAT, source, 1, MPI_COMM_WORLD, &status);

        /* matmult */
        for (int i=0; i < N; i++)
        for (int j=0; j < rows; j++) {
            D[j][i] = 0;
            for (k=0; k < N; k++)
                D[j][i] += A[j][k] * B[k][i];
        }

        MPI_Send(&offset, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&rows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&D, rows*N, MPI_FLOAT, 0, 2, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}