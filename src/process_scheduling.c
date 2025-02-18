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
    if (!ready_queue || !result || dyn_array_size(ready_queue) == 0)
    {
        return false; // Return false for invalid input
    }

    // // Initialize statistics
    // unsigned long total_run_time = 0;
    // float total_wait_time = 0;
    // float total_turnaround_time = 0;

    // // Iterate over all processes in the ready_queue
    // uint32_t current_time = 0;  // Keeps track of when the CPU is available for the next process

    // // Assuming each PCB is of type ProcessControlBlock_t
    // for (size_t i = 0; i < dyn_array_size(ready_queue); ++i) {
    //     ProcessControlBlock_t *pcb = (ProcessControlBlock_t *)dyn_array_get(ready_queue, i);

    //     if (!pcb) {
    //         return false;  // If PCB is NULL, return false
    //     }

    //     // Calculate wait time: it's the difference between the current time and the arrival time
    //     uint32_t wait_time = (current_time >= pcb->arrival) ? (current_time - pcb->arrival) : 0;

    //     // Calculate turnaround time: it's the wait time + the burst time of the process
    //     uint32_t turnaround_time = wait_time + pcb->remaining_burst_time;

    //     // Update the statistics
    //     total_wait_time += wait_time;
    //     total_turnaround_time += turnaround_time;
    //     total_run_time += pcb->remaining_burst_time;

    //     // Update the current time: when this process finishes
    //     current_time += pcb->remaining_burst_time;
    // }

    // // Calculate averages
    // result->average_waiting_time = total_wait_time / dyn_array_size(ready_queue);
    // result->average_turnaround_time = total_turnaround_time / dyn_array_size(ready_queue);
    // result->total_run_time = total_run_time;

    return true; // Success
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
    FILE *file = fopen(input_file, "rb");
    if (file == NULL)
    {
        return NULL; // Error opening file
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
