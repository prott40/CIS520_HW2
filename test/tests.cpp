#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include "gtest/gtest.h"
#include "processing_scheduling.h"

// Using a C library requires extern "C" to prevent function mangling
extern "C"
{
#include <dyn_array.h>
}

#define NUM_PCB 30

unsigned int score;
unsigned int total;

// check if load process doesn't have existing file
TEST(LoadProcessControlBlocks, FileDoesNotExist)
{
    const char *filename = "nonexistent_file.bin";
    dyn_array_t *result = load_process_control_blocks(filename);
    EXPECT_EQ(result, nullptr);
    if (result == nullptr)
    {
        score += 5;
    } // Award 5 points for this test case
}
// tests for preston's laod pcb
///*
// if file is incomplete
TEST(LoadProcessControlBlocks, CorruptedOrIncompleteFile)
{
    const char *filename = "incomplete_file.bin";
    FILE *file = fopen(filename, "wb");
    ASSERT_NE(file, nullptr);

    // uint32_t pcb_count = 2; // Pretend the file will have 2 PCBs
    // fwrite(&pcb_count, sizeof(uint32_t), 1, file);
    uint32_t burst_time = 5; // Write only one field (incomplete data)
    fwrite(&burst_time, sizeof(uint32_t), 1, file);

    fclose(file);

    dyn_array_t *result = load_process_control_blocks(filename);
    EXPECT_EQ(result, nullptr);
    if (result == nullptr)
    {
        score += 10;
    } // Award 10 points for this test case

    remove(filename);
}
// if file is correct
TEST(LoadProcessControlBlocks, ValidFile_imitate_binary)
{
    const char *filename = "valid_file.bin";
    FILE *file = fopen(filename, "wb");
    ASSERT_NE(file, nullptr); // Ensure the file opens correctly

    // Write the number of PCBs (N = 2)
    uint32_t pcb_count = 2;
    fwrite(&pcb_count, sizeof(uint32_t), 1, file);

    // Write the first PCB (burst time, priority, arrival time)
    uint32_t burst_time_1 = 5, priority_1 = 1, arrival_1 = 0;
    fwrite(&burst_time_1, sizeof(uint32_t), 1, file);
    fwrite(&priority_1, sizeof(uint32_t), 1, file);
    fwrite(&arrival_1, sizeof(uint32_t), 1, file);

    // Write the second PCB
    uint32_t burst_time_2 = 8, priority_2 = 2, arrival_2 = 2;
    fwrite(&burst_time_2, sizeof(uint32_t), 1, file);
    fwrite(&priority_2, sizeof(uint32_t), 1, file);
    fwrite(&arrival_2, sizeof(uint32_t), 1, file);

    fclose(file);

    // Call the function to load the PCBs
    dyn_array_t *result = load_process_control_blocks(filename);
    ASSERT_NE(result, nullptr);                   // Ensure the function returns a valid dyn_array_t
    ASSERT_EQ(dyn_array_size(result), pcb_count); // Ensure the array size matches the PCB count

    // Validate the first PCB
    ProcessControlBlock_t *pc = (ProcessControlBlock_t *)dyn_array_at(result, 0);
    ASSERT_NE(pc, nullptr);                                      // Ensure the pointer is valid
    EXPECT_EQ((int)pc->remaining_burst_time, (int)burst_time_1); // Validate burst time
    EXPECT_EQ((int)pc->priority, (int)priority_1);               // Validate priority
    EXPECT_EQ((int)pc->arrival, (int)arrival_1);                 // Validate arrival time
    EXPECT_EQ(pc->started, false);                               // Validate default initialization

    // Validate the second PCB
    pc = (ProcessControlBlock_t *)dyn_array_at(result, 1);
    ASSERT_NE(pc, nullptr);
    EXPECT_EQ((int)pc->remaining_burst_time, (int)burst_time_2);
    EXPECT_EQ((int)pc->priority, (int)priority_2);
    EXPECT_EQ((int)pc->arrival, (int)arrival_2);
    EXPECT_EQ(pc->started, false);

    // Assign score for passing this test case
    score += 20;

    // Clean up
    dyn_array_destroy(result);
    remove(filename);
}

