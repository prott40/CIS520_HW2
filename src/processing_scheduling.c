#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dyn_array.h"
#include "processing_scheduling.h"

#define UNUSED(x) (void)(x)
int priority_compare(const void *a, const void *b)
{
    ProcessControlBlock_t *pcb1 = (ProcessControlBlock_t *)a;
    ProcessControlBlock_t *pcb2 = (ProcessControlBlock_t *)b;
    // if priority 1 is not equal to priority 2 then the return 1 - 2 otherwise return arrival1 - arrival2
    return (pcb1->priority != pcb2->priority) ? pcb1->priority - pcb2->priority : pcb1->arrival - pcb2->arrival; // If equal priority, earlier arrival goes first
}

// comaprison funcion to compare burst times times using the dynamic array sort funtion
// \param a is the first value
// \param b is the second value
// outline copied from https://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html
int sjf_compare(const void *a, const void *b)
{
    const ProcessControlBlock_t *pcb1 = (const ProcessControlBlock_t *)a;
    const ProcessControlBlock_t *pcb2 = (const ProcessControlBlock_t *)b;
    // if burst 1 is not equal to burst 2 return burst 1 - 2 otherwise return arrival 1 minus ariaval 2
    return (pcb1->remaining_burst_time != pcb2->remaining_burst_time) ? (pcb1->remaining_burst_time - pcb2->remaining_burst_time) : (pcb1->arrival - pcb2->arrival);
}
// comaprison funcion to compare arrival times using the dynamic array sort funtion
// \param a is the first value
// \param b is the second value
// outline copied from https://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html
int arrival_time_compare(const void *a, const void *b)
{
    const ProcessControlBlock_t *pcb1 = (const ProcessControlBlock_t *)a;
    const ProcessControlBlock_t *pcb2 = (const ProcessControlBlock_t *)b;
    // if arival 1 is less than arrival 2 return -1 otherwise rethen arrival 1 is greater tahn 2
    return (pcb1->arrival < pcb2->arrival) ? -1 : (pcb1->arrival > pcb2->arrival);
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
bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result)
{
    // Validate inputs
    if (!ready_queue || !result)
    {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return false; // Return false for null input
    }

    // check if the queue is empty and handle case
    if (dyn_array_size(ready_queue) == 0)
    {
        result->average_waiting_time = 0.0f;
        result->average_turnaround_time = 0.0f;
        result->total_run_time = 0UL;
        return true; // Success with zeroed results
    }

    // Sort the array by arrival time and check if it failed
    if (!dyn_array_sort(ready_queue, arrival_time_compare))
    {
        fprintf(stderr, "Failed to sort ready queue by arrival time\n");
        return false;
    }

    /*
    // check to make sure values are sorted
    printf("Sorted Processes (Arrival Time, Burst Time):\n");
    for (size_t i = 0; i < dyn_array_size(ready_queue); ++i) {
        ProcessControlBlock_t *pc = dyn_array_at(ready_queue, i);
        if (pc) {
            printf("Process %zu: Arrival = %u, Burst = %u\n", i, pc->arrival, pc->remaining_burst_time);
        }
    }
    //*/
    // Initialize statistics
    unsigned long total_run_time = 0;
    float total_wait_time = 0;
    float total_turnaround_time = 0;

    // Iterate over all processes in the ready_queue
    // use current_time to track when CPU is available for the next process
    uint32_t current_time = 0;

    // look at each item in the queue
    for (size_t i = 0; i < dyn_array_size(ready_queue); ++i)
    {
        // convert the item in array to control block
        ProcessControlBlock_t *pc = dyn_array_at(ready_queue, i);
        // check if conversion worked
        if (!pc)
        {
            fprintf(stderr, "%s:%d process control block is null\n", __FILE__, __LINE__);
            return false; // If PCB is NULL, return false
        }

        // Calculate wait time
        uint32_t wait_time;
        // when the current time reached the arrival begin counting
        if (current_time >= pc->arrival)
        {
            wait_time = current_time - pc->arrival;
        }
        // befire process begins
        else
        {
            wait_time = 0;
        }
        // Calculate turnaround time
        uint32_t turnaround_time = wait_time + pc->remaining_burst_time;

        // return value of process
        // printf("Process %zu: Wait Time = %u, Turnaround Time = %u\n", i, wait_time, turnaround_time);

        // Update the statistics
        total_wait_time += wait_time;
        total_turnaround_time += turnaround_time;
        total_run_time += pc->remaining_burst_time;

        // Update the current time with when this process finishes
        current_time += pc->remaining_burst_time;
    }

    // Calculate averages
    result->average_waiting_time = total_wait_time / dyn_array_size(ready_queue);
    result->average_turnaround_time = total_turnaround_time / dyn_array_size(ready_queue);
    result->total_run_time = total_run_time;

    // Print the final results
    // printf("Total Waiting Time = %.2f, Total Turnaround Time = %.2f, Total Run Time = %lu\n", total_wait_time, total_turnaround_time, total_run_time);
    // printf("Average Waiting Time = %.2f, Average Turnaround Time = %.2f\n", result->average_waiting_time, result->average_turnaround_time);

    return true; // Success
}

