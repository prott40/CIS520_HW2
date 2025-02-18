#include <stdio.h>
#include "dyn_array.h"

// Flag values
// SHRUNK to indicate shrink_to_fit was called and size needs to be corrected
// SORTED to track if the objects have been sorted by us (sorted is set by sort and unset by insert/push)
// these are just ideas
// typedef enum {NONE = 0x00, SHRUNK = 0x01, SORTED = 0x02, ALL = 0xFF} DYN_FLAGS;

struct dyn_array 
{
	// DYN_FLAGS flags;
	size_t capacity;
	size_t size;
	const size_t data_size;
	void *array;
	void (*destructor)(void *);
};

// Supports 64bit+ size_t!
// Semi-arbitrary cap on contents. We'll run out of memory before this happens anyway.
// Allowing it to be externally set
#ifndef DYN_MAX_CAPACITY
#define DYN_MAX_CAPACITY (((size_t) 1) << ((sizeof(size_t) << 3) - 8))
#endif

// casts pointer and does arithmetic to get index of element
#define DYN_ARRAY_POSITION(dyn_array_ptr, idx) \
	(((uint8_t *) (dyn_array_ptr)->array) + ((idx) * (dyn_array_ptr)->data_size))
// Gets the size (in bytes) of n dyn_array elements
#define DYN_SIZE_N_ELEMS(dyn_array_ptr, n) ((dyn_array_ptr)->data_size * (n))



// Modes of operation for dyn_shift
typedef enum { MODE_INSERT = 0x01, MODE_EXTRACT = 0x02, MODE_ERASE = 0x06, TYPE_REMOVE = 0x02 } DYN_SHIFT_MODE;

// The core of any insert/remove operation, check the impl for details
// One inserts, one decreases. Super simple stuff.
// Only do the absolute minimum amount of checks before calling one of these, they're tough
bool dyn_shift_insert(dyn_array_t *const dyn_array, const size_t position, const size_t count,
					  const DYN_SHIFT_MODE mode, const void *const data_src);

bool dyn_shift_remove(dyn_array_t *const dyn_array, const size_t position, const size_t count,
					  const DYN_SHIFT_MODE mode, void *const data_dst);




dyn_array_t *dyn_array_create(const size_t capacity, const size_t data_type_size, void (*destruct_func)(void *)) 
{
    // Validate inputs
    if (data_type_size == 0 || capacity > DYN_MAX_CAPACITY) {
		fprintf(stderr, "%s:%d invalid input parameter\n", __FILE__, __LINE__);
        return NULL; // Invalid input
    }

    // Determine the actual capacity (rounding up to the nearest power of 2)
    size_t actual_capacity = 16;
    while (actual_capacity < capacity) {
        if (actual_capacity > SIZE_MAX / 2) { 
			fprintf(stderr, "%s:%d overflow error\n", __FILE__, __LINE__);
            return NULL;
        }
        actual_capacity <<= 1;
    }

    // Allocate memory for the dynamic array's data
    void *array = malloc(data_type_size * actual_capacity);
    if (!array) {
		fprintf(stderr, "%s:%d allocation failure\n", __FILE__, __LINE__);
        return NULL; 
    }

    // Allocate memory for the dynamic array structure
    dyn_array_t *dyn_array = malloc(sizeof(dyn_array_t));
    if (!dyn_array) {
		fprintf(stderr, "%s:%d dynamic allocaiton failurer\n", __FILE__, __LINE__);
        free(array);
        return NULL; 
    }

    // Initialize each field individually (required because of `const`)
    dyn_array->capacity = actual_capacity;
    dyn_array->size = 0;
    *(size_t *)&dyn_array->data_size = data_type_size; // Use a cast to modify the `const` field safely
    dyn_array->array = array;
    dyn_array->destructor = destruct_func;

    return dyn_array; // Successfully created
}