TEST(LoadProcessControlBlocksTest, ValidFile_ReadActual)
{
    const char *test_file = "../pcb.bin"; // Path to the actual binary file

    // Load the process control blocks from the binary file
    dyn_array_t *pcb_array = load_process_control_blocks(test_file);
    ASSERT_NE(pcb_array, nullptr);

    // Get the size of the dynamic array
    size_t pcb_count = dyn_array_size(pcb_array);
    printf("Loaded %zu ProcessControlBlock_t entries from %s\n", pcb_count, test_file);

    // Verify that the size is non-zero (we expect at least one PCB)
    EXPECT_GT((int)pcb_count, 0);

    // Dynamically inspect the loaded PCBs
    for (size_t i = 0; i < pcb_count; ++i)
    {
        ProcessControlBlock_t *loaded_pcb = (ProcessControlBlock_t *)dyn_array_at(pcb_array, i);
        ASSERT_NE(loaded_pcb, nullptr);

        // Print details for debugging
        printf("PCB %zu: Remaining Burst = %u, Priority = %u, Arrival = %u\n",
               i, loaded_pcb->remaining_burst_time, loaded_pcb->priority, loaded_pcb->arrival);

        // Example assertion (update based on expected properties of the binary file)
        EXPECT_GE((int)loaded_pcb->remaining_burst_time, 0); // Burst time should be non-negative
        EXPECT_GE((int)loaded_pcb->priority, 0);             // Priority should be non-negative
    }
    score += 20;
    // Clean up
    dyn_array_destroy(pcb_array);
}

// Test cases for first_come_first_serve
// check null queue
TEST(FirstComeFirstServe, NullReadyQueue)
{
    dyn_array_t *ready_queue = nullptr;
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = first_come_first_serve(ready_queue, &result);
    EXPECT_EQ(success, false);
    score += 5; // Award 5 points for this test case
}

// check null result
TEST(FirstComeFirstServe, NullResult)
{
    dyn_array_t *ready_queue = dyn_array_create(0, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);

    bool success = first_come_first_serve(ready_queue, nullptr);
    EXPECT_EQ(success, false);
    score += 5; // Award 5 points for this test case

    dyn_array_destroy(ready_queue);
}

// check empty queue
TEST(FirstComeFirstServe, EmptyReadyQueue)
{
    dyn_array_t *ready_queue = dyn_array_create(0, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);

    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = first_come_first_serve(ready_queue, &result);
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 0);
    EXPECT_EQ((int)result.average_waiting_time, 0);
    EXPECT_EQ((int)result.total_run_time, 0);
    score += 10; // Award 10 points for this test case

    dyn_array_destroy(ready_queue);
}

// check single process
TEST(FirstComeFirstServe, SingleProcess)
{
    ProcessControlBlock_t pcb = {10, 1, 0, false}; // Remaining burst time: 10, priority: 1, arrival: 0
    dyn_array_t *ready_queue = dyn_array_create(1, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    dyn_array_push_back(ready_queue, &pcb);

    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = first_come_first_serve(ready_queue, &result);
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 10);
    EXPECT_EQ((int)result.average_waiting_time, 0);
    EXPECT_EQ((int)result.total_run_time, 10);
    score += 15; // Award 15 points for this test case

    dyn_array_destroy(ready_queue);
}

// check multiple processes
// check multiple processes (using three processes as expected)
TEST(FirstComeFirstServe, MultipleProcesses)
{
    // Use three processes
    ProcessControlBlock_t pcb1 = {10, 0, 0, false}; // Arrival: 0, Burst: 10
    ProcessControlBlock_t pcb2 = {5, 0, 1, false};  // Arrival: 1, Burst: 5
    ProcessControlBlock_t pcb3 = {8, 0, 2, false};  // Arrival: 2, Burst: 8

    dyn_array_t *ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    dyn_array_push_back(ready_queue, &pcb1);
    dyn_array_push_back(ready_queue, &pcb2);
    dyn_array_push_back(ready_queue, &pcb3);

    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = first_come_first_serve(ready_queue, &result);
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 15); // (10 + 14 + 21) / 3 = 15
    EXPECT_EQ((int)result.average_waiting_time, 7);     // (0 + 9 + 13) / 3 ≈ 7 (if truncated)
    EXPECT_EQ((int)result.total_run_time, 23);          // Last process finishes at time 23
    score += 20;                                        // Award 20 points for this test case

    dyn_array_destroy(ready_queue);
}

