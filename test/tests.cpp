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
#define QUANTUM 5 // Used for Robin Round for process as the run time limit

unsigned int score;
unsigned int total;

// check if load process doesn't have existing file
TEST(LoadProcessControlBlocks, FileDoesNotExist) {
    const char *filename = "nonexistent_file.bin";
    dyn_array_t *result = load_process_control_blocks(filename);
    EXPECT_EQ(result, nullptr);
    if (result == nullptr) 
    {
        score += 5;
    } // Award 5 points for this test case
}

// if file is incomplete
TEST(LoadProcessControlBlocks, CorruptedOrIncompleteFile) {
    const char *filename = "incomplete_file.bin";
    FILE *file = fopen(filename, "wb");
    ASSERT_NE(file, nullptr);

    uint32_t pcb_count = 2; // Pretend the file will have 2 PCBs
    fwrite(&pcb_count, sizeof(uint32_t), 1, file);
    uint32_t burst_time = 5; // Write only one field (incomplete data)
    fwrite(&burst_time, sizeof(uint32_t), 1, file);

    fclose(file);

    dyn_array_t *result = load_process_control_blocks(filename);
    EXPECT_EQ(result, nullptr);
    if (result == nullptr) 
    {
        score += 10;
    }// Award 10 points for this test case

    remove(filename);
}

// if file is correct
TEST(LoadProcessControlBlocks, ValidFile_imitate_binary) {
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
    ASSERT_NE(result, nullptr); // Ensure the function returns a valid dyn_array_t
    ASSERT_EQ(dyn_array_size(result), pcb_count); // Ensure the array size matches the PCB count

    // Validate the first PCB
    ProcessControlBlock_t *pc = (ProcessControlBlock_t *)dyn_array_at(result, 0);
    ASSERT_NE(pc, nullptr); // Ensure the pointer is valid
    EXPECT_EQ((int)pc->remaining_burst_time, (int)burst_time_1); // Validate burst time
    EXPECT_EQ((int)pc->priority, (int)priority_1);               // Validate priority
    EXPECT_EQ((int)pc->arrival, (int)arrival_1);                 // Validate arrival time
    EXPECT_EQ(pc->started, false);                              // Validate default initialization

    // Validate the second PCB
    pc = (ProcessControlBlock_t *)dyn_array_at(result, 1);
    ASSERT_NE(pc, nullptr);
    EXPECT_EQ((int)pc->remaining_burst_time, (int)burst_time_2);
    EXPECT_EQ((int)pc->priority, (int)priority_2);
    EXPECT_EQ((int)pc->arrival, (int)arrival_2);
    EXPECT_EQ(pc->started, false);

    // Assign score for passing this test case
    score += 20; // Assuming 20 points are awarded for passing this test

    // Clean up
    dyn_array_destroy(result);
    remove(filename);
}

TEST(LoadProcessControlBlocksTest, ValidFile_ReadActual)
{
    const char *test_file = "../pcb.bin"; // Use the existing pcb.bin file

    // Load the process control blocks
    dyn_array_t *pcb_array = load_process_control_blocks(test_file);
    ASSERT_NE(pcb_array, nullptr);

    // Check the size of the loaded array (Assuming there are 3 entries in the file)
    EXPECT_EQ((int)dyn_array_size(pcb_array), 3);

    // Check the values of the loaded ProcessControlBlock_t structures
    ProcessControlBlock_t *loaded_pcb = (ProcessControlBlock_t *)dyn_array_at(pcb_array, 0);
    EXPECT_EQ((int)loaded_pcb->remaining_burst_time, 4);
    EXPECT_EQ((int)loaded_pcb->priority, 15);

    loaded_pcb = (ProcessControlBlock_t *)dyn_array_at(pcb_array, 1);
    EXPECT_EQ((int)loaded_pcb->remaining_burst_time, 10);
    EXPECT_EQ((int)loaded_pcb->priority, 0);

    loaded_pcb = (ProcessControlBlock_t *)dyn_array_at(pcb_array, 2);
    EXPECT_EQ((int)loaded_pcb->remaining_burst_time, 0);
    EXPECT_EQ((int)loaded_pcb->priority, 2);
    // Clean up
    dyn_array_destroy(pcb_array);
}


