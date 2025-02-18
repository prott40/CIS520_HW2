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
    // Validate inputs
    if (!ready_queue || !result || dyn_array_size(ready_queue) == 0) {
        return false;  // Return false for invalid input
    }

    // Initialize statistics
    unsigned long total_run_time = 0;
    float total_wait_time = 0;
    float total_turnaround_time = 0;

    // Iterate over all processes in the ready_queue
    uint32_t current_time = 0;  // Keeps track of when the CPU is available for the next process

    // Assuming each PCB is of type ProcessControlBlock_t
    for (size_t i = 0; i < dyn_array_size(ready_queue); ++i) {
        ProcessControlBlock_t *pcb = (ProcessControlBlock_t *)dyn_array_get(ready_queue, i);

        if (!pcb) {
            return false;  // If PCB is NULL, return false
        }

        // Calculate wait time: it's the difference between the current time and the arrival time
        uint32_t wait_time = (current_time >= pcb->arrival) ? (current_time - pcb->arrival) : 0;

        // Calculate turnaround time: it's the wait time + the burst time of the process
        uint32_t turnaround_time = wait_time + pcb->remaining_burst_time;

        // Update the statistics
        total_wait_time += wait_time;
        total_turnaround_time += turnaround_time;
        total_run_time += pcb->remaining_burst_time;

        // Update the current time: when this process finishes
        current_time += pcb->remaining_burst_time;
    }

    // Calculate averages
    result->average_waiting_time = total_wait_time / dyn_array_size(ready_queue);
    result->average_turnaround_time = total_turnaround_time / dyn_array_size(ready_queue);
    result->total_run_time = total_run_time;

    return true;  // Success
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
dyn_array_t *load_process_control_blocks(const char *input_file) {
    // Check parameters to ensure that it is valid
    if (input_file == NULL) {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return NULL;
    }

    // Open the binary file for reading
    FILE *ptr = fopen(input_file, "rb");  // r for read, b for binary
    if (ptr == NULL) {
        fprintf(stderr, "%s:%d error opening file\n", __FILE__, __LINE__);
        return NULL;
    }

    // Read the process control block count
    uint32_t pcb_count;
    if (fread(&pcb_count, sizeof(uint32_t), 1, ptr) != 1) {
        fprintf(stderr, "%s:%d error reading PCB count\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }

    // Create the process control block array
    ProcessControlBlock_t *pc = malloc(sizeof(ProcessControlBlock_t) * pcb_count);
    if (!pc) {
        fprintf(stderr, "%s:%d error allocating memory\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }

    // Loop over the PCB count and read all fields for each process
    for (uint32_t i = 0; i < pcb_count; ++i) {
        // Read the remaining burst time
        if (fread(&pc[i].remaining_burst_time, sizeof(uint32_t), 1, ptr) != 1) {
            fprintf(stderr, "%s:%d error reading burst time for PCB %u\n", __FILE__, __LINE__, i);
            free(pc);
            fclose(ptr);
            return NULL;
        }

        // Read the priority
        if (fread(&pc[i].priority, sizeof(uint32_t), 1, ptr) != 1) {
            fprintf(stderr, "%s:%d error reading priority for PCB %u\n", __FILE__, __LINE__, i);
            free(pc);
            fclose(ptr);
            return NULL;
        }

        // Read the arrival time
        if (fread(&pc[i].arrival, sizeof(uint32_t), 1, ptr) != 1) {
            fprintf(stderr, "%s:%d error reading arrival time for PCB %u\n", __FILE__, __LINE__, i);
            free(pc);
            fclose(ptr);
            return NULL;
        }

        // Initialize other fields with default values
        pc[i].started = false;  // Default value
    }

   


bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}