dyn_array_t *dyn_array_import(const void *const data, const size_t count, const size_t data_type_size, void (*destruct_func)(void *)) 
{
	// Validate inputs
	if (!data || count == 0) 
	{
		fprintf(stderr, "%s:%d invalid input parameter\n", __FILE__, __LINE__);
		return NULL;
	}

	// Create a new dynamic array
	dyn_array_t *dyn_array = dyn_array_create(count, data_type_size, destruct_func);
	if (!dyn_array) 
	{
		fprintf(stderr, "%s:%d dynamic arrray allocaiton failedr\n", __FILE__, __LINE__);
		return NULL;
	}

	// Copy data into the dynamic array
	if (dyn_shift_insert(dyn_array, 0, count, MODE_INSERT, data)) 
	{
		return dyn_array; // Success
	}

	// Cleanup in case of failure
	fprintf(stderr, "%s:%d array copy fialed\n", __FILE__, __LINE__);
	dyn_array_destroy(dyn_array);
	return NULL;
}


const void *dyn_array_export(const dyn_array_t *const dyn_array) 
{
    // Check for a valid input
    if (!dyn_array || !dyn_array->array || dyn_array->size == 0) 
    {
		fprintf(stderr, "%s:%d invalid input parameter\n", __FILE__, __LINE__);
        return NULL;
    }

    // Allocate memory for a copy of the data
    void *export_data = malloc(dyn_array->size * dyn_array->data_size);
    if (!export_data) 
    {
		fprintf(stderr, "%s:%d Memory allocation failed \n", __FILE__, __LINE__);
        return NULL; 
    }

    // Copy the contents of the dynamic array into the newly allocated memory
    memcpy(export_data, dyn_array->array, dyn_array->size * dyn_array->data_size);

    // Return the pointer to the copied data
    return export_data;
}


void dyn_array_destroy(dyn_array_t *dyn_array) 
{
    if (!dyn_array) {
		fprintf(stderr, "%s:%d null parameter\n", __FILE__, __LINE__);
        return; 
	}
    // Clear all elements in the dynamic array, applying the destructor if provided
    dyn_array_clear(dyn_array);

    // Free the internal array if it exists
    if (dyn_array->array) 
	{
        free(dyn_array->array);
        dyn_array->array = NULL; // Avoid dangling pointers
    }

    // Free the dynamic array structure itself
    free(dyn_array);
    dyn_array = NULL; // Avoid dangling pointers
}

///
/// Returns a pointer to the object at the front of the array
/// \param dyn_array the dynamic array
/// \return Pointer to front object (NULL on error/empty array)
///
void *dyn_array_front(const dyn_array_t *const dyn_array) 
{
    // Check if the dynamic array is valid and non-empty
    if (dyn_array && dyn_array->size > 0 && dyn_array->array) 
    {
        // Return a pointer to the first element
        return dyn_array->array;  // Already points to the start
    }

    fprintf(stderr, "%s:%d null or invalid parameters\n", __FILE__, __LINE__);
    return NULL;
}


bool dyn_array_push_front(dyn_array_t *const dyn_array, const void *const object) 
{
    // Validate inputs
    if (!dyn_array || !object) 
    {
		fprintf(stderr, "%s:%d null or invalid parameters\n", __FILE__, __LINE__);
        return false; // 
    }

    // Attempt to insert the object at the front of the array
    return dyn_shift_insert(dyn_array, 0, 1, MODE_INSERT, object);
}


bool dyn_array_pop_front(dyn_array_t *const dyn_array) 
{
    // Validate input
    if (!dyn_array) 
    {
		fprintf(stderr, "%s:%d null or invalid parameters\n", __FILE__, __LINE__);
        return false; 
    }

    // Check if the array is empty
    if (dyn_array->size == 0) 
    {
		fprintf(stderr, "%s:%d null array\n", __FILE__, __LINE__);
        return false; 
    }

    // Remove the object at the front of the array
    return dyn_shift_remove(dyn_array, 0, 1, MODE_ERASE, NULL);
}


bool dyn_array_extract_front(dyn_array_t *const dyn_array, void *const object) 
{
    // Validate inputs
    if (!dyn_array || !object) 
    {
		fprintf(stderr, "%s:%d null or invalid parameters\n", __FILE__, __LINE__);
        return false; \
    }

    // Check if the array is empty
    if (dyn_array->size == 0) 
    {
		fprintf(stderr, "%s:%d nothing to extract\n", __FILE__, __LINE__);
        return false; 
    }

    // Extract the object at the front of the array
    return dyn_shift_remove(dyn_array, 0, 1, MODE_EXTRACT, object);
}



