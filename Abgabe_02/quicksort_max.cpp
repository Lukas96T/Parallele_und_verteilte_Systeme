#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NUM 32767                                             // Elementanzahl

// ---------------------------------------------------------------------------
// Vertausche zwei Zahlen im Feld v an der Position i und j

void swap(float *v, int i, int j)
{
    float t = v[i]; 
    v[i] = v[j];
    v[j] = t;
}

// ---------------------------------------------------------------------------
// Parallele Version von Quicksort (Wirth) 

void quicksort(float *v, int start, int end, int depth) 
{
    int i = start, j = end;
    float pivot;
    depth--;

    pivot = v[(start + end) / 2];                         // mittleres Element
    do {
        while (v[i] < pivot)
            i++;
        while (pivot < v[j])
            j--;
        if (i <= j) {               // wenn sich beide Indizes nicht beruehren
            swap(v, i, j);
            i++;
            j--;
        }
    } while (i <= j);
    if (start < j)
        if(depth > 0){                                      // Teile und herrsche
            #pragma omp task                        //alternativ mit #pragma omp parallel
            quicksort(v, start, j, depth);                      // Linkes Segment zerlegen
        }else{
            quicksort(v, start, j, 0);
        }              
    if (i < end)
        if(depth > 0){
            #pragma omp task                        //alternativ mit #pragma omp parallel
            quicksort(v, i, end, depth);                       // Rechtes Segment zerlegen
        }else{
            quicksort(v, i, end, 0); 
        }
}


// ---------------------------------------------------------------------------
// Hauptprogramm

int main(int argc, char *argv[])
{
    float *v;                                                         // Feld
    float *w;
    float *x;
    float *y;
    int iter;                                                // Wiederholungen

    float time_parallel_1 = 0;
    float time_parallel_2 = 0;
    float time_parallel_3 = 0;
    float time = 0;
    float start = 0;
    float end = 0;  
    float temp = 0;
    bool equal = true;      

    if (argc != 2) {                                      // Benutzungshinweis
        printf ("Vector sorting\nUsage: %s <NumIter>\n", argv[0]); 
        return 0;
    }
    iter = atoi(argv[1]);                               
    v = (float *) calloc(NUM, sizeof(float));        // Speicher reservieren
    w = (float *) calloc(NUM, sizeof(float));
    x = (float *) calloc(NUM, sizeof(float));
    y = (float *) calloc(NUM, sizeof(float));

    printf("Perform vector sorting %d times...\n", iter);


    //pragma omp for                        als Variante für Parallelisierung, würde allerdings 
                                            //nicht quicksort selbst parallelisieren, sondern nur die 
                                            //einzelnen seriellen quicksorts parallel aufrufen
    for (int i = 0; i < iter; i++) {        // Wiederhole das Sortieren
        for (int j = 0; j < NUM; j++){      // Mit Zufallszahlen initialisieren
            temp = (float)rand();
            v[j] = temp;
            w[j] = temp;
            x[j] = temp;
            y[j] = temp;
        }

        start = omp_get_wtime();
        quicksort(v, 0, NUM-1,0);                              // Sortierung
        end = omp_get_wtime();
        time += end - start;

        start = omp_get_wtime();
        #pragma omp parallel
        {
            #pragma omp single nowait
            {
                quicksort(w, 0, NUM-1,2); //festgelegte Tiefe = 2, wie oft parallelsiert werden soll
            }                             //geringer Speedup, weil nicht oft genug parallelisiert wird
        }
        end = omp_get_wtime();
        time_parallel_1 += end - start;

        start = omp_get_wtime();
        #pragma omp parallel
        {
            #pragma omp single nowait
            {
                quicksort(x, 0, NUM-1,10); //festgelegte Tiefe = 10, wie oft parallelsiert werden soll
            }                              //hoher Speedup, weil viel parallelsiert wird, aber das er-
        }                                  //stellen der Threads nicht zu zeitaufwändig wird
        end = omp_get_wtime();
        time_parallel_2 += end - start;

        start = omp_get_wtime();
        #pragma omp parallel
        {
            #pragma omp single nowait
            {
                quicksort(y, 0, NUM-1,100); //festgelegte Tiefe = 100, wie oft parallelsiert werden soll
            }                               //geringer Speedup, weil die erstellung so vieler Threads zu
        }                                   //zeitaufwändig ist
        end = omp_get_wtime();
        time_parallel_3 += end - start;

        for (int j = 0; j < NUM; j++){
            if(v[j] != x[j] || v[j] != w[j] || v[j] != y[j])
            equal = false;
        }
    }
    printf("\nNonparallel in %f seconds.\n",time);
    printf("\nParallel with depth 2 in %f seconds.",time_parallel_1);
    printf ("\nSpeedup: %f\n", time/time_parallel_1);
    printf("\nParallel with depth 10 in %f seconds.",time_parallel_2);
    printf ("\nSpeedup: %f\n", time/time_parallel_2);
    printf("\nParallel with depth 100 in %f seconds.",time_parallel_3);
    printf ("\nSpeedup: %f\n", time/time_parallel_3);

    if(equal == true)
        printf("\nQuicksort is working\n");
    return 0;
}