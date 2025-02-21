#include <stdio.h>
#include <stdlib.h>

// Include headers: dyn_array, processing_scheduling
#include "dyn_array.h"
#include "processing_scheduling.h"

#define FCFS "FCFS"
#define P "P"
#define RR "RR"
#define SJF "SJF"

// Add and comment your analysis code in this function.
int main(int argc, char **argv) 
{
    if (argc < 3) 
    {
        printf("%s <pcb file> <schedule algorithm> [quantum]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Allocate memory for FCFS scheduler results
    ScheduleResult_t *FCFS_Result = (ScheduleResult_t *)malloc(sizeof(ScheduleResult_t));
    if (!FCFS_Result) 
    {
        fprintf(stderr, "Error: Memory allocation failed for FCFS_Result.\n");
        return EXIT_FAILURE;
    }

    // Load the process control blocks from the binary file
    dyn_array_t *binArray = load_process_control_blocks(argv[0]);
    if (!binArray) 
    {
        fprintf(stderr, "Error: Failed to load process control blocks from file %s.\n", argv[1]);
        free(FCFS_Result);
        return EXIT_FAILURE;
    }

    // Perform the FCFS scheduling
    bool res = first_come_first_serve(binArray, FCFS_Result);

    // Check if the scheduling was successful
    if (res) 
    {
        printf("Scheduling completed successfully.\n");
    } 
    else 
    {
        fprintf(stderr, "Scheduling failed.\n");
        dyn_array_destroy(binArray);
        free(FCFS_Result);
        return EXIT_FAILURE;
    }

    // Open the README.md file for appending results
    FILE *pFile = fopen("README.md", "a");
    if (!pFile) 
    {
        fprintf(stderr, "Error: Failed to open README.md for writing.\n");
        dyn_array_destroy(binArray);
        free(FCFS_Result);
        return EXIT_FAILURE;
    }

    // Write results to README.md
    fprintf(pFile, "Average wait time: %.2f\n", FCFS_Result->average_waiting_time);
    fprintf(pFile, "Average turnaround time: %.2f\n", FCFS_Result->average_turnaround_time);
    fprintf(pFile, "Total run time: %lu\n", FCFS_Result->total_run_time);

    fclose(pFile);

    // Clean up
    dyn_array_destroy(binArray);
    free(FCFS_Result);

    return EXIT_SUCCESS;
}