void *dyn_array_back(const dyn_array_t *const dyn_array) 
{
    // Validate input
    if (!dyn_array || dyn_array->size == 0) 
    {
		fprintf(stderr, "%s:%d null or invalid parameters\n", __FILE__, __LINE__);
        return NULL; // Invalid array or empty
    }

    // Return the pointer to the last element
    return DYN_ARRAY_POSITION(dyn_array, dyn_array->size - 1);
}

bool dyn_array_push_back(dyn_array_t *const dyn_array, const void *const object) 
{
    // Validate input
    if (!dyn_array || !object) 
    {
		fprintf(stderr, "%s:%d null or invalid parameters\n", __FILE__, __LINE__);
        return false; 
	}
    // Insert the object at the back of the array
    return dyn_shift_insert(dyn_array, dyn_array->size, 1, MODE_INSERT, object);
}

bool dyn_array_pop_back(dyn_array_t *const dyn_array) 
{
    // Validate input
    if (!dyn_array || dyn_array->size == 0) 
    {
		fprintf(stderr, "%s:%d null or invalid parameters\n", __FILE__, __LINE__);
        return false; 
	}
    // Remove the last element
    return dyn_shift_remove(dyn_array, dyn_array->size - 1, 1, MODE_ERASE, NULL);
}


bool dyn_array_extract_back(dyn_array_t *const dyn_array, void *const object) 
{
    // Validate input
    if (!dyn_array || dyn_array->size == 0 || !object) 
    {
		fprintf(stderr, "%s:%d null or invalid parameters\n", __FILE__, __LINE__);
        return false; 
    }

    // Extract the last element
    return dyn_shift_remove(dyn_array, dyn_array->size - 1, 1, MODE_EXTRACT, object);
}



void *dyn_array_at(const dyn_array_t *const dyn_array, const size_t index) 
{
    // Validate inputs
    if (!dyn_array) 
    {
		fprintf(stderr, "%s:%d null or invalid parameters\n", __FILE__, __LINE__);
        return NULL;
    }

    if (index >= dyn_array->size) 
    {
		fprintf(stderr, "%s:%d out of bounds index\n", __FILE__, __LINE__);
        return NULL; 
    }

    // Return a pointer to the element at the given index
    return DYN_ARRAY_POSITION(dyn_array, index);
}


bool dyn_array_insert(dyn_array_t *const dyn_array, const size_t index, const void *const object) 
{
    if (!dyn_array || !object) {
        fprintf(stderr, "%s:%d invalid parameters\n", __FILE__, __LINE__);
        return false;
    }

    
    if (index > dyn_array->size) {
        fprintf(stderr, "%s:%d invalid index: %zu (must be <= %zu)\n", __FILE__, __LINE__, index, dyn_array->size);
        return false;
    }

    
    if (!dyn_array->array) {
        fprintf(stderr, "%s:%d internal array is NULL\n", __FILE__, __LINE__);
        return false;
    }
    return object && dyn_shift_insert(dyn_array, index, 1, MODE_INSERT, object);
}


bool dyn_array_erase(dyn_array_t *const dyn_array, const size_t index) 
{
    // Validate inputs
    if (!dyn_array || index >= dyn_array->size) 
    {
		fprintf(stderr, "%s:%d Invalid parameters\n", __FILE__, __LINE__);
        return false;
    }

    // Delegate removal logic to dyn_shift_remove
    return dyn_shift_remove(dyn_array, index, 1, MODE_ERASE, NULL);
}

bool dyn_array_extract(dyn_array_t *const dyn_array, const size_t index, void *const object) 
{
	 // Validate parameters
    if (!dyn_array || !object || index >= dyn_array->size) 
    {
		fprintf(stderr, "%s:%d  Invalid parameters\n", __FILE__, __LINE__);
        return false;
    }
	return dyn_array && object && dyn_array->size > index
		   && dyn_shift_remove(dyn_array, index, 1, MODE_EXTRACT, object);
}

