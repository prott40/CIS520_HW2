#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dyn_array.h"
#include "processing_scheduling.h"


// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

int arrival_time_compare(const void *a, const void *b) {
    const ProcessControlBlock_t *pcb1 = (const ProcessControlBlock_t *)a;
    const ProcessControlBlock_t *pcb2 = (const ProcessControlBlock_t *)b;
    return pcb1->arrival - pcb2->arrival; // Sort in ascending order of arrival
}

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
/*
bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    // Validate inputs
    if (!ready_queue || !result) {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return false;  // Return false for null input
    }

    // Handle the empty queue case
    if (dyn_array_size(ready_queue) == 0) {
        result->average_waiting_time = 0.0f;
        result->average_turnaround_time = 0.0f;
        result->total_run_time = 0UL;
        return true;  // Success with zeroed results
    }
    // sort the array by arival time
    if (!dyn_array_sort(ready_queue, arrival_time_compare)) {
        fprintf(stderr, "Failed to sort ready queue by arrival time\n");
        return false;
    }

    // Initialize statistics
    unsigned long total_run_time = 0;
    float total_wait_time = 0;
    float total_turnaround_time = 0;

    // Iterate over all processes in the ready_queue
    uint32_t current_time = 0;  // Keeps track of when the CPU is available for the next process

    // Assuming each PCB is of type ProcessControlBlock_t
    for (size_t i = 0; i < dyn_array_size(ready_queue); ++i) {
        ProcessControlBlock_t *pc = dyn_array_at(ready_queue, i);

        if (!pc) {
            fprintf(stderr, "%s:%d process control block is null\n", __FILE__, __LINE__);
            return false;  // If PCB is NULL, return false
        }

        // Calculate wait time: it's the difference between the current time and the arrival time
        uint32_t wait_time = (current_time >= pc->arrival) ? (current_time - pc->arrival) : 0;

        // Calculate turnaround time: it's the wait time + the burst time of the process
        uint32_t turnaround_time = wait_time + pc->remaining_burst_time;

        // Update the statistics
        total_wait_time += wait_time;
        total_turnaround_time += turnaround_time;
        total_run_time += pc->remaining_burst_time;

        // Update the current time: when this process finishes
        current_time += pc->remaining_burst_time;
    }

    // Calculate averages
    result->average_waiting_time = total_wait_time / dyn_array_size(ready_queue);
    result->average_turnaround_time = total_turnaround_time / dyn_array_size(ready_queue);
    result->total_run_time = total_run_time;

    return true;  // Success
}
*/
bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    // Validate inputs
    if (!ready_queue || !result) {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return false;  // Return false for null input
    }

    // Handle the empty queue case
    if (dyn_array_size(ready_queue) == 0) {
        result->average_waiting_time = 0.0f;
        result->average_turnaround_time = 0.0f;
        result->total_run_time = 0UL;
        return true;  // Success with zeroed results
    }

    // Sort the array by arrival time
    if (!dyn_array_sort(ready_queue, arrival_time_compare)) {
        fprintf(stderr, "Failed to sort ready queue by arrival time\n");
        return false;
    }

    // Debugging: Print the sorted processes to check if the sorting works
    printf("Sorted Processes (Arrival Time, Burst Time):\n");
    for (size_t i = 0; i < dyn_array_size(ready_queue); ++i) {
        ProcessControlBlock_t *pc = dyn_array_at(ready_queue, i);
        if (pc) {
            printf("Process %zu: Arrival = %u, Burst = %u\n", i, pc->arrival, pc->remaining_burst_time);
        }
    }

    // Initialize statistics
    unsigned long total_run_time = 0;
    float total_wait_time = 0;
    float total_turnaround_time = 0;

    // Iterate over all processes in the ready_queue
    uint32_t current_time = 0;  // Keeps track of when the CPU is available for the next process

    // Assuming each PCB is of type ProcessControlBlock_t
    for (size_t i = 0; i < dyn_array_size(ready_queue); ++i) {
        ProcessControlBlock_t *pc = dyn_array_at(ready_queue, i);

        if (!pc) {
            fprintf(stderr, "%s:%d process control block is null\n", __FILE__, __LINE__);
            return false;  // If PCB is NULL, return false
        }

        // Calculate wait time: it's the difference between the current time and the arrival time
        uint32_t wait_time = (current_time >= pc->arrival) ? (current_time - pc->arrival) : 0;

        // Calculate turnaround time: it's the wait time + the burst time of the process
        uint32_t turnaround_time = wait_time + pc->remaining_burst_time;

        // Debugging: Print wait time and turnaround time for each process
        printf("Process %zu: Wait Time = %u, Turnaround Time = %u\n", i, wait_time, turnaround_time);

        // Update the statistics
        total_wait_time += wait_time;
        total_turnaround_time += turnaround_time;
        total_run_time += pc->remaining_burst_time;

        // Update the current time: when this process finishes
        current_time += pc->remaining_burst_time;
    }

    // Calculate averages
    result->average_waiting_time = total_wait_time / dyn_array_size(ready_queue);
    result->average_turnaround_time = total_turnaround_time / dyn_array_size(ready_queue);
    result->total_run_time = total_run_time;

    // Debugging: Print the final results
    printf("Total Waiting Time = %.2f, Total Turnaround Time = %.2f, Total Run Time = %lu\n", 
           total_wait_time, total_turnaround_time, total_run_time);
    printf("Average Waiting Time = %.2f, Average Turnaround Time = %.2f\n", 
           result->average_waiting_time, result->average_turnaround_time);

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