TEST(FirstComeFirstServe, ProcessesOutOfOrderArrival)
{
    // Define processes out of order
    ProcessControlBlock_t pcb1 = {15, 0, 3, false}; // Arrival: 3, Burst: 15
    ProcessControlBlock_t pcb2 = {10, 0, 2, false}; // Arrival: 2, Burst: 10
    ProcessControlBlock_t pcb3 = {5, 0, 1, false};  // Arrival: 1, Burst: 5
    ProcessControlBlock_t pcb4 = {15, 0, 0, false}; // Arrival: 0, Burst: 15

    // Create the dynamic array
    dyn_array_t *ready_queue = dyn_array_create(4, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);

    // Add processes to the ready queue (out of order)
    dyn_array_push_back(ready_queue, &pcb4); // Arrival: 0
    dyn_array_push_back(ready_queue, &pcb2); // Arrival: 2
    dyn_array_push_back(ready_queue, &pcb3); // Arrival: 1
    dyn_array_push_back(ready_queue, &pcb1); // Arrival: 3

    // Initialize result
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};

    // Execute the scheduling function
    bool success = first_come_first_serve(ready_queue, &result);

    // Validate results
    EXPECT_EQ((int)success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 26); // (15 + 19 + 28 + 42) / 4 = 26
    EXPECT_EQ((int)result.average_waiting_time, 14);    // (0 + 14 + 18 + 27) / 4 ≈ 14.75 (truncated)
    EXPECT_EQ((int)result.total_run_time, 45);          // Last process finishes at time 45

    // Award points
    score += 25; // Award 25 points for this test case

    // Clean up
    dyn_array_destroy(ready_queue);
}

TEST(FirstComeFirstServer, WithGivenPCBFile)
{
    dyn_array_t *queue = load_process_control_blocks("../pcb.bin");
    ScheduleResult_t result;

    ASSERT_TRUE(first_come_first_serve(queue, &result)); // Quantum = 2

    dyn_array_destroy(queue);
}

TEST(ShortestJobFirst, NullReadyQueue)
{
    // create array and result empty
    dyn_array_t *ready_queue = nullptr;
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    // run to see if catch faiure
    bool success = shortest_job_first(ready_queue, &result);
    EXPECT_EQ(success, false);
    // increase score
    score += 5;
}

