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

void quicksort_parallel(float *v, int start, int end, int depth) 
{
    int i = start, j = end;
    float pivot;
    depth++;

    if(depth < 8){
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
    if (start < j)                                        // Teile und herrsche
        #pragma omp task
        quicksort_parallel(v, start, j, depth);                      // Linkes Segment zerlegen
    if (i < end)
        #pragma omp task
        quicksort_parallel(v, i, end, depth);                       // Rechtes Segment zerlegen

    }else
    {
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
    if (start < j)                                        // Teile und herrsche
        quicksort_parallel(v, start, j, depth);                      // Linkes Segment zerlegen
    if (i < end)
        quicksort_parallel(v, i, end, depth);                       // Rechtes Segment zerlegen
    }
}

// ---------------------------------------------------------------------------
// Serielle Version von Quicksort (Wirth) 

void quicksort(float *v, int start, int end) 
{
    int i = start, j = end;
    float pivot;

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
   if (start < j)                                        // Teile und herrsche
       quicksort(v, start, j);                      // Linkes Segment zerlegen
   if (i < end)
       quicksort(v, i, end);                       // Rechtes Segment zerlegen
}

// ---------------------------------------------------------------------------
// Hauptprogramm

int main(int argc, char *argv[])
{
    float *v;                                                         // Feld
    float *w;                            
    int iter;                                                // Wiederholungen

    float time_parallel = 0;
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

    printf("Perform vector sorting %d times...\n", iter);

    for (int i = 0; i < iter; i++) {               // Wiederhole das Sortieren
        for (int j = 0; j < NUM; j++){      // Mit Zufallszahlen initialisieren
            temp = (float)rand();
            v[j] = temp;
            w[j] = temp;
        }
        start = omp_get_wtime();
        quicksort(v, 0, NUM-1);                              // Sortierung
        end = omp_get_wtime();
        time += end - start;

        start = omp_get_wtime();
        quicksort_parallel(w, 0, NUM-1,0);
        end = omp_get_wtime();
        time_parallel += end - start;

        for (int j = 0; j < NUM; j++){
            if(v[j] != w[j])
            equal = false;
        }
    }
    printf("\nParallel in %f seconds.",time_parallel);
    printf("\nNonparallel in %f seconds.",time);
    printf ("\nSpeedup: %f\n", time/time_parallel);

    if(equal == true)
    printf("\nQuicksort is working\n");
    return 0;
}