///*
// Runs the Shortest Job First Scheduling algorithm over the incoming ready_queue
// \param ready queue a dyn_array of type ProcessControlBlock_t that contain be up to N elements
// \param result used for shortest job first stat tracking \ref ScheduleResult_t
// \return true if function ran successful else false for an error
bool shortest_job_first(dyn_array_t *ready_queue, ScheduleResult_t *result)
{
    // Validate inputs
    if (!ready_queue || !result)
    {
        fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
        return false;
    }

    // Get the size of the ready queue
    size_t n = dyn_array_size(ready_queue);
    
    // Check for empty queue
    if (n == 0)
    {
        // If the queue is empty, zero out the results
        result->average_waiting_time = 0.0f;
        result->average_turnaround_time = 0.0f;
        result->total_run_time = 0UL;
        return true;
    }

    // Sort the ready queue by burst time and then by arrival time
    if (!dyn_array_sort(ready_queue, sjf_compare))
    {
        fprintf(stderr, "Failed to sort ready queue\n");
        return false;
    }

    // Initialize statistics
    unsigned long total_wait_time = 0;
    unsigned long total_turnaround_time = 0;
    unsigned long total_run_time = 0;
    uint32_t current_time = 0;

    // Iterate over the sorted processes
    for (size_t i = 0; i < n; ++i)
    {
        // Convert and check for failed conversion
        ProcessControlBlock_t *pcb = (ProcessControlBlock_t *)dyn_array_at(ready_queue, i);
        if (!pcb)
        {
            fprintf(stderr, "Process Control Block is NULL\n");
            return false;
        }

        // If the current time is less than the arrival time, CPU is idle until the process arrives
        if (current_time < pcb->arrival)
        {
            current_time = pcb->arrival;
        }

        // Calculate wait time and turnaround time
        uint32_t wait_time = current_time - pcb->arrival;
        uint32_t turnaround_time = wait_time + pcb->remaining_burst_time;

        // Update statistics
        total_wait_time += wait_time;
        total_turnaround_time += turnaround_time;
        total_run_time += pcb->remaining_burst_time;  // Fix: Increment total_run_time by burst time

        // Move current time forward
        current_time += pcb->remaining_burst_time;
    }

    // Calculate averages
    result->average_waiting_time = (float)total_wait_time / n;
    result->average_turnaround_time = (float)total_turnaround_time / n;
    result->total_run_time = total_run_time;

    return true;
}
//*/
///*
// Runs the Priority algorithm over the incoming ready_queue
// \param ready queue a dyn_array of type ProcessControlBlock_t that contain be up to N elements
// \param result used for shortest job first stat tracking \ref ScheduleResult_t
// \return true if function ran successful else false for an error
bool priority(dyn_array_t *ready_queue, ScheduleResult_t *result)
{
    // Validate input parameters
    if (!ready_queue || !result)
    {
        fprintf(stderr, "Error: NULL ready_queue or result\n");
        return false;
    }

    size_t n = dyn_array_size(ready_queue);
    if (n == 0)
    {
        // If no processes, set results to zero
        result->average_waiting_time = 0.0f;
        result->average_turnaround_time = 0.0f;
        result->total_run_time = 0UL;
        return true;
    }

    // Sort the ready queue by priority (ascending) and arrival time (ascending)
    if (!dyn_array_sort(ready_queue, priority_compare))
    {
        fprintf(stderr, "Error: Failed to sort ready queue\n");
        return false;
    }

    unsigned long total_wait_time = 0;
    unsigned long total_turnaround_time = 0;
    unsigned long total_run_time = 0;
    uint32_t current_time = 0;

    // Iterate through sorted queue
    for (size_t i = 0; i < n; ++i)
    {
        ProcessControlBlock_t *pcb = (ProcessControlBlock_t *)dyn_array_at(ready_queue, i);
        if (!pcb)
        {
            fprintf(stderr, "Error: NULL PCB encountered\n");
            return false;
        }

        // If CPU is idle, move to arrival time
        if (current_time < pcb->arrival)
        {
            current_time = pcb->arrival;
        }

        // Compute wait time and turnaround time
        uint32_t wait_time = current_time - pcb->arrival;
        uint32_t turnaround_time = wait_time + pcb->remaining_burst_time;

        // Update statistics
        total_wait_time += wait_time;
        total_turnaround_time += turnaround_time;
        total_run_time = current_time + pcb->remaining_burst_time;

        // Move current time forward
        current_time += pcb->remaining_burst_time;
    }

    // Compute averages
    result->average_waiting_time = (float)total_wait_time / n;
    result->average_turnaround_time = (float)total_turnaround_time / n;
    result->total_run_time = total_run_time;

    return true;
}

