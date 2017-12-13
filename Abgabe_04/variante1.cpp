#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MASTER 0         // taskid von erstem task 
#define FROM_MASTER 1    // Nachrichtentypen 
#define FROM_WORKER 2    



// ----------------------------------------------------------------------------------
// Speicheranforderung fuer eine leere Matrix A[row][col]. 

float **alloc_mat(int row, int col) 
{
    float **A;

    A = (float **) calloc(row, sizeof(float *));           // Zeiger auf die Zeilen
	if (A) {
		A[0] = (float *)calloc(row*col, sizeof(float));         // Alle Matrixelemente
		if (A[0]) {
			for (int i = 1; i < row; i++)
				A[i] = A[i-1] + col;
			return A;
		}
	}
	perror("out of memory!"); exit(1);
}

// ----------------------------------------------------------------------------------
// Zufaellige Initialisierung einer Matrix mit den Werten [0..9]. 

void init_mat(float **A, int row, int col) 
{
    for (int i = 0; i < row*col; i++)
        A[0][i] = (float)(rand() % 10);
}

// ----------------------------------------------------------------------------------
// Sequentielle Matrixmultiplikation C = A*B. 

float **mult_mat(float **A, float **B, int d1, int d2, int d3) 
{
    float **C = alloc_mat(d1, d3);                            // Erzeugt neue Matrix
    int i, j, k;

    for (i = 0; i < d1; i++)
        for (j = 0; j < d3; j++)
            for (k = 0; k < d2; k++)
                C[i][j] += A[i][k] * B[k][j];                 // Matrixmultiplikation

    return C;
}

void mult_mat(float **A, float **B, float **C, int d0, int d1, int d2, int d3) 
{
    int i, j, k;

    for (i = d0; i < d1; i++)          // Multipliziert nur Teile einer großen Matrix
        for (j = 0; j < d3; j++)
            for (k = 0; k < d2; k++)
                C[i][j] += A[i][k] * B[k][j];          // Füllt existierende Matrix C

}

// ----------------------------------------------------------------------------------
// Tested die Gleichheit von Matrizen  

void is_correct(float **A, float **B, int row, int col)
{
    int i, j;

    for (i = 0; i < row; i++) 
        for (j = 0; j < col; j++) 
            if (A[i][j] != B[i][j]) 
                printf("error!\n");

    printf("ok.\n");
}

// ---------------------------------------------------------------------------
// Ausgabe der Matrixelemente fuer Debugzwecke

void print_mat(float **A, int row, int col, char *tag)
{
    int i, j;

    printf("Matrix %s:\n", tag);
    for (i = 0; i < row; i++) {
        for (j = 0; j < col; j++) 
            printf("%6.1f   ", A[i][j]);
        printf("\n"); 
    }
}
// ---------------------------------------------------------------------------
// Gibt GGT zurück

int ggt(int x, int y)
{
  int z;
  while(y)
  {
    z= x%y;
    x=y;
    y=z;
  }
  return x;
}

float **alloc_mat(int row, int col);
void    init_mat(float **A, int row, int col);
float **mult_mat(float **A, float **B, int d1, int d2, int d3);
void    is_correct(float **A, float **B, int row, int col);

