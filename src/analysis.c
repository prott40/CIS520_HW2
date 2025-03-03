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
    int alg;
    if(strncmp(argv[2],FCFS,4)==0){
        alg = 0;
    }
    else if(strncmp(argv[2],P,1)==0){
        alg = 1;
    }
    else if(strncmp(argv[2],RR,1)==0){
        alg = 2;
    }
    else if(strncmp(argv[2],SJF,1)==0){
        alg = 3;
    }
	else{
		alg = 4;
	}
    int quanta;
    if (sscanf(argv[3], "%d", &quanta) != 1) {
        fprintf(stderr, "Invalid quanta: %s\n", argv[3]);
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
    ScheduleResult_t *Result = (ScheduleResult_t *)malloc(sizeof(ScheduleResult_t));
    //check for failed allocation
    if (!Result) 
    {
        fprintf(stderr, "%s:%d failed fcfs result malloc fail\n", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }
    // Load the process control blocks from the binary file
    dyn_array_t *binArray = load_process_control_blocks(file_name);
    // check if binary file allocation failed
    if (!binArray) 
    {
        fprintf(stderr, "%s:%d failed first come first serve\n", __FILE__, __LINE__);
        free(Result);
        return EXIT_FAILURE;
    }
    if(alg == 0){
        // Perform the FCFS scheduling
        
        // Check if the scheduling was successful
        if (first_come_first_serve(binArray, Result)) 
        {
            fprintf(stderr, "%s:%d passed fcfsy \n", __FILE__, __LINE__);
        } 
        else 
        {
            fprintf(stderr, "%s:%d failed load procces control block\n", __FILE__, __LINE__);
            dyn_array_destroy(binArray);
            free(Result);
            return EXIT_FAILURE;
        }
    }
    else if(alg ==1){
        if(priority(binArray, Result)){
            fprintf(stderr, "%s:%d passed priority \n", __FILE__, __LINE__);
        }
        else{
            fprintf(stderr, "%s:%d failed priorityk\n", __FILE__, __LINE__);
            dyn_array_destroy(binArray);
            free(Result);
            return EXIT_FAILURE;
        }
    }
    else if(alg == 2){
		if (round_robin(binArray, Result)) 
        {
            fprintf(stderr, "%s:%d passed rr \n", __FILE__, __LINE__);
        } 
        else 
        {
            fprintf(stderr, "%s:%d failed round robin\n", __FILE__, __LINE__);
            dyn_array_destroy(binArray);
            free(Result);
            return EXIT_FAILURE;
        }
    }
    else if(alg ==3){
        
        if (shortest_job_first(binArray, Result)) 
        {
            fprintf(stderr, "%s:%d passed sjf \n", __FILE__, __LINE__);
        } 
        else 
        {
            fprintf(stderr, "%s:%d failed shortest job fisrt\n", __FILE__, __LINE__);
            dyn_array_destroy(binArray);
            free(Result);
            return EXIT_FAILURE;
        }
    }
	else{
		if (shortest_remaining_time_first(binArray, Result)) 
        {
            fprintf(stderr, "%s:%d passed srjf \n", __FILE__, __LINE__);
        } 
        else 
        {
            fprintf(stderr, "%s:%d failed shortest remaing job first\n", __FILE__, __LINE__);
            dyn_array_destroy(binArray);
            free(Result);
            return EXIT_FAILURE;
        }
	}
   

    // Open the README.md file for appending results
    // from https://stackoverflow.com/questions/19429138/append-to-the-end-of-a-file-in-c
    FILE *pFile = fopen("../README.md", "a");
    // check if th file opens
    if (!pFile) 
    {
        fprintf(stderr, "%s:%d readme failed to open \n", __FILE__, __LINE__);
        dyn_array_destroy(binArray);
        free(Result);
        return EXIT_FAILURE;
    }
    // Write results to README.md
    fprintf(pFile,"---------%s %s-----------\n",time_buffer,argv[2]);
    fprintf(pFile, "Average wait time: %.2f\n", Result->average_waiting_time);
    fprintf(pFile, "Average turnaround time: %.2f\n", Result->average_turnaround_time);
    fprintf(pFile, "Total run time: %lu\n", Result->total_run_time);
    fprintf(pFile,"---------------------------------------\n");
    // close file
    fclose(pFile);
    // release all memory 
    dyn_array_destroy(binArray);
    free(Result);
    free(file_name);

    return EXIT_SUCCESS;
}
