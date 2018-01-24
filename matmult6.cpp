// compile in Linux with gcc:
// g++ helloworld hello_world.cpp -lOpenCL -fopenmp

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS // Diese Zeile muss hinzugefügt werden, falls man mit OpenCL 2.0 oder höher arbeitet, da clCreateCommandQueue veraltet ist.
#include "CL/cl.h"                              
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define DIM 1000
#define DATA_SIZE   DIM*DIM*sizeof(float)                          
#define MEM_SIZE    DATA_SIZE*sizeof(float)   

/**  
const char *KernelSource =
  "#define DIM 1000 																																\n"
	"__kernel void matmult (__global float *A, __global float *B, __global float *C)  \n"
	"{									 																															\n"
	"		int i, j, k; 																																	\n"
	"		i = get_global_id(0);																													\n"
	"		for (j = 0; j < DIM;j++)	 																										\n"
	"  		for (k = 0; k < DIM; k++)																				 						\n"
  "				C[i*DIM+j] += A[i*DIM+k] * B[k*DIM+j];																		\n"
	"}					      		                               															\n"
	"																																									\n";
**/



/**
//Aufgabe 1
//Reduktion von Feldzugriffen
const char *KernelSource =
  "#define DIM 1000 																																\n"
	"__kernel void matmult (__global float *A, __global float *B, __global float *C)  \n"
	"{									 																															\n"
	"		int i, j, k; 																																	\n"
	"		float sum;																																		\n"
	"		i = get_global_id(0);																													\n"
	"		for (j = 0; j < DIM;j++){ 																										\n"
	"			sum = 0.0;																																	\n"
	"  		for (k = 0; k < DIM; k++)																				 						\n"
  "				sum += A[i*DIM+k] * B[k*DIM+j];																						\n"
  "			C[i*DIM+j] = sum;																														\n"
  "		}					      		                               														\n"
  "}																																								\n"
	"																																									\n";
**/



/**
//Aufgabe 2
//Getauschte Schleifen
const char *KernelSource =
  "#define DIM 1000 																																\n"
	"__kernel void matmult (__global float *A, __global float *B, __global float *C)  \n"
	"{									 																															\n"
	"		int i, j, k; 																																	\n"
	"		float sum = 0.0;																															\n"
	"		j = get_global_id(0);																													\n"
	"		i = get_global_id(1);																													\n"
	"  	for (k = 0; k < DIM; k++)																				 							\n"
	"			sum += A[i*DIM +k] * B[k*DIM+j];																						\n"
  "		C[i*DIM+j] = sum;																															\n"
  "}					      		                               															\n"
	"																																									\n";
**/



/**
//Aufgabe 3
//Speicheroptimierung
const char *KernelSource =
  "#define DIM 1000 																																\n"
	"__kernel void matmult (__global float *A, __global float *B, __global float *C)  \n"
	"{									 																															\n"
	"		int i, j, k; 																																	\n"
	"		float Al[DIM], sum;																														\n"
	"		i = get_global_id(0);																													\n"
	"  	for (k = 0; k < DIM; k++)																				 							\n"
	"			Al[k] = A[i*DIM+k];																													\n"
	"		for(j = 0; j < DIM; j++) {																										\n"
	"			sum = 0.0;																																	\n"
	"			for(k = 0; k < DIM; k++)																										\n"
	"				sum += Al[k] * B[k*DIM+j];																								\n"
	"			C[i*DIM+j] = sum;																														\n"
	"		}					      		                               														\n"
	"}					      		                               															\n"
	"																																									\n";
**/



/**
//Aufgabe 4
//Verteilte Speicheroptimierung in Arbeitsgruppen
const char *KernelSource =
  "#define DIM 1000 																																\n"
	"__kernel void matmult (__global float *A, __global float *B, __global float *C,  \n"
	"												__local float *Bl)																				\n"
	"{									 																															\n"
	"		int i, j, k; 																																	\n"
	"		float Al[DIM], sum;																														\n"
	"		i = get_global_id(0);																													\n"
	"		int il = get_local_id(0);																											\n"
	"		int nl = get_local_size(0);																										\n"
	"  	for (k = 0; k < DIM; k++)																				 							\n"
	"			Al[k] = A[i*DIM+k];																													\n"
	"		for(j = 0; j < DIM; j++) {																										\n"
	"			for(k = il; k < DIM; k += nl)																								\n"
	"				Bl[k] = B[k*DIM+j];																												\n"
	"			sum = 0.0;																																	\n"
	"			for(k = 0; k < DIM; k++)																										\n"
	"				sum += Al[k] * Bl[k];																											\n"
	"			C[i*DIM+j] = sum;																														\n"
	"		}					      		                               														\n"
	"}					      		                               															\n"
	"																																									\n";
**/


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

// Zufaellige Initialisierung einer Matrix mit den Werten [0..9]. 

void init_mat(float **A, int row, int col) 
{
    for (int i = 0; i < row*col; i++)
        A[0][i] = (float)(rand() % 10);
}

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

// Ausgabe der Matrixelemente fuer Debugzwecke

