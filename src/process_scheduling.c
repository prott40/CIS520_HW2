#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dyn_array.h"
#include "processing_scheduling.h"


// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

// private function
void virtual_cpu(ProcessControlBlock_t *process_control_block) 
{
	// decrement the burst time of the pcb
	--process_control_block->remaining_burst_time;
}
// Runs the First Come First Served Process Scheduling algorithm over the incoming ready_queue
// \param ready queue a dyn_array of type ProcessControlBlock_t that contain be up to N elements
// \param result used for first come first served stat tracking \ref ScheduleResult_t
// \return true if function ran successful else false for an error
bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	if(ready_queue == NULL || result == NULL){
		fprintf(stderr, "%s:%d invalid param\n", __FILE__,__LINE__);
        return NULL;
	}

	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}

bool shortest_job_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}

bool priority(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}

bool round_robin(dyn_array_t *ready_queue, ScheduleResult_t *result, size_t quantum) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	UNUSED(quantum);
	return false;
}
// Reads the PCB burst time values from the binary file into ProcessControlBlock_t remaining_burst_time field
// for N number of PCB burst time stored in the file.
// \param input_file the file containing the PCB burst times
// \return a populated dyn_array of ProcessControlBlocks if function ran successful else NULL for an error
dyn_array_t *load_process_control_blocks(const char *input_file) 
{
    // Check parameters to ensure that it is valid
    if (input_file == NULL) {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return NULL;
    }

    // Open the binary file for reading
    FILE *ptr = fopen(input_file, "rb");  // r for read, b for binary
	// chekc if read has failed
    if (ptr == NULL) {
        fprintf(stderr, "%s:%d error opening file\n", __FILE__, __LINE__);
        return NULL;
    }

    // Read the process control block count
    uint32_t pcb_count;
	// read the  pcb count from the file 
    if (fread(&pcb_count, sizeof(uint32_t), 1, ptr) != 1) {
        fprintf(stderr, "%s:%d error reading file\n", __FILE__, __LINE__);
        fclose(ptr); // Close the file before returning
        return NULL;
    }

    // Create the process control block struct for the size of count
    ProcessControlBlock_t *pc = malloc(sizeof(ProcessControlBlock_t) * pcb_count);
	// if the allocation has failed
    if (!pc) {
        fprintf(stderr, "%s:%d error allocating memory\n", __FILE__, __LINE__);
        fclose(ptr); // Close the file before returning
        return NULL;
    }

    // Loop over allocated memory and read the values into the struct fields
    for (uint32_t i = 0; i < pcb_count; ++i) {
		// iterate over memory and save the remainng burst time for each loaction
        if (fread(&pc[i].remaining_burst_time, sizeof(uint32_t), 1, ptr) != 1) {
            fprintf(stderr, "%s:%d error reading burst time for PCB %u\n", __FILE__, __LINE__, i);
            free(pc);
            fclose(ptr);
            return NULL;
        }

        // Initialize other fields with default values
        pc[i].priority = 0;
        pc[i].arrival = 0;
        pc[i].started = false;
    }

    // Close the file after reading all data
    fclose(ptr);

    // Create the dynamic array to return based on the data
    dyn_array_t *dyn_array = dyn_array_import(pc, pcb_count, sizeof(ProcessControlBlock_t), free);
	// if failed import
    if (!dyn_array) {
        fprintf(stderr, "%s:%d error creating dynamic array\n", __FILE__, __LINE__);
        free(pc);  // Free the temporary memory before returning
        return NULL;
    }

    // Free the temporary static array as its data has been imported
    free(pc);

    return dyn_array;
}

bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}