/// 
/// Removes and optionally destructs all array elements
/// \param dyn_array the dynamic array
///
void dyn_array_clear(dyn_array_t *const dyn_array) 
{
    // Check if dyn_array is NULL
    if (!dyn_array) {
        fprintf(stderr, "%s:%d Invalid parameters: dyn_array is NULL\n", __FILE__, __LINE__);
        return;  // Return early as we can't proceed without a valid array
    }
    // Check if the array is non-empty
    if (dyn_array->size > 0) {
        // If the array is non-empty, proceed to clear it
        dyn_shift_remove(dyn_array, 0, dyn_array->size, MODE_ERASE, NULL);
    } else {
        // Optionally log that the array is empty
        fprintf(stderr, "%s:%d Array is already empty, no elements to remove\n", __FILE__, __LINE__);
    }
}

/// 
/// Tests if the array is empty
/// \param dyn_array the dynamic array
/// \return true if the array is empty (or NULL was passed), false otherwise
///
bool dyn_array_empty(const dyn_array_t *const dyn_array) 
{
    // Check if dyn_array is NULL
    if (!dyn_array ||!dyn_array->array ) {
        fprintf(stderr, "%s:%d Invalid parameter: dyn_array is NULL\n", __FILE__, __LINE__);
        return true;  // If NULL is passed, consider it as empty
    }


    // Return true if the size is zero, false otherwise
    return dyn_array->size == 0;
}

/// 
/// Returns size of the array
/// \param dyn_array the dynamic array
/// \return the size of the array, 0 on error
///
size_t dyn_array_size(const dyn_array_t *const dyn_array) 
{
    // Check if dyn_array is NULL
    if (!dyn_array) {
        fprintf(stderr, "%s:%d Invalid parameter: dyn_array is NULL\n", __FILE__, __LINE__);
        return 0;  // Return 0 for error
    }

    // If dyn_array is valid, return its size
    return dyn_array->size;
}

/// 
/// Returns the current capacity of the array
/// \param dyn_array the dynamic array
/// \return the capacity of the array, 0 on error
///
size_t dyn_array_capacity(const dyn_array_t *const dyn_array) 
{
    // Check if dyn_array is NULL
    if (!dyn_array) {
        fprintf(stderr, "%s:%d Invalid parameter: dyn_array is NULL\n", __FILE__, __LINE__);
        return 0;  // Return 0 for error
    }

    // If dyn_array is valid, return its capacity
    return dyn_array->capacity;
}
/// 
/// Returns the size of the object stored in the array
/// \param dyn_array the dynamic array
/// \return the size of a stored object (bytes), 0 on error
///
size_t dyn_array_data_size(const dyn_array_t *const dyn_array) 
{
    // Check if dyn_array is NULL
    if (!dyn_array) {
        fprintf(stderr, "%s:%d Invalid parameter: dyn_array is NULL\n", __FILE__, __LINE__);
        return 0;  // Return 0 for error
    }

    // If dyn_array is valid, return the size of the stored object
    return dyn_array->data_size;
}

/// 
/// Sorts the array according to the given comparator function
/// compare(x,y) < 0 iff x < y
/// compare(x,y) = 0 iff x == y
/// compare(x,y) > 0 iff y > x
/// Sort is not guaranteed to be stable
/// \param dyn_array the dynamic array
/// \param compare the comparison function
/// \return bool representing success of the operation
///
bool dyn_array_sort(dyn_array_t *const dyn_array, int (*const compare)(const void *, const void *)) 
{
    // Check if dyn_array or compare function is NULL, or if the array is empty
    if (!dyn_array || !compare || dyn_array->size == 0) {
        fprintf(stderr, "%s:%d Invalid parameters: dyn_array is NULL, compare function is NULL, or array is empty\n", __FILE__, __LINE__);
        return false;  // Return false if any parameter is invalid
    }

    // Perform sorting if parameters are valid
    qsort(dyn_array->array, dyn_array->size, dyn_array->data_size, compare);
    return true;  // Return true indicating the sorting operation was successful
}