void print_mat(float **A, int row, int col)
{
    int i, j;

    printf("Matrix C:\n");
    for (i = 0; i < row; i++) {
        for (j = 0; j < col; j++) 
            printf("%6.1f   ", A[i][j]);
        printf("\n"); 
    }
}

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

int main(void)
{
	cl_int 				    err;                      
	cl_platform_id*   platforms = NULL;         
	char			        platform_name[1024];      
	cl_device_id	    device_id = NULL;         
	cl_uint			      num_of_platforms = 0,     
					          num_of_devices = 0;       
	cl_context 			  context;                  
	cl_kernel 			  kernel;                   
	cl_command_queue  command_queue;            
	cl_program 			  program;                  
	cl_mem				    Ap, Bp, Cp;            
	
	size_t				    global[1] ={DIM};  
	float 						**A, **B, **C, **D;
	float 						start, end, time;

	
	A = alloc_mat(DIM, DIM); init_mat(A, DIM, DIM);
	B = alloc_mat(DIM, DIM); init_mat(B, DIM, DIM);
	C = alloc_mat(DIM, DIM);
	D = alloc_mat(DIM, DIM);


	err = clGetPlatformIDs(0, NULL, &num_of_platforms);
	if (err != CL_SUCCESS)
	{
		printf("No platforms found. Error: %d\n", err);
		return 0;
	}

	// 
	platforms = (cl_platform_id *)malloc(num_of_platforms);
	err = clGetPlatformIDs(num_of_platforms, platforms, NULL);
	if (err != CL_SUCCESS)
	{
		printf("No platforms found. Error: %d\n", err);
		return 0;
	}
	else
	{
		//
		int nvidia_platform = 0;

		// 
		for (unsigned int i=0; i<num_of_platforms; i++)
		{
			//
			clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platform_name), platform_name,	NULL);
			if (err != CL_SUCCESS)
			{
				printf("Could not get information about platform. Error: %d\n", err);
				return 0;
			}
			
			// 
			if (strstr(platform_name, "NVIDIA") != NULL)
			{
				nvidia_platform = i;
				break;
			}
		}

		// 
		err = clGetDeviceIDs(platforms[nvidia_platform], CL_DEVICE_TYPE_GPU, 1, &device_id, &num_of_devices);
		if (err != CL_SUCCESS)
		{
			printf("Could not get device in platform. Error: %d\n", err);
			return 0;
		}
	}

	// 
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	if (err != CL_SUCCESS)
	{
		printf("Unable to create context. Error: %d\n", err);
		return 0;
	}

	// 
	command_queue = clCreateCommandQueue(context, device_id, 0, &err);
	if (err != CL_SUCCESS)
	{
		printf("Unable to create command queue. Error: %d\n", err);
		return 0;
	}

	// 
	program = clCreateProgramWithSource(context, 1, (const char **)&KernelSource, NULL, &err);
	if (err != CL_SUCCESS)
	{
		printf("Unable to create program. Error: %d\n", err);
		return 0;
	}

  //
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error building program. Error: %d\n", err);
		return 0;
	}// 
	platforms = (cl_platform_id *)malloc(num_of_platforms);

        

 
kernel = clCreateKernel(program, "matmult", &err);// Spezifiziere Kernel
 if (err != CL_SUCCESS)
	{
		printf("Error setting kernel. Error: %d\n", err);
		return 0;
	}

start = omp_get_wtime();

Ap = clCreateBuffer(context, CL_MEM_READ_ONLY, DATA_SIZE, NULL, &err); // Erzeuge Puffer
Bp = clCreateBuffer(context, CL_MEM_READ_ONLY, DATA_SIZE, NULL, &err);
Cp = clCreateBuffer(context, CL_MEM_WRITE_ONLY, DATA_SIZE, NULL, &err);
 
clEnqueueWriteBuffer(command_queue, Ap, CL_TRUE, 0, DATA_SIZE, A[0], 0, NULL, NULL);
clEnqueueWriteBuffer(command_queue, Bp, CL_TRUE, 0, DATA_SIZE, B[0], 0, NULL, NULL);
 
clSetKernelArg(kernel, 0, sizeof(cl_mem), &Ap);// Setzte die Argumentliste für Kernel
clSetKernelArg(kernel, 1, sizeof(cl_mem), &Bp);
clSetKernelArg(kernel, 2, sizeof(cl_mem), &Cp);
// ...
clFinish(command_queue);
clEnqueueNDRangeKernel (command_queue, kernel, 1, NULL, global, NULL, 0, NULL, NULL);
clEnqueueReadBuffer(command_queue, Cp, CL_TRUE, 0, DATA_SIZE, C[0], 0, NULL, NULL);

 end = omp_get_wtime();
 time = end-start;
 printf("Laufzeit: %f\n",time);
        

	D = mult_mat(A, B, 1000, 1000, 1000);
	is_correct(C,D, 1000, 1000);

	/* 4) */
	clReleaseMemObject(Ap);
	clReleaseMemObject(Bp);
	clReleaseMemObject(Cp);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);

	return 0;

    
}
