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
/*
// if file is incomplete
TEST(LoadProcessControlBlocks, CorruptedOrIncompleteFile) {
    const char *filename = "incomplete_file.bin";
    FILE *file = fopen(filename, "wb");
    ASSERT_NE(file, nullptr);

    //uint32_t pcb_count = 2; // Pretend the file will have 2 PCBs
    //fwrite(&pcb_count, sizeof(uint32_t), 1, file);
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
*/
TEST(LoadProcessControlBlocks, CorruptedOrIncompleteFile) {
    const char *filename = "corrupted_file.bin";

    // Case 1: File is too small to contain even the pcb_count
    FILE *file = fopen(filename, "wb");
    ASSERT_NE(file, nullptr);
    uint8_t small_data = 0xFF; // Just 1 byte, not enough for pcb_count
    fwrite(&small_data, sizeof(uint8_t), 1, file);
    fclose(file);

    dyn_array_t *result = load_process_control_blocks(filename);
    EXPECT_EQ(result, nullptr); // Should return nullptr for invalid file
    remove(filename);

    // Case 4: File is completely empty
    file = fopen(filename, "wb");
    ASSERT_NE(file, nullptr);
    fclose(file);

    result = load_process_control_blocks(filename);
    EXPECT_EQ(result, nullptr); // Should return nullptr for empty file
    remove(filename);
}

TEST(LoadProcessControlBlocksTest, ValidFileRead)
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
    EXPECT_EQ((int)result.average_turnaround_time, 15); // (10 + 14 + 23) / 3 = 19
    EXPECT_EQ((int)result.average_waiting_time, 7);    //(0 + 10 + 15) / 3 = 11
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