/// 
/// Inserts the given object into the correct sorted position
/// increasing the container size by one
/// and moving any contents beyond the sorted position down one
/// Note: calling this on an unsorted array will insert it... somewhere
/// \param dyn_array the dynamic array
/// \param object the object to insert
/// \param compare the comparison function
/// \return bool representing success of the operation
///
bool dyn_array_insert_sorted(dyn_array_t *const dyn_array, const void *const object, int (*const compare)(const void *, const void *)) 
{
    // Check if dyn_array, object, or compare is NULL
    if (!dyn_array || !object || !compare) {
        fprintf(stderr, "%s:%d Invalid parameters: dyn_array is NULL, object is NULL, or compare function is NULL\n", __FILE__, __LINE__);
        return false;  // Return false if any parameter is invalid
    }

    // Check if dyn_array has sufficient capacity to insert new element
    if (dyn_array->size >= dyn_array->capacity) {
        fprintf(stderr, "%s:%d Insufficient capacity in dynamic array\n", __FILE__, __LINE__);
        return false;  // Return false if the array is full
    }

    // Determine the correct position to insert the object
    size_t ordered_position = 0;
    if (dyn_array->size) {
        // Find the correct sorted position for the new object
        while (ordered_position < dyn_array->size && compare(object, DYN_ARRAY_POSITION(dyn_array, ordered_position)) > 0) {
            ++ordered_position;
        }
    }

    // Attempt to insert the object at the correct position
    return dyn_shift_insert(dyn_array, ordered_position, 1, MODE_INSERT, object);
}

/// 
/// Applies the given function to every object in the array
/// \param dyn_array the dynamic array
/// \param func the function to apply
/// \param arg argument that will be passed to the function (as parameter 2)
/// \return bool representing success of operation (really just pointer and size checks)
///
bool dyn_array_for_each(dyn_array_t *const dyn_array, void (*const func)(void *const, void *), void *arg) 
{
    // Check if the input parameters are valid (not NULL)
    if (!dyn_array || !dyn_array->array || !func) {
        fprintf(stderr, "%s:%d Invalid parameters passed to dyn_array_for_each\n", __FILE__, __LINE__);
        return false; // Return false if any parameter is NULL
    }

    // Ensure the array has a valid size (non-negative and within bounds)
    if (dyn_array->size == 0) {
        fprintf(stderr, "%s:%d Array is empty, no elements to apply function to\n", __FILE__, __LINE__);
        return false; // Return false if the array is empty
    }

    // Validate that the data array is not corrupted or invalid
    if (!dyn_array->array) {
        fprintf(stderr, "%s:%d Array pointer is NULL or corrupted\n", __FILE__, __LINE__);
        return false; // Return false if the data array is NULL
    }

    // Ensure that each element can be safely accessed
    uint8_t *data_walker = (uint8_t *)dyn_array->array;
    for (size_t idx = 0; idx < dyn_array->size; ++idx, data_walker += dyn_array->data_size) {
        // Apply the given function to each object in the array
        func((void *const)data_walker, arg);
    }

    return true; // Return true if the operation is successful
}



/*
	// No return value. It either goes or it doesn't. shrink_to_fit is more of a request
	void dyn_array_shrink_to_fit(dyn_array_t *const dyn_array) {
	if (dyn_array) {
		void *new_address = realloc(dyn_array->data, DYN_ARRAY_SIZE_N_ELEMS(dyn_array, dyn_array->size));
		if (new_address) {
			dyn_array->data = new_address;
			SET_FLAG(dyn_array,SHRUNK);
		}
	}
	}
*/




//
///
// HERE BE DRAGONS
///
//


// Checks to see if the object can handle an increase in size (and optionally increases capacity)
bool dyn_request_size_increase(dyn_array_t *const dyn_array, const size_t increment);

#define MODE_IS_TYPE(mode, type) ((mode) & (type))

