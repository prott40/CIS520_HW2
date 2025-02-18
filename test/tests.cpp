#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include "gtest/gtest.h"


// Using a C library requires extern "C" to prevent function mangling
extern "C"
{
#include <dyn_array.h>
#include <processing_scheduling.h>
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
    if (result == nullptr) score += 5; // Award 5 points for this test case
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
    if (result == nullptr) score += 10; // Award 10 points for this test case

    remove(filename);
}

// if file is correct
TEST(LoadProcessControlBlocks, ValidFile) {
    const char *filename = "valid_file.bin";
    FILE *file = fopen(filename, "wb");
    ASSERT_NE(file, nullptr);

    uint32_t pcb_count = 2;
    fwrite(&pcb_count, sizeof(uint32_t), 1, file);

    // Write two PCBs
    uint32_t burst_time_1 = 5, priority_1 = 1, arrival_1 = 0;
    uint32_t burst_time_2 = 8, priority_2 = 2, arrival_2 = 2;
    fwrite(&burst_time_1, sizeof(uint32_t), 1, file);
    fwrite(&priority_1, sizeof(uint32_t), 1, file);
    fwrite(&arrival_1, sizeof(uint32_t), 1, file);
    fwrite(&burst_time_2, sizeof(uint32_t), 1, file);
    fwrite(&priority_2, sizeof(uint32_t), 1, file);
    fwrite(&arrival_2, sizeof(uint32_t), 1, file);

    fclose(file);

    dyn_array_t *result = load_process_control_blocks(filename);
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(dyn_array_size(result), pcb_count);

    ProcessControlBlock_t pcb;
    dyn_array_at(result, 0);
    EXPECT_EQ((int)pcb.remaining_burst_time, (int)burst_time_1);
    EXPECT_EQ((int)pcb.priority, (int)priority_1);
    EXPECT_EQ((int)pcb.arrival, (int)arrival_1);
    EXPECT_EQ(pcb.started, false);

    dyn_array_at(result, 1);
    EXPECT_EQ((int)pcb.remaining_burst_time, (int)burst_time_2);
    EXPECT_EQ((int)pcb.priority, (int)priority_2);
    EXPECT_EQ((int)pcb.arrival, (int)arrival_2);
    EXPECT_EQ(pcb.started, false);

    score += 20; // Award 20 points for this test case

    dyn_array_destroy(result);
    remove(filename);
}

// if there are no processes in the file
TEST(LoadProcessControlBlocks, ZeroProcesses) {
    const char *filename = "zero_processes_file.bin";
    FILE *file = fopen(filename, "wb");
    ASSERT_NE(file, nullptr);

    uint32_t pcb_count = 0;
    fwrite(&pcb_count, sizeof(uint32_t), 1, file);
    fclose(file);

    dyn_array_t *result = load_process_control_blocks(filename);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ((int)dyn_array_size(result), 0);

    score += 5; // Award 5 points for this test case

    dyn_array_destroy(result);
    remove(filename);
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
    EXPECT_EQ((int)result.average_turnaround_time, 19); // (10 + 15 + 32) / 3
    EXPECT_EQ((int)result.average_waiting_time, 11);    // (0 + 10 + 23) / 3
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
    EXPECT_EQ((int)result.average_turnaround_time, 19); // Based on sorted order: (10 + 15 + 32) / 3
    EXPECT_EQ((int)result.average_waiting_time, 11);    // Based on sorted order: (0 + 10 + 23) / 3
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
