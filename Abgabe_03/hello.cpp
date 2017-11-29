#include <stdio.h>
#include <mpi.h>	//bindet mpi.h ein sodass MPI Funktionen benutzt werden können 

int main(int argc, char** argv)
{
	int nodeID;
	int numNodes;

	MPI_Init(&argc, &argv); //Initialisiert das MPI-Laufzeitsystem mit den 
													//Kommandozeilenparametern des Hauptprogramms

	MPI_Comm_size(MPI_COMM_WORLD, &numNodes); //Liefert die Größe des Kommunikators MPI_Comm_World
																						//d.h. die Anzahl der Prozesse innerhalb des Kommunikators
																						//und speichert sie in der Variable numNodes

	MPI_Comm_rank(MPI_COMM_WORLD, &nodeID); //Bestimmt den Rang eines Prozesses innerhalb
																				 	//des Kommunikators MPI_COMM_WORLD und speichert
																					//ihn in der Variable nodeID

	printf("Hello world from process %d of %d\n", nodeID, numNodes);
	//Jeder erzeugte Prozess gibt auf der Konsole aus um den wievielten Prozess(MPI_Comm_rank -> nodeID) aus
	//der Anzahl aller erzeugten Prozesse(MPI_Comm_rank -> numNodes) es sich gerade handelt

	MPI_Finalize(); //Meldet den Prozess beim MPI_Laufzeitsystem ab, ab hier ist
									//kein Aufruf von MPI-Funktionen mehr erlaubt
	return 0;
}