// inserting between idx 1 and 2 (between B and C) means you're moving everything from 2 down to make room
// (can't use traditional insert lingo (new space is following idx) because push_front can't say position -1)
// [A][B][C][D][E][?]
//	   \--F
//		-----> 1
// [A][B][?][C][D][E]
// (and then inserted)
// [A][B][F][C][D][E]
bool dyn_shift_insert(dyn_array_t *const dyn_array, const size_t position, const size_t count,
					  const DYN_SHIFT_MODE mode, const void *const data_src) 
{
	if (dyn_array && count && mode == MODE_INSERT && data_src) 
	{
		// may or may not need to increase capacity.
		// We'll ask the capacity function if we can do it.
		// If we can, do it. If not... Too bad for the user.
		if (position <= dyn_array->size && dyn_request_size_increase(dyn_array, count)) 
		{
			if (position != dyn_array->size) 
			{  // wasn't a gap at the end, we need to move data
				memmove(DYN_ARRAY_POSITION(dyn_array, position + count), DYN_ARRAY_POSITION(dyn_array, position),
						DYN_SIZE_N_ELEMS(dyn_array, dyn_array->size - position));
			}
			memcpy(DYN_ARRAY_POSITION(dyn_array, position), data_src, dyn_array->data_size * count);
			dyn_array->size += count;
			return true;
		}
	}
	return false;
}

// Shifts contents. Mode flag controls what happens and how (duh?)
// So, if you erase idx 1, you're filling a gap of 1 at position 1
// [A][X][B][C][D][E]
//	   <---- 1
// [A][B][C][D][E][?]
bool dyn_shift_remove(dyn_array_t *const dyn_array, const size_t position, const size_t count,
					  const DYN_SHIFT_MODE mode, void *const data_dst) 
{
	if (dyn_array && count && dyn_array->size && MODE_IS_TYPE(mode, TYPE_REMOVE)  // mode = MODE_EXTRACT || MODE_ERASE
		&& (position + count) <= dyn_array->size)   // verify size and range
{ 

		// shrinking in size
		// nice and simple (?)
		if (mode == MODE_ERASE) 
		{
			if (dyn_array->destructor) // erasing AND have deconstructor
			{
				uint8_t *arr_pos = DYN_ARRAY_POSITION(dyn_array, position);
				for (size_t total = count; total; --total, arr_pos += dyn_array->data_size) 
				{
					dyn_array->destructor(arr_pos);
				}
			}
		} 
		else 
		{  // extracting data
			if (data_dst) 
			{
				memcpy(data_dst, DYN_ARRAY_POSITION(dyn_array, position), dyn_array->data_size * count);
			} 
			else 
			{
				return false;  // Extract with no dest??
			}
		}
		// pointer arithmatic on void pointers is illegal nowadays :C
		// GCC allows it for compatability, other provide it for GCC compatability. Way to implement a standard.
		// It should be cast to some sort of byte pointer, which is a pain. Hooray for macros
		if (position + count < dyn_array->size) 
		{
			// there's a actual gap, not just a hole to make at the end
			memmove(DYN_ARRAY_POSITION(dyn_array, position), DYN_ARRAY_POSITION(dyn_array, position + count),
					DYN_SIZE_N_ELEMS(dyn_array, dyn_array->size - (position + count)));
		}
		// decrease the size and return
		dyn_array->size -= count;
		return true;
	}
	return false;
}

bool dyn_request_size_increase(dyn_array_t *const dyn_array, const size_t increment) 
{
	// check to see if the size can be increased by the increment
	// and increase capacity if need be
	// average case will be perfectly fine, single increment
	if (dyn_array) 
	{
		// if (!ADDITION_MAY_OVERFLOW(dyn_array->size, increment)) {
		// increment is ok, but is the capacity?
		if (dyn_array->capacity >= (dyn_array->size + increment)) 
		{
			// capacity is ok!
			return true;
		}
		// have to reallocate, is that even possible?
		size_t needed_size = dyn_array->size + increment;

		// INSERT SHRINK_TO_FIT CORRECTION HERE

		if (needed_size <= DYN_MAX_CAPACITY) 
		{
			size_t new_capacity = dyn_array->capacity << 1;
			while (new_capacity < needed_size) 
			{
				new_capacity <<= 1;
			}

			// we can theoretically hold this, check if we can allocate that
			// if (!MULTIPLY_MAY_OVERFLOW(new_capacity, dyn_array->data_size)) {
			// we won't overflow, so we can at least REQUEST this change
			void *new_array = realloc(dyn_array->array, new_capacity * dyn_array->data_size);
			if (new_array) 
			{
				// success! Wasn't that easy?
				dyn_array->array	= new_array;
				dyn_array->capacity = new_capacity;
				return true;
			}
		}
	}
	return false;
}