/*
dyn_array_t *load_process_control_blocks(const char *input_file) 
{
    // check invaild parametes
    if (input_file == NULL) {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return NULL;
    }

    FILE *ptr = fopen(input_file, "rb");
    if (ptr == NULL) {
        fprintf(stderr, "%s:%d error opening file\n", __FILE__, __LINE__);
        return NULL;
    }

    // Validate file size before reading pcb_count
    fseek(ptr, 0, SEEK_END);
    long file_size = ftell(ptr);
    rewind(ptr);

    if ((unsigned int)file_size < sizeof(uint32_t)) {
        fprintf(stderr, "%s:%d file too small to contain PCB count\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }

    uint32_t pcb_count;
    if (fread(&pcb_count, sizeof(uint32_t), 1, ptr) != 1) {
        fprintf(stderr, "%s:%d error reading PCB count\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }

    // Check for overflow risk and invalid values
    const uint32_t MAX_PCB_COUNT = 100000; // Set a reasonable upper limit
    if (pcb_count == 0 || pcb_count > MAX_PCB_COUNT) {
        fprintf(stderr, "%s:%d invalid PCB count: %u\n", __FILE__, __LINE__, pcb_count);
        fclose(ptr);
        return NULL;
    }

    long expected_size = sizeof(uint32_t) + pcb_count * (3 * sizeof(uint32_t));
    if (file_size < expected_size) {
        fprintf(stderr, "%s:%d file too small (size=%ld, expected=%ld)\n", __FILE__, __LINE__, file_size, expected_size);
        fclose(ptr);
        return NULL;
    }
	
	//dyn_array_t *dyn_array = dyn_array_create(0, sizeof(ProcessControlBlock_t), free);
	
    // load process control block
    ProcessControlBlock_t *pc = (ProcessControlBlock_t*)malloc(sizeof(ProcessControlBlock_t) * pcb_count);
    if (!pc) {
        fprintf(stderr, "%s:%d error allocating memory\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }

    for (uint32_t i = 0; i < pcb_count; ++i) {
        if (fread(&pc[i].remaining_burst_time, sizeof(uint32_t), 1, ptr) != 1 ||
            fread(&pc[i].priority, sizeof(uint32_t), 1, ptr) != 1 ||
            fread(&pc[i].arrival, sizeof(uint32_t), 1, ptr) != 1) {
            fprintf(stderr, "%s:%d error reading PCB %u\n", __FILE__, __LINE__, i);
            free(pc);
            fclose(ptr);
            return NULL;
        }

        pc[i].started = false;
    }

    fclose(ptr);

    dyn_array_t *dyn_array = dyn_array_import(pc, pcb_count, sizeof(ProcessControlBlock_t), NULL);
    if (!dyn_array) {
        fprintf(stderr, "%s:%d error creating dynamic array\n", __FILE__, __LINE__);
        free(pc);
        return NULL;
    }
	free(pc);
    return dyn_array;
}
*/
dyn_array_t *load_process_control_blocks(const char *input_file)
{
    // check invaild parametes
    if (input_file == NULL) {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return NULL;
    }

    

    FILE *file = fopen(input_file, "rb");
    if (file == NULL)
    {
        return NULL; // Error opening file
    }
    // Validate file size before reading pcb_count
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    if ((unsigned int)file_size < sizeof(uint32_t)) {
        fprintf(stderr, "%s:%d file too small to contain PCB count\n", __FILE__, __LINE__);
        fclose(file);
        return NULL;
    }

    // Create the dynamic array for ProcessControlBlock_t right now I dont care about intial capacity
    dyn_array_t *dyn_array = dyn_array_create(0, sizeof(ProcessControlBlock_t), NULL);
    if (dyn_array == NULL)
    {
        fclose(file);
        return NULL; // Error creating dynamic array
    }

    ProcessControlBlock_t pcb;
   
    while (fread(&pcb, sizeof(ProcessControlBlock_t), 1, file) == 1) // read in 1 pcb at a time
    {
        // Add the loaded ProcessControlBlock_t to the dynamic array
        if (!dyn_array_push_back(dyn_array, &pcb))
        {
            fclose(file);
            // Error adding PCB to dynamic array
            dyn_array_destroy(dyn_array); // Clean up the dynamic array
            return NULL;
        }
        printf(" Burst = %u priority %u: Arrival = %u,\n",  pcb.remaining_burst_time,pcb.priority, pcb.arrival);
    }

    fclose(file);
    return dyn_array;
}



bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}
