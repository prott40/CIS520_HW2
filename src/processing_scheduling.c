#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dyn_array.h"
#include "processing_scheduling.h"



#define UNUSED(x) (void)(x)
// comaprison funcion to compare burst times times using the dynamic array sort funtion
// \param a is the first value
// \param b is the second value
// outline copied from https://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html
int sjf_compare(const void *a, const void *b) {
    const ProcessControlBlock_t *pcb1 = (const ProcessControlBlock_t *)a;
    const ProcessControlBlock_t *pcb2 = (const ProcessControlBlock_t *)b;

    return (pcb1->remaining_burst_time != pcb2->remaining_burst_time) ? (pcb1->remaining_burst_time - pcb2->remaining_burst_time) : (pcb1->arrival - pcb2->arrival);
}
// comaprison funcion to compare arrival times using the dynamic array sort funtion
// \param a is the first value
// \param b is the second value
// outline copied from https://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html
int arrival_time_compare(const void *a, const void *b) {
    const ProcessControlBlock_t *pcb1 = (const ProcessControlBlock_t *)a;
    const ProcessControlBlock_t *pcb2 = (const ProcessControlBlock_t *)b;
    return (pcb1->arrival < pcb2->arrival) ? -1 : (pcb1->arrival > pcb2->arrival);
}
// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
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
bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    // Validate inputs
    if (!ready_queue || !result) {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return false;  // Return false for null input
    }

    // check if the queue is empty and handle case
    if (dyn_array_size(ready_queue) == 0) {
        result->average_waiting_time = 0.0f;
        result->average_turnaround_time = 0.0f;
        result->total_run_time = 0UL;
        return true;  // Success with zeroed results
    }

    // Sort the array by arrival time and check if it failed
    if (!dyn_array_sort(ready_queue, arrival_time_compare)) {
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
    for (size_t i = 0; i < dyn_array_size(ready_queue); ++i) {
        // convert the item in array to control block
        ProcessControlBlock_t *pc = dyn_array_at(ready_queue, i);
        // check if conversion worked
        if (!pc) {
            fprintf(stderr, "%s:%d process control block is null\n", __FILE__, __LINE__);
            return false;  // If PCB is NULL, return false
        }

        // Calculate wait time
        uint32_t wait_time;
        // when the current time reached the arrival begin counting
        if (current_time >= pc->arrival) {
            wait_time = current_time - pc->arrival;
        } 
        // befire process begins
        else {
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

    return true;  // Success
}

///*
    // Runs the Shortest Job First Scheduling algorithm over the incoming ready_queue
	// \param ready queue a dyn_array of type ProcessControlBlock_t that contain be up to N elements
	// \param result used for shortest job first stat tracking \ref ScheduleResult_t
	// \return true if function ran successful else false for an error
bool shortest_job_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	
    // Validate inputs
    if (!ready_queue || !result) {
        fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
        return false;
    }
    // get the size of the 
    size_t n = dyn_array_size(ready_queue);
    // check for empty array
    if (n == 0) {
        // If the queue is empty, zero out the results
        result->average_waiting_time = 0.0f;
        result->average_turnaround_time = 0.0f;
        result->total_run_time = 0UL;
        return true;
    }
    
    // Sort the ready queue by burst time and then by arrival time
    if (!dyn_array_sort(ready_queue, sjf_compare)) {
        fprintf(stderr, "Failed to sort ready queue\n");
        return false;
    }
    /*
    // print sorted process
    printf("Sorted Processes (Burst Time, Arrival Time):\n");
    for (size_t i = 0; i < n; ++i) {
        ProcessControlBlock_t *pcb = (ProcessControlBlock_t *)dyn_array_at(ready_queue, i);
        if (pcb) {
            printf("Process %zu: Burst = %u, Arrival = %u\n", i, pcb->remaining_burst_time, pcb->arrival);
        }
    }
    //*/
    // Initialize statistics
    unsigned long total_wait_time = 0;
    unsigned long total_turnaround_time = 0;
    unsigned long total_run_time = 0;
    uint32_t current_time = 0;
    
        // Iterate over the sorted processes
    for (size_t i = 0; i < n; ++i) {
        // convert and check for failed conversion
        ProcessControlBlock_t *pcb = (ProcessControlBlock_t *)dyn_array_at(ready_queue, i);
        if (!pcb) {
            fprintf(stderr, "Process Control Block is NULL\n");
            return false;
        }
    
        // If the current time is less than the arrival time, CPU is idle until process arrives
        if (current_time < pcb->arrival) {
            current_time = pcb->arrival;
        }
    
        // Calculate wait time and turnaround time
        uint32_t wait_time = current_time - pcb->arrival;
        uint32_t turnaround_time = wait_time + pcb->remaining_burst_time;
    
        // Update statistics
        total_wait_time += wait_time;
        total_turnaround_time += turnaround_time;
        total_run_time = current_time + pcb->remaining_burst_time;
    
        // Print wait time and turnaround time for each process
        //printf("Process %zu: Wait Time = %u, Turnaround Time = %u\n", i, wait_time, turnaround_time);
    
        // Move current time forward
        current_time += pcb->remaining_burst_time;
    }
    
    // Calculate averages
    result->average_waiting_time = (float)total_wait_time / n;
    result->average_turnaround_time = (float)total_turnaround_time / n;
    result->total_run_time = total_run_time;
    
    // Print final results
        /*
    printf("Total Waiting Time = %lu, Total Turnaround Time = %lu, Total Run Time = %lu\n",
        total_wait_time, total_turnaround_time, total_run_time);
    printf("Average Waiting Time = %.2f, Average Turnaround Time = %.2f\n",
        result->average_waiting_time, result->average_turnaround_time);
    //*/
    return true;
     
    
}
//*/
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

///*
// preston's load_process_control_blocks
// Reads the PCB burst time values from the binary file into ProcessControlBlock_t remaining_burst_time field
// for N number of PCB burst time stored in the file.
// \param input_file the file containing the PCB burst times
// \return a populated dyn_array of ProcessControlBlocks if function ran successful else NULL for an error
dyn_array_t *load_process_control_blocks(const char *input_file) 
{
    // check invaild parametes
    if (input_file == NULL) {
        fprintf(stderr, "%s:%d invalid parameter\n", __FILE__, __LINE__);
        return NULL;
    }

    FILE *ptr = fopen(input_file, "rb");
    // make sure the file opens
    if (ptr == NULL) {
        fprintf(stderr, "%s:%d error opening file\n", __FILE__, __LINE__);
        return NULL;
    }

    // Validate file size before reading pcb_count
    fseek(ptr, 0, SEEK_END);
    long file_size = ftell(ptr);
    rewind(ptr);
    // check the size of the file
    if ((unsigned int)file_size < sizeof(uint32_t)) {
        fprintf(stderr, "%s:%d file too small to contain PCB count\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }
    // read the pcb count
    uint32_t pcb_count;
    if (fread(&pcb_count, sizeof(uint32_t), 1, ptr) != 1) {
        fprintf(stderr, "%s:%d error reading PCB count\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }

    // Check for overflow risk and invalid values 
    const uint32_t MAX_PCB_COUNT = 4294967295; // Set to largest uint32 value
    if (pcb_count == 0 || pcb_count > MAX_PCB_COUNT) {
        fprintf(stderr, "%s:%d invalid PCB count: %u\n", __FILE__, __LINE__, pcb_count);
        fclose(ptr);
        return NULL;
    }
	
    // load process control block
    ProcessControlBlock_t *pc = (ProcessControlBlock_t*)malloc(sizeof(ProcessControlBlock_t) * pcb_count);
    // check if load fails
    if (!pc) {
        fprintf(stderr, "%s:%d error allocating memory\n", __FILE__, __LINE__);
        fclose(ptr);
        return NULL;
    }
    // iterate over the total blocks
    for (uint32_t i = 0; i < pcb_count; ++i) {
        // check that each vlaue can be for tht block
        if (fread(&pc[i].remaining_burst_time, sizeof(uint32_t), 1, ptr) != 1 || fread(&pc[i].priority, sizeof(uint32_t), 1, ptr) != 1 || fread(&pc[i].arrival, sizeof(uint32_t), 1, ptr) != 1) {
            fprintf(stderr, "%s:%d error reading PCB %u\n", __FILE__, __LINE__, i);
            free(pc);
            fclose(ptr);
            return NULL;
        }
        // output the read values
        //printf("burst = %u priority = %u arrival = %u\n", pc[i].remaining_burst_time, pc[i].priority, pc[i].arrival);
        // initialize start
        pc[i].started = false;
    }
    // handle the file
    fclose(ptr);
    // import the read pcb's into a new dynamic array
    dyn_array_t *dyn_array = dyn_array_import(pc, pcb_count, sizeof(ProcessControlBlock_t), NULL);
    // check for failed dynamic array
    if (!dyn_array) {
        fprintf(stderr, "%s:%d error creating dynamic array\n", __FILE__, __LINE__);
        free(pc);
        return NULL;
    }
    // handle the allocated memory 
	free(pc);
    // return value
    return dyn_array;
}
//*/
/*
// chris load_process_control_blocks/
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
        //printf(" Burst = %u priority %u: Arrival = %u,\n",  pcb.remaining_burst_time,pcb.priority, pcb.arrival);
    }

    fclose(file);
    return dyn_array;
}

//*/

bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}
