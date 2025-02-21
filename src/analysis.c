#include <stdio.h>
#include <stdlib.h>

//include headers: dyn_array, processing_scheduling
#include "dyn_array.h"
#include "processing_scheduling.h"

#define FCFS "FCFS"
#define P "P"
#define RR "RR"
#define SJF "SJF"

// Add and comment your analysis code in this function.
// THIS IS NOT FINISHED.
int main(int argc, char **argv) 
{
	if (argc < 3) 
	{
		printf("%s <pcb file> <schedule algorithm> [quantum]\n", argv[0]);
		return EXIT_FAILURE;
	}

	//make fcfs scheduler
	ScheduleResult_t * FCFS_Result = (ScheduleResult_t*)malloc(sizeof(ScheduleResult_t));

	//instantiate dyn_array
		//(might need to be made a pointer)
	dyn_array_t * binArray = (dyn_array_t*)malloc(sizeof(dyn_array_t));


	//instantiate binary and set array to the result given the file path
		//Find file path from input args
	binArray = load_process_control_blocks(argv[0]);

	//instantiate FCFS sorter and pass binary info to it
	bool first_come_first_serve(binArray, FCFS_Result)

	//maybe error check using boolean


	//send data to README
		//open file using fopen("filepath.txt","a");
		//append info to the end using fwrite(stuff to write,"a")
		//fclose(objectName) at the end
		FILE *pFile;

		//opens README and appends next stuff to the end

			!!!!!FIGURE OUT EXACT FILE PATH!!!!!
			
		pFile = fopen("README.md","a");

		//float average_waiting_time;	 // the average waiting time in the ready queue until first schedue on the cpu
		//float average_turnaround_time;  // the average completion time of the PCBs
		//unsigned long total_run_time;   // the total time to process all the PCBs in the ready queue
		
		fprintf(pFile,"Average wait time: ");
		fprintf(pFile, FCFS_Result->average_waiting_time);

		fprintf(pFile,"Average turnaround time: ");
		fprintf(pFile, FCFS_Result->average_turnaround_time);

		fprintf(pFile,"Total run time: ");
		fprintf(pFile, FCFS_Result->total_run_time);

		fclose(pFile);

	return EXIT_SUCCESS;
}