// if there are no processes in the file
TEST(LoadProcessControlBlocks, ZeroProcesses) {
    const char *filename = "zero_processes_file.bin";
    FILE *file = fopen(filename, "wb");
    ASSERT_NE(file, nullptr);

    uint32_t pcb_count = 0;
    fwrite(&pcb_count, sizeof(uint32_t), 1, file);
    fclose(file);

    // Expect the function to return NULL for zero processes
    dyn_array_t *result = load_process_control_blocks(filename);
    EXPECT_EQ(result, nullptr); // Check if result is NULL

    score += 5; // Award 5 points for this test case

    remove(filename); // Clean up the file
}

// Test cases for first_come_first_serve
// check null queue
TEST(FirstComeFirstServe, NullReadyQueue) {
    dyn_array_t *ready_queue = nullptr;
    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = first_come_first_serve(ready_queue, &result);
    EXPECT_EQ(success, false);
    score += 5; // Award 5 points for this test case
}

// check null result
TEST(FirstComeFirstServe, NullResult) {
    dyn_array_t *ready_queue = dyn_array_create(0, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);

    bool success = first_come_first_serve(ready_queue, nullptr);
    EXPECT_EQ(success, false);
    score += 5; // Award 5 points for this test case

    dyn_array_destroy(ready_queue);
}

// check empty queue
TEST(FirstComeFirstServe, EmptyReadyQueue) {
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
TEST(FirstComeFirstServe, SingleProcess) {
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
TEST(FirstComeFirstServe, MultipleProcesses) {
    ProcessControlBlock_t pcb1 = {10, 1, 0, false}; // Remaining burst time: 10, priority: 1, arrival: 0
    ProcessControlBlock_t pcb2 = {5, 2, 1, false};  // Remaining burst time: 5, priority: 2, arrival: 1
    ProcessControlBlock_t pcb3 = {8, 3, 2, false};  // Remaining burst time: 8, priority: 3, arrival: 2

    dyn_array_t *ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    dyn_array_push_back(ready_queue, &pcb1);
    dyn_array_push_back(ready_queue, &pcb2);
    dyn_array_push_back(ready_queue, &pcb3);

    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = first_come_first_serve(ready_queue, &result);
    EXPECT_EQ(success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 15); //(10 + 14 + 21) / 3 = 15
    EXPECT_EQ((int)result.average_waiting_time, 7);    // (0 + 9 + 13) / 3 = 7.33
    EXPECT_EQ((int)result.total_run_time, 23);         // Last process finishes at time 23
    score += 20; // Award 20 points for this test case

    dyn_array_destroy(ready_queue);
}

// check out of order arrival
TEST(FirstComeFirstServe, ProcessesOutOfOrderArrival) {
    ProcessControlBlock_t pcb1 = {8, 1, 2, false};  // Arrival: 2
    ProcessControlBlock_t pcb2 = {5, 2, 1, false};  // Arrival: 1
    ProcessControlBlock_t pcb3 = {10, 3, 0, false}; // Arrival: 0

    dyn_array_t *ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), nullptr);
    ASSERT_NE(ready_queue, nullptr);
    dyn_array_push_back(ready_queue, &pcb1);
    dyn_array_push_back(ready_queue, &pcb2);
    dyn_array_push_back(ready_queue, &pcb3);

    ScheduleResult_t result = {0.0f, 0.0f, 0UL};
    bool success = first_come_first_serve(ready_queue, &result);
    EXPECT_EQ((int)success, true);
    EXPECT_EQ((int)result.average_turnaround_time, 19); // (10 + 14 + 23) / 3 = 19
    EXPECT_EQ((int)result.average_waiting_time, 11);    //(0 + 10 + 15) / 3 = 11
    EXPECT_EQ((int)result.total_run_time, 23);         // Last process finishes at time 23
    score += 25; // Award 25 points for this test case

    dyn_array_destroy(ready_queue);
}

class GradeEnvironment : public testing::Environment
{
    public:
        virtual void SetUp()
        {
            score = 0;
            total = 210;
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
