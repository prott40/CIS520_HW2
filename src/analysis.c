#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    // added need ../ to access the file wanted and \n at end of string
    size_t file_name_length = strnlen(argv[1],20) + 3 + 1;

    char * file_name = malloc(file_name_length);
    if(!file_name){
        fprintf(stderr, "%s:%d failed file name malloc\n", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }
    
    snprintf(file_name,file_name_length,"../%s",argv[1]);
    // Allocate memory for FCFS scheduler results
    ScheduleResult_t *FCFS_Result = (ScheduleResult_t *)malloc(sizeof(ScheduleResult_t));
    if (!FCFS_Result) 
    {
        fprintf(stderr, "%s:%d failed fcfs result malloc fail\n", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    // Load the process control blocks from the binary file
    dyn_array_t *binArray = load_process_control_blocks(file_name);
    if (!binArray) 
    {
        fprintf(stderr, "%s:%d failed load procces control block\n", __FILE__, __LINE__);
        free(FCFS_Result);
        return EXIT_FAILURE;
    }

    // Perform the FCFS scheduling
    bool res = first_come_first_serve(binArray, FCFS_Result);

    // Check if the scheduling was successful
    if (res) 
    {
        fprintf(stderr, "%s:%d passed sorting array \n", __FILE__, __LINE__);
    } 
    else 
    {
        fprintf(stderr, "%s:%d failed load procces control block\n", __FILE__, __LINE__);
        dyn_array_destroy(binArray);
        free(FCFS_Result);
        return EXIT_FAILURE;
    }

    // Open the README.md file for appending results
    FILE *pFile = fopen("../README.md", "a");
    if (!pFile) 
    {
        fprintf(stderr, "%s:%d readme failed to open \n", __FILE__, __LINE__);
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
    free(file_name);

    return EXIT_SUCCESS;
}
