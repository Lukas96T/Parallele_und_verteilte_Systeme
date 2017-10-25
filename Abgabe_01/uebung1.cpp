#include <stdio.h> //C Standard Input und Output Library wird eingebunden um eine Ausgabe auf dem Terminal zu ermöglichen
#include <omp.h> //omp.h wird eingebunden um OpenMP-Befehle zu ermöglichen

//Aufgabe 1.1:

int main(int argc, char* argv[])
{
	int numThreads; //int-Variable für die Anzahl der Threads
	int threadID;	//int-Variable für die Nummer eines erzeugten Threads
	float start; //float-Variable für die Startzeit des Programmes
	float end;	//float-Variable für die Endzeit des Programmes

	start = omp_get_wtime();	//Die Startzeit des Programmes wird durch die OpenMP Methode omp_get_wtime() festgelegt

	/**
	Der Block in den Klammern nach dem #pragma-Statement wird auf so vielen Threads wie bei num_threads() festgelegt wird
	parallel ausgeführt
	*/
	#pragma omp parallel num_threads(8)
	{
		threadID = omp_get_thread_num(); //threadID wird die Nummer des Threads zugewiesen in dem die Methode omp_get_thread_num() aufgerufen wird
		printf("Hello from thread %d\n", threadID);

		/**
		Falls die threadID gleich 0 ist, dass heißt es wird nur in das if-Statement gegangen, falls es auf dem Master-Thread aufgerufen wird, 
		wird Anzahl der erzeugten Threads auf der Konsole ausgegeben
		*/
		if(threadID == 0)
		{
			numThreads = omp_get_num_threads();	//numThreads wird die Anzahl der derzeit laufenden Threads zugewiesen
			printf("Number of threads: %d\n", numThreads);
		}
	}

	end = omp_get_wtime(); //Die Endzeit des Programmes wird durch die OpenMP Methode omp_get_wtime() festgelegt
	printf("This task took %f seconds\n", end - start);

	return 0;
}

/**
Aufgabe 1.3:


*/