// ----------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    int tasks,                 // Anzahl an Tasks 
        taskid,                // Task ID
        workers, i,            // Anzahl an Arbeitern 
        bsize, bpos,           // Zeilenabschnitt von Matrix A
        averow, extra,         // Berechnung von Zeilenabschnitten 
        X, D1, D2, D3;
    float **A, **B, **C, **D, **TestMat;  // Matrizen
    double time_start, time_end, time0, time1, time2, time3;
    MPI_Status status;         // Statusvariable

    X = 4;

    /* read user input */
    D1 = atoi(argv[1]);		// rows of A and C
    D2 = atoi(argv[2]);     // cols of A and rows of B
    D3 = atoi(argv[3]);     // cols of B and C	

    if(D1 != 1000 || D2 != 1000 || D3 != 1000){
        if(taskid == MASTER){
            printf("\nMit D1 D2 D3 != 1000 ist nur Variante 1 möglich.\n");
        }
        X = 2;
    }

    A = alloc_mat(D1, D2); init_mat(A, D1, D2);    // Speicher f�r Matrizen holen 
    B = alloc_mat(D2, D3); init_mat(B, D2, D3);    // und initialisieren

    TestMat = mult_mat(A, B, D1, D2, D3); 

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    MPI_Comm_size(MPI_COMM_WORLD, &tasks);
    MPI_Request request;

    for (int variante = 1; variante < X; variante++){    
    
        if (tasks < 2 ) {
            printf("Need at least two tasks!\n");
            MPI_Abort(MPI_COMM_WORLD, 0); exit(1);
        }

        //****************************** Master task ************************************
        if (taskid == MASTER) {
            if(D1 != 1000 || D2 != 1000 || D3 != 1000){
                printf("\nMit D1 D2 D3 != 1000 ist nur Variante 1 möglich.\n");
            }       
            
            C = alloc_mat(D1, D3);      

            if (variante == 1){

                workers = tasks-1;
                averow = D1 / workers;                                 // Mittlere Blockgr��e 
                extra = D1 % workers;                                                 // Rest
                
                time_start = MPI_Wtime();
                for (i=1, bpos = 0; i<=workers; i++, bpos += bsize) {
                    if (i > extra) {                // Senden der Matrixdaten an die Arbeiter 
                        bsize = averow;
                    } else {
                        bsize = averow+1;
                    }
                    MPI_Send(&bpos,   1,        MPI_INT,   i, FROM_MASTER, MPI_COMM_WORLD);
                    MPI_Send(&bsize,  1,        MPI_INT,   i, FROM_MASTER, MPI_COMM_WORLD);
                    MPI_Send(A[bpos], bsize*D2, MPI_FLOAT, i, FROM_MASTER, MPI_COMM_WORLD);
                    MPI_Send(B[0],    D2*D3,    MPI_FLOAT, i, FROM_MASTER, MPI_COMM_WORLD);
                }
                for (i=1; i<=workers; i++) {    // Empfangen der Ergebnisse von den Arbeitern 
                    MPI_Recv(&bpos,   1,        MPI_INT,   i, FROM_WORKER, MPI_COMM_WORLD, &status);
                    MPI_Recv(&bsize,  1,        MPI_INT,   i, FROM_WORKER, MPI_COMM_WORLD, &status);
                    MPI_Recv(C[bpos], bsize*D3, MPI_FLOAT, i, FROM_WORKER, MPI_COMM_WORLD, &status);
                }
                time_end = MPI_Wtime();
                time1 = time_end - time_start;
                printf("\nTaking %f seconds with variante 1\n",time1);

                time_start = MPI_Wtime();
                D = mult_mat(A, B, D1, D2, D3);          // Sequentielle Matrixmultiplikation
                time_end = MPI_Wtime();
                time0 = time_end - time_start;
                printf("\nTaking %f seconds with variante squentiell\n",time0);

                is_correct(C, TestMat, D1, D3);                             // Ergebnis ueberpruefen
            }

            if (variante == 2){

                workers = tasks-1;
                averow = D1 / workers;                                 // Mittlere Blockgr��e 
                extra = D1 % workers;                                                 // Rest
                
                time_start = MPI_Wtime();
                for (i=1, bpos = 0; i<=workers; i++, bpos += bsize) {
                    if (i > extra) {                // Senden der Matrixdaten an die Arbeiter 
                        bsize = averow;
                    } else {
                        bsize = averow+1;
                    }
                    MPI_Send(&bpos,   1,        MPI_INT,   i, FROM_MASTER, MPI_COMM_WORLD);
                    MPI_Send(&bsize,  1,        MPI_INT,   i, FROM_MASTER, MPI_COMM_WORLD);
                    MPI_Send(A[bpos], bsize*D2, MPI_FLOAT, i, FROM_MASTER, MPI_COMM_WORLD);
                    MPI_Send(B[0],    D2*D3,    MPI_FLOAT, i, FROM_MASTER, MPI_COMM_WORLD);
                }
                for (i=1; i<=workers; i++) {    // Empfangen der Ergebnisse von den Arbeitern 
                    MPI_Irecv(&bpos,   1,        MPI_INT,   i, FROM_WORKER, MPI_COMM_WORLD, &request);
                    MPI_Irecv(&bsize,  1,        MPI_INT,   i, FROM_WORKER, MPI_COMM_WORLD, &request);
                    MPI_Waitall(2, &request, &status);

                    MPI_Recv(C[bpos], bsize*D3, MPI_FLOAT, i, FROM_WORKER, MPI_COMM_WORLD, &status);
                }

                time_end = MPI_Wtime();
                time2 = time_end - time_start;
                printf("\nTaking %f seconds with variante 2\n",time2);

                is_correct(C, TestMat, D1, D3); 
            }

            if (variante == 3){
                
            }
        }

        //**************************** Arbeiter Task ************************************
        if (taskid > MASTER) {

            if (variante == 1){
                MPI_Recv(&bpos,  1,        MPI_INT,   MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
                MPI_Recv(&bsize, 1,        MPI_INT,   MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
                A = alloc_mat(bsize, D2); 
                B = alloc_mat(D2,    D3); 
                MPI_Recv(A[0],   bsize*D2, MPI_FLOAT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
                MPI_Recv(B[0],   D2*D3,    MPI_FLOAT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);

                C = mult_mat(A, B, bsize, D2, D3);              // Teilmatrizen multiplizieren

                MPI_Send(&bpos,  1,        MPI_INT,   MASTER, FROM_WORKER, MPI_COMM_WORLD);
                MPI_Send(&bsize, 1,        MPI_INT,   MASTER, FROM_WORKER, MPI_COMM_WORLD);
                MPI_Send(C[0],   bsize*D3, MPI_FLOAT, MASTER, FROM_WORKER, MPI_COMM_WORLD);
            }

            if (variante == 2){
                MPI_Irecv(&bpos,  1,        MPI_INT,   MASTER, FROM_MASTER, MPI_COMM_WORLD, &request);
                MPI_Irecv(&bsize, 1,        MPI_INT,   MASTER, FROM_MASTER, MPI_COMM_WORLD, &request);
                MPI_Irecv(B[0],   D2*D3,    MPI_FLOAT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &request);

                MPI_Waitall(3, &request, &status);

                MPI_Recv(A[0],   bsize*D2, MPI_FLOAT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);

                A = alloc_mat(bsize, D2); 
                B = alloc_mat(D2,    D3); 

                C = mult_mat(A, B, bsize, D2, D3);              // Teilmatrizen multiplizieren

                MPI_Send(&bpos,  1,        MPI_INT,   MASTER, FROM_WORKER, MPI_COMM_WORLD);
                MPI_Send(&bsize, 1,        MPI_INT,   MASTER, FROM_WORKER, MPI_COMM_WORLD);
                MPI_Send(C[0],   bsize*D3, MPI_FLOAT, MASTER, FROM_WORKER, MPI_COMM_WORLD);
            }

            if (variante == 3){

            }
        }
    }

    if (taskid == MASTER) {
        printf("\nSpeedup für Variante 1: %f",time0/time1);

        if(X != 2){
            printf("\nSpeedup für Variante 2: %f",time0/time2);
            printf("\nSpeedup für Variante 2: %f",time0/time3);
        }
    }

    MPI_Finalize();
}