bool round_robin(dyn_array_t *ready_queue, ScheduleResult_t *result, size_t quantum)
{
    if (!ready_queue || !result || quantum == 0 || dyn_array_size(ready_queue) == 0)
    {
        return false;
    }

    if (!dyn_array_sort(ready_queue, arrival_time_compare))
    {
        fprintf(stderr, "Error: Failed to sort ready queue\n");
        return false;
    }

    float total_waiting_time = 0.0;
    float total_turnaround_time = 0.0;
    unsigned long total_run_time = 0;
    unsigned long current_time = 0;
    size_t num_processes = dyn_array_size(ready_queue);

    while (dyn_array_size(ready_queue) > 0)
    {
        ProcessControlBlock_t *current_process = dyn_array_at(ready_queue, 0);
        if (!current_process)
        {
            return false; // Error: process is NULL
        }

        // If CPU is idle, move to the arrival time
        if (current_time < current_process->arrival)
        {
            current_time = current_process->arrival;
        }

        // Calculate wait time: current time - arrival time
        unsigned long wait_time = current_time - current_process->arrival;

        if (current_process->remaining_burst_time <= quantum) // complete the process
        {
            while (current_process->remaining_burst_time > 0)
            {
                current_process->started = true;
                virtual_cpu(current_process); // Decrement the remaining burst time
                current_time++;               // Increment the time for each execution cycle
            }
            // After the process finishes, calculate turnaround time
            unsigned long turnaround_time = current_time - current_process->arrival;
            total_turnaround_time += turnaround_time;
            total_waiting_time += wait_time;
            total_run_time += turnaround_time; // Run time is typically the turnaround time
            dyn_array_pop_front(ready_queue);  // Remove process from ready queue
        }
        else // run the process up to the quantum
        {
            for (size_t i = 0; i < quantum; i++)
            {
                current_process->started = true;
                virtual_cpu(current_process); // Decrement the remaining burst time
                current_time++;               // Increment the time for each execution cycle
                total_waiting_time--;
            }
            // Re-insert the process into the ready queue if not finished
            dyn_array_push_back(ready_queue, current_process);
            dyn_array_pop_front(ready_queue);
        }
    }

    // Store the results
    result->average_waiting_time = total_waiting_time / num_processes;
    result->average_turnaround_time = total_turnaround_time / num_processes;
    result->total_run_time = total_run_time;

    return true;
}

