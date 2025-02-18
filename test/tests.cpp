#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include "gtest/gtest.h"
#include "../include/processing_scheduling.h"

// Using a C library requires extern "C" to prevent function mangling
extern "C"
{
#include <dyn_array.h>
}

#define NUM_PCB 30
#define QUANTUM 5 // Used for Robin Round for process as the run time limit

unsigned int score;
unsigned int total;

TEST(LoadProcessControlBlocksTest, ValidFile)
{
	const char *test_file = "../pcb.bin"; // Use the existing pcb.bin file

	// Load the process control blocks
	dyn_array_t *pcb_array = load_process_control_blocks(test_file);
	ASSERT_NE(pcb_array, nullptr);

	// Check the size of the loaded array (Assuming there are 3 entries in the file)
	EXPECT_EQ(dyn_array_size(pcb_array), 3);

	// Check the values of the loaded ProcessControlBlock_t structures
	ProcessControlBlock_t *loaded_pcb = (ProcessControlBlock_t *)dyn_array_at(pcb_array, 0);
	EXPECT_EQ(loaded_pcb->remaining_burst_time, 4);

	loaded_pcb = (ProcessControlBlock_t *)dyn_array_at(pcb_array, 1);
	EXPECT_EQ(loaded_pcb->remaining_burst_time, 10);

	loaded_pcb = (ProcessControlBlock_t *)dyn_array_at(pcb_array, 2);
	EXPECT_EQ(loaded_pcb->remaining_burst_time, 0);

	// Clean up
	dyn_array_destroy(pcb_array);
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