TEST(ShortestJobFirst, NullResult)
{
    // creates array and make sure it empty
    dyn_array_t *ready_queue = dyn_array_create(0, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    // gice result param a null value to check if it fails
    bool success = shortest_job_first(ready_queue, nullptr);
    EXPECT_EQ(success, false);
    // increase score
    score += 5;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(ShortestJobFirst, EmptyReadyQueue)
{
    // create empty queue
    dyn_array_t *ready_queue = dyn_array_create(0, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    // set reslt
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    // run shortest on empty
    bool success = shortest_job_first(ready_queue, &result);
    // check expected values
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 0);
    EXPECT_EQ((int)result.average_waiting_time, 0);
    EXPECT_EQ((int)result.total_run_time, 0);
    // increase score
    score += 10;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(ShortestJobFirst, SingleProcess)
{
    // give a single process
    ProcessControlBlock_t pcb = {5, 1, 0, false};
    dyn_array_t *ready_queue = dyn_array_create(1, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    // add to back of array
    dyn_array_push_back(ready_queue, &pcb);
    // create result
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    // run single process
    bool success = shortest_job_first(ready_queue, &result);
    // check results
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 5);
    EXPECT_EQ((int)result.average_waiting_time, 0);
    EXPECT_EQ((int)result.total_run_time, 5);
    // increase score
    score += 15;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(ShortestJobFirst, MultipleProcesses)
{
    // create muliple processs
    ProcessControlBlock_t pcb1 = {8, 0, 0, false};
    ProcessControlBlock_t pcb2 = {4, 0, 1, false};
    ProcessControlBlock_t pcb3 = {2, 0, 2, false};
    // allocate array
    dyn_array_t *ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    // add proccesses to array
    dyn_array_push_back(ready_queue, &pcb1);
    dyn_array_push_back(ready_queue, &pcb2);
    dyn_array_push_back(ready_queue, &pcb3);
    // create result
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    // run alogorith
    bool success = shortest_job_first(ready_queue, &result);
    // make sure is true and return proper runtime
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.total_run_time, 14);
    // increase score
    score += 20;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(ShortestJobFirst, DifferentArrivalTimes)
{
    // create pcb blocks
    ProcessControlBlock_t pcb1 = {5, 0, 0, false};
    ProcessControlBlock_t pcb2 = {2, 0, 3, false};
    ProcessControlBlock_t pcb3 = {4, 0, 6, false};
    // create array  and load the blocks to it
    dyn_array_t *ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    dyn_array_push_back(ready_queue, &pcb1);
    dyn_array_push_back(ready_queue, &pcb2);
    dyn_array_push_back(ready_queue, &pcb3);
    // create result
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = shortest_job_first(ready_queue, &result);
    // make sre rus corrreect with correct total run time
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.total_run_time, 11);
    // increase score
    score += 25;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(ShortestJobFirst, WithGivenPCBFile)
{
    dyn_array_t *queue = load_process_control_blocks("../pcb.bin");
    ScheduleResult_t result;

    ASSERT_TRUE(shortest_job_first(queue, &result));

    dyn_array_destroy(queue);
}

TEST(PriorityScheduling, NullReadyQueue)
{
    // create null quue
    dyn_array_t *ready_queue = nullptr;
    // create empty result
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    // check to see if null queue fails
    bool success = priority(ready_queue, &result);
    EXPECT_EQ(success, false);
    // Award 5 points
    score += 5;
}

TEST(PriorityScheduling, NullResult)
{
    // create empty array
    dyn_array_t *ready_queue = dyn_array_create(0, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    // give a null result
    bool success = priority(ready_queue, nullptr);
    EXPECT_EQ(success, false);
    // Award 5 points
    score += 5;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(PriorityScheduling, EmptyReadyQueue)
{
    // cereate queue
    dyn_array_t *ready_queue = dyn_array_create(0, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    // create result
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    // check empty
    bool success = priority(ready_queue, &result);
    // check all vaules
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 0);
    EXPECT_EQ((int)result.average_waiting_time, 0);
    EXPECT_EQ((int)result.total_run_time, 0);
    // Award 10 points
    score += 10;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(PriorityScheduling, SingleProcess)
{
    // create single process
    ProcessControlBlock_t pcb = {10, 1, 0, false};
    // ready queue and enque single process
    dyn_array_t *ready_queue = dyn_array_create(1, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    dyn_array_push_back(ready_queue, &pcb);
    // create result and test single process
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = priority(ready_queue, &result);
    // check values
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 10);
    EXPECT_EQ((int)result.average_waiting_time, 0);
    EXPECT_EQ((int)result.total_run_time, 10);
    // Award 15 points
    score += 15;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(PriorityScheduling, MultipleProcesses)
{
    // create multipkle processes
    ProcessControlBlock_t pcb1 = {8, 3, 0, false}; // Priority 3, Arrival 0, Burst 8
    ProcessControlBlock_t pcb2 = {4, 1, 1, false}; // Priority 1, Arrival 1, Burst 4
    ProcessControlBlock_t pcb3 = {6, 2, 2, false}; // Priority 2, Arrival 2, Burst 6
    // create queu and them to queue
    dyn_array_t *ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    dyn_array_push_back(ready_queue, &pcb1);
    dyn_array_push_back(ready_queue, &pcb2);
    dyn_array_push_back(ready_queue, &pcb3);
    // create result
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    // run algorithm
    bool success = priority(ready_queue, &result);
    // check result
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 10);
    EXPECT_EQ((int)result.average_waiting_time, 4);
    EXPECT_EQ((int)result.total_run_time, 19);
    // Award 20 points
    score += 20;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(PriorityScheduling, SamePriorityDifferentArrival)
{
    // create processs
    ProcessControlBlock_t pcb1 = {5, 2, 0, false}; // Priority 2, Arrival 0, Burst 5
    ProcessControlBlock_t pcb2 = {3, 2, 1, false}; // Priority 2, Arrival 1, Burst 3
    ProcessControlBlock_t pcb3 = {6, 2, 2, false}; // Priority 2, Arrival 2, Burst 6
    // create queue and add them to queue
    dyn_array_t *ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    dyn_array_push_back(ready_queue, &pcb1);
    dyn_array_push_back(ready_queue, &pcb2);
    dyn_array_push_back(ready_queue, &pcb3);
    // create reult and run algorithm
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = priority(ready_queue, &result);
    // check return values
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 8);
    EXPECT_EQ((int)result.average_waiting_time, 3);
    EXPECT_EQ((int)result.total_run_time, 14);
    // Award 20 points
    score += 20;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(PriorityScheduling, ProcessesOutOfOrderPriority)
{
    // create process
    ProcessControlBlock_t pcb1 = {10, 4, 3, false}; // Priority 4, Arrival 3, Burst 10
    ProcessControlBlock_t pcb2 = {5, 2, 2, false};  // Priority 2, Arrival 2, Burst 5
    ProcessControlBlock_t pcb3 = {7, 1, 1, false};  // Priority 1, Arrival 1, Burst 7
    ProcessControlBlock_t pcb4 = {8, 3, 0, false};  // Priority 3, Arrival 0, Burst 8
    // create queue and load processes into queue
    dyn_array_t *ready_queue = dyn_array_create(4, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    dyn_array_push_back(ready_queue, &pcb4);
    dyn_array_push_back(ready_queue, &pcb2);
    dyn_array_push_back(ready_queue, &pcb3);
    dyn_array_push_back(ready_queue, &pcb1);
    // create result and run algorithm
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = priority(ready_queue, &result);
    // check reults
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 16);
    EXPECT_EQ((int)result.average_waiting_time, 9);
    EXPECT_EQ((int)result.total_run_time, 31);
    // Award 25 points
    score += 25;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(PriorityScheduling, WithGivenPCBFile)
{
    dyn_array_t *queue = load_process_control_blocks("../pcb.bin");
    ScheduleResult_t result;

    ASSERT_TRUE(priority(queue, &result));

    dyn_array_destroy(queue);
}

TEST(RoundRobinTest, MultipleProcessesLargeQuant)
{
    dyn_array_t *queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);

    ProcessControlBlock_t p1 = {3, 0, 0, false}; // Process 1: Burst 3
    ProcessControlBlock_t p2 = {3, 0, 0, false}; // Process 2: Burst 3
    ProcessControlBlock_t p3 = {3, 0, 0, false}; // Process 3: Burst 3

    dyn_array_push_back(queue, &p1);
    dyn_array_push_back(queue, &p2);
    dyn_array_push_back(queue, &p3);

    ScheduleResult_t result;
    ASSERT_TRUE(round_robin(queue, &result, 3)); // Quantum = 3

    // Ensure correct type casting for comparison
    ASSERT_EQ(result.average_waiting_time, (float)3.0);  // For float values
    ASSERT_EQ(result.average_turnaround_time, (float)6.0);  // For float values
    ASSERT_EQ(result.total_run_time, (float)18);  // Ensure unsigned long

    dyn_array_destroy(queue);
}

TEST(RoundRobinTest, MultipleProcessesSmallQuant)
{
    dyn_array_t *queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);

    ProcessControlBlock_t p1 = {3, 0, 0, false}; // Process 1: Burst 3
    ProcessControlBlock_t p2 = {3, 0, 0, false}; // Process 2: Burst 3
    ProcessControlBlock_t p3 = {3, 0, 0, false}; // Process 3: Burst 3

    dyn_array_push_back(queue, &p1);
    dyn_array_push_back(queue, &p2);
    dyn_array_push_back(queue, &p3);

    ScheduleResult_t result;
    ASSERT_TRUE(round_robin(queue, &result, (float)2)); // Quantum = 2
    ASSERT_EQ(result.average_waiting_time, (float)5);
    ASSERT_EQ(result.average_turnaround_time, (float)8);
    ASSERT_EQ(result.total_run_time, (float)24);

    dyn_array_destroy(queue);
}

TEST(RoundRobinTest, MultipleProcessesDiffArrivalTimes)
{
    dyn_array_t *queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);

    ProcessControlBlock_t p1 = {3, 0, 2, false}; // Process 1: Burst 3
    ProcessControlBlock_t p2 = {3, 0, 0, false}; // Process 2: Burst 3
    ProcessControlBlock_t p3 = {3, 0, 0, false}; // Process 3: Burst 3

    dyn_array_push_back(queue, &p1);
    dyn_array_push_back(queue, &p2);
    dyn_array_push_back(queue, &p3);

    ScheduleResult_t result;
    ASSERT_TRUE(round_robin(queue, &result,(float) 2)); // Quantum = 2
    EXPECT_NEAR(result.average_waiting_time, (float)4.33333, (float)1e-5);
    EXPECT_NEAR(result.average_turnaround_time,(float) 7.33333,(float) 1e-5);
    ASSERT_EQ(result.total_run_time, (float)22);

    dyn_array_destroy(queue);
}

TEST(RoundRobinTest, WithGivenPCBFile)
{
    dyn_array_t *queue = load_process_control_blocks("../pcb.bin");
    ScheduleResult_t result;

    ASSERT_TRUE(round_robin(queue, &result, (float)2)); // Quantum = 2

    dyn_array_destroy(queue);
}

TEST(RoundRobinTest, QuantIsZero)
{
    dyn_array_t *queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);

    ProcessControlBlock_t p1 = {3, 0, 0, false}; // Process 1: Burst 3
    ProcessControlBlock_t p2 = {3, 0, 0, false}; // Process 2: Burst 3
    ProcessControlBlock_t p3 = {3, 0, 0, false}; // Process 3: Burst 3

    dyn_array_push_back(queue, &p1);
    dyn_array_push_back(queue, &p2);
    dyn_array_push_back(queue, &p3);

    ScheduleResult_t result;
    ASSERT_FALSE(round_robin(queue, &result, (float)0));

    dyn_array_destroy(queue);
}

TEST(RoundRobinTest, NullReadyQueue)
{
    // create null quue
    dyn_array_t *ready_queue = nullptr;
    // create empty result
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    // check to see if null queue fails
    bool success = round_robin(ready_queue, &result, 2);
    EXPECT_EQ(success, false);
    // Award 5 points
    score += 5;
}

// TEST(RoundRobinTest, WithPCBFILE)
// {
//     dyn_array_t *queue = load_process_control_blocks("../pcb.bin");

//     ScheduleResult_t result;
//     ASSERT_TRUE(round_robin(queue, &result, 2)); // Quantum = 2
//     ASSERT_EQ(result.average_waiting_time, 5);
//     ASSERT_EQ(result.average_turnaround_time, 8);
//     ASSERT_EQ(result.total_run_time, 24);

//     dyn_array_destroy(queue);
// }

TEST(RoundRobinTest, NullResult)
{
    // create empty array
    dyn_array_t *ready_queue = dyn_array_create(0, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    // give a null result
    bool success = round_robin(ready_queue, nullptr, 1);
    EXPECT_EQ(success, false);
    // Award 5 points
    score += 5;
    // clean up
    dyn_array_destroy(ready_queue);
}

TEST(RoundRobinTest, EmptyReadyQueue)
{
    // cereate queue
    dyn_array_t *ready_queue = dyn_array_create(0, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    // create result
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    // check empty
    bool success = round_robin(ready_queue, &result, 1);
    EXPECT_FALSE(success);
    // clean up
    dyn_array_destroy(ready_queue);
}

class GradeEnvironment : public testing::Environment
{
public:
    virtual void SetUp()
    {
        score = 0;
        total = 315;
    }

    virtual void TearDown()
    {
        ::testing::Test::RecordProperty("points_given", score);
        ::testing::Test::RecordProperty("points_total", total);
        std::cout << "SCORE: " << score << '/' << total << std::endl;
    }
};

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new GradeEnvironment);

    return RUN_ALL_TESTS();
}