///*
// preston's load_process_control_blocks
// Reads the PCB burst time values from the binary file into ProcessControlBlock_t remaining_burst_time field
// for N number of PCB burst time stored in the file.
// \param input_file the file containing the PCB burst times
// \return a populated dyn_array of ProcessControlBlocks if function ran successful else NULL for an error
dyn_array_t *load_process_control_blocks(const char *input_file)
{
    // check invaild parametes
    if (input_file == NULL)
    {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return NULL;
    }

    FILE *ptr = fopen(input_file, "rb");
    // make sure the file opens
    if (ptr == NULL)
    {
        fprintf(stderr, "%s:%d error opening file\n", __FILE__, __LINE__);
        return NULL;
    }

    // Validate file size before reading pcb_count
    fseek(ptr, 0, SEEK_END);
    long file_size = ftell(ptr);
    rewind(ptr);
    // check the size of the file
    if ((unsigned int)file_size < sizeof(uint32_t))
    {
        fprintf(stderr, "%s:%d file too small to contain PCB count\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }
    // read the pcb count
    uint32_t pcb_count;
    if (fread(&pcb_count, sizeof(uint32_t), 1, ptr) != 1)
    {
        fprintf(stderr, "%s:%d error reading PCB count\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }

    // Check for overflow risk and invalid values
    const uint32_t MAX_PCB_COUNT = 4294967295; // Set to largest uint32 value
    if (pcb_count == 0 || pcb_count > MAX_PCB_COUNT)
    {
        fprintf(stderr, "%s:%d invalid PCB count: %u\n", __FILE__, __LINE__, pcb_count);
        fclose(ptr);
        return NULL;
    }

    // load process control block
    ProcessControlBlock_t *pc = (ProcessControlBlock_t *)malloc(sizeof(ProcessControlBlock_t) * pcb_count);
    // check if load fails
    if (!pc)
    {
        fprintf(stderr, "%s:%d error allocating memory\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }
    // iterate over the total blocks
    for (uint32_t i = 0; i < pcb_count; ++i)
    {
        // check that each vlaue can be for tht block
        if (fread(&pc[i].remaining_burst_time, sizeof(uint32_t), 1, ptr) != 1 || fread(&pc[i].priority, sizeof(uint32_t), 1, ptr) != 1 || fread(&pc[i].arrival, sizeof(uint32_t), 1, ptr) != 1)
        {
            fprintf(stderr, "%s:%d error reading PCB %u\n", __FILE__, __LINE__, i);
            free(pc);
            fclose(ptr);
            return NULL;
        }
        // output the read values
        // printf("burst = %u priority = %u arrival = %u\n", pc[i].remaining_burst_time, pc[i].priority, pc[i].arrival);
        // initialize start
        pc[i].started = false;
    }
    // handle the file
    fclose(ptr);
    // import the read pcb's into a new dynamic array
    dyn_array_t *dyn_array = dyn_array_import(pc, pcb_count, sizeof(ProcessControlBlock_t), NULL);
    // check for failed dynamic array
    if (!dyn_array)
    {
        fprintf(stderr, "%s:%d error creating dynamic array\n", __FILE__, __LINE__);
        free(pc);
        return NULL;
    }
    // handle the allocated memory
    free(pc);
    // return value
    return dyn_array;
}

bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result)
{
    // Initialize statistics
    unsigned long total_wait_time = 0;
    unsigned long total_turnaround_time = 0;
    unsigned long total_run_time = 0;
    uint32_t current_time = 0;

    // Validate input parameters
    if (!ready_queue || !result) {
        fprintf(stderr, "Error: NULL ready_queue or result\n");
        return false;
    }

    // Get the size of the queue
    size_t originalArraySize = dyn_array_size(ready_queue);
    size_t n = originalArraySize;

    // Check for empty array
    if (n == 0) {
        // If the queue is empty, zero out the results
        result->average_waiting_time = 0.0f;
        result->average_turnaround_time = 0.0f;
        result->total_run_time = 0UL;
        return true;
    }

    while (n > 0) {

        // Sort the ready queue based on remaining burst time
        if (!dyn_array_sort(ready_queue, sjf_compare)) {
            fprintf(stderr, "Failed to sort ready queue\n");
            return false;
        }
        
        // Get the process with the shortest remaining burst time
        ProcessControlBlock_t *current_process = dyn_array_at(ready_queue, 0);
        if (!current_process) {
            return false; // Error: process is NULL
        }

        // If the current time is less than the process's arrival time, wait for it to arrive
        if (current_time < current_process->arrival) {
            current_time = current_process->arrival;
        }

        // Calculate wait time and turnaround time
        uint32_t wait_time = current_time - current_process->arrival;
        uint32_t turnaround_time = wait_time + current_process->remaining_burst_time;

        // Update statistics
        total_wait_time += wait_time;
        total_turnaround_time += turnaround_time;
        total_run_time += current_process->remaining_burst_time;

        // Run the process (decrease its remaining burst time by 1)
        virtual_cpu(current_process);
        current_time++;

        // If the process has finished, remove it from the ready queue
        if (current_process->remaining_burst_time == 0) {
            dyn_array_pop_front(ready_queue);
        }

        // Update the current time
        //current_time += current_process->remaining_burst_time;


        // Update the size of the ready queue
        n = dyn_array_size(ready_queue);
    }

    // Calculate averages
    result->average_waiting_time = (float)total_wait_time / originalArraySize;
    result->average_turnaround_time = (float)total_turnaround_time / originalArraySize;
    result->total_run_time = total_run_time;

    return true;
}
