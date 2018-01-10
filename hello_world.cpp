// compile in Linux with gcc:
// g++ hello_world.cpp -lOpenCL


#include "CL/cl.h"                              //includiert OpenCL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS // Muss hinzugefügt werden, falls man mit OpenCL 2.0 oder höher arbeiten will, da clCreateCommandQueu verarltet ist
#define DATA_SIZE   10                          // Legt die Menge der Daten fest
#define MEM_SIZE    DATA_SIZE * sizeof(float)   // Legt die Größe des benötigten Speichers fest

/** **/ 					//Quelltext des Kernels
const char *KernelSource =
	"#define DATA_SIZE 10	                                              	\n"
	"__kernel void test(__global float *input, __global float *output)  	\n"
	"{									\n"
	"	size_t i = get_global_id(0);					\n"
	"	output[i] = input[i] * input[i];				\n"
	"}									\n"
	"\n";

/** **/
int main (void)
{
	cl_int 			err;                      // Fehlernummer
	cl_platform_id*   	platforms = NULL;         // Zeiger auf ID der Platform
	char			platform_name[1024];      // Name der Platform	
	cl_device_id	    	device_id = NULL;         // ID des Device
	cl_uint			num_of_platforms = 0,     // Anzahl der Platformen
				num_of_devices = 0;       // Anzahl der Devices
	cl_context 			  context;                  	// Der Kontext
	cl_kernel 			  kernel;                   	// Der Kernel für den Quelltext von oben
	cl_command_queue  		  command_queue;		// Die Kommand queue 
	cl_program 			  program;                  	// Das gebuildete Programm
	cl_mem				  input, output;            	// Speicher für Input und Output
	float				  data[DATA_SIZE] =         	// Array für die zu berechnenden Daten
	                    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	size_t				  global[1] = {DATA_SIZE};  	// Array das DATA_SIZE speichert
	float				  results[DATA_SIZE] = {0}; 	// das Array für das Ergebnis

	/* 1) Testet verschiedene mögliche Fehler, bevor das Programm ausgeführt werden kann */

	// Gibt einen Fehler aus, wenn keine Platform gefunden wurde
	err = clGetPlatformIDs(0, NULL, &num_of_platforms);
	if (err != CL_SUCCESS)
	{
		printf("No platforms found. Error: %d\n", err);
		return 0;
	}

	// Gibt einen Fehler aus, wenn keine Platform gefunden wurde
	platforms = (cl_platform_id *)malloc(num_of_platforms);
	err = clGetPlatformIDs(num_of_platforms, platforms, NULL);
	if (err != CL_SUCCESS)
	{
		printf("No platforms found. Error: %d\n", err);
		return 0;
	}
	else
	{
		// Legt eine Platformnummer an
		int nvidia_platform = 0;

		// Geht alle möglichen verfügbaren Platformen durch
		for (unsigned int i=0; i<num_of_platforms; i++)
		{
			// Gibt Platforminfo zurück oder gibt einen Fehler aus
			clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platform_name), platform_name,	NULL);
			if (err != CL_SUCCESS)
			{
				printf("Could not get information about platform. Error: %d\n", err);
				return 0;
			}
			
			// Setzt die Platformnummer auf die Nummer der NVIDIA Platform 
			if (strstr(platform_name, "NVIDIA") != NULL)
			{
				nvidia_platform = i;
				break;
			}
		}

		// Prüft, ob das Device der NVIDIA Platform funktioniert
		err = clGetDeviceIDs(platforms[nvidia_platform], CL_DEVICE_TYPE_GPU, 1, &device_id, &num_of_devices);
		if (err != CL_SUCCESS)
		{
			printf("Could not get device in platform. Error: %d\n", err);
			return 0;
		}
	}

	// Prüft, ob der Kontext erstellt werden kann
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	if (err != CL_SUCCESS)
	{
		printf("Unable to create context. Error: %d\n", err);
		return 0;
	}

	// Prüft, ob die command queue erstellt werden kann
	command_queue = clCreateCommandQueue(context, device_id, 0, &err);
	if (err != CL_SUCCESS)
	{
		printf("Unable to create command queue. Error: %d\n", err);
		return 0;
	}

	// Prüft, ob das Programm aus dem Quelltext erstellt werden kann
	program = clCreateProgramWithSource(context, 1, (const char **)&KernelSource, NULL, &err);
	if (err != CL_SUCCESS)
	{
		printf("Unable to create program. Error: %d\n", err);
		return 0;
	}

  	// Prüft, ob das Programm gebuildet werden kann
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error building program. Error: %d\n", err);
		return 0;
	}

	// Prüft, ob der Kernel erzeugt werden kann
	kernel = clCreateKernel(program, "test", &err);
	if (err != CL_SUCCESS)
	{
		printf("Error setting kernel. Error: %d\n", err);
		return 0;
	}


	/* 2) */

	// Erzeugt einen Input- und Output-Buffer
	input  = clCreateBuffer (context, CL_MEM_READ_ONLY, MEM_SIZE, NULL, &err);
	output = clCreateBuffer (context, CL_MEM_WRITE_ONLY, MEM_SIZE, NULL, &err);

	// Kopiert die Daten von data in input
	clEnqueueWriteBuffer(command_queue, input, CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);

	// Legt die Reihenfolge der Kernelargumente fest
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);


	/* 3) Starten der eigentlichen Berechnung und gibt das Ergebnis aus */

	// Fügt den Kernel in die globale Befehlsqueue ein
	clEnqueueNDRangeKernel (command_queue, kernel, 1, NULL, global, NULL, 0, NULL, NULL);

	// Wartet auf Beenden der Operationen
	clFinish(command_queue);

	// Kopiert den Inhalt von output in result
	clEnqueueReadBuffer(command_queue, output, CL_TRUE, 0, MEM_SIZE, results, 0, NULL, NULL);

	// Gibt das Ergebnis aus result aus
	for (unsigned int i=0; i < DATA_SIZE; i++)
		printf("%f\n", results[i]);


	/* 4) Gibt die OpenCL Resourcen frei */
	clReleaseMemObject(input);
	clReleaseMemObject(output);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);

	return 0;
}

