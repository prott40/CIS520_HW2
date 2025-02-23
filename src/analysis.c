#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// Include headers: dyn_array, processing_scheduling
#include "dyn_array.h"
#include "processing_scheduling.h"

#define FCFS "FCFS"
#define P "P"
#define RR "RR"
#define SJF "SJF"


int main(int argc, char **argv) 
{
    // check arg count
    if (argc < 3) 
    {
        printf("%s <pcb file> <schedule algorithm> [quantum]\n", argv[0]);
        return EXIT_FAILURE;
    }
    // get time for read me
    time_t rawtime;
    struct tm *timeinfo;
    char time_buffer[80];
    // convert time 
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    // convert to string for printing
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    

    //takes file name and adds space for ../ and \n opperatort to read
    size_t file_name_length = strnlen(argv[1],20) + 3 + 1;
    // create the space for the file
    char * file_name = malloc(file_name_length);
    //check for failed file allocation
    if(!file_name){
        fprintf(stderr, "%s:%d failed file name malloc\n", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }
    // concatinate the file name 
    snprintf(file_name,file_name_length,"../%s",argv[1]);
    // Allocate memory for FCFS scheduler results
    ScheduleResult_t *FCFS_Result = (ScheduleResult_t *)malloc(sizeof(ScheduleResult_t));
    //check for failed allocation
    if (!FCFS_Result) 
    {
        fprintf(stderr, "%s:%d failed fcfs result malloc fail\n", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }
    // Load the process control blocks from the binary file
    dyn_array_t *binArray = load_process_control_blocks(file_name);
    // check if binary file allocation failed
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
    // from https://stackoverflow.com/questions/19429138/append-to-the-end-of-a-file-in-c
    FILE *pFile = fopen("../README.md", "a");
    // check if th file opens
    if (!pFile) 
    {
        fprintf(stderr, "%s:%d readme failed to open \n", __FILE__, __LINE__);
        dyn_array_destroy(binArray);
        free(FCFS_Result);
        return EXIT_FAILURE;
    }
    // Write results to README.md
    fprintf(pFile,"---------%s-----------\n",time_buffer);
    fprintf(pFile, "Average wait time: %.2f\n", FCFS_Result->average_waiting_time);
    fprintf(pFile, "Average turnaround time: %.2f\n", FCFS_Result->average_turnaround_time);
    fprintf(pFile, "Total run time: %lu\n", FCFS_Result->total_run_time);
    fprintf(pFile,"---------------------------------------\n");
    // close file
    fclose(pFile);
    // release all memory 
    dyn_array_destroy(binArray);
    free(FCFS_Result);
    free(file_name);

    return EXIT_SUCCESS;
}
