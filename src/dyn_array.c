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
    if (data_type_size == 0 || capacity > DYN_MAX_CAPACITY) 
    {
        return NULL;
    }

    // Allocate memory for the dynamic array structure
    dyn_array_t *dyn_array = (dyn_array_t *)malloc(sizeof(dyn_array_t));
    if (!dyn_array) 
    {
        return NULL;
    }

    // Determine the actual capacity (rounding up to the nearest power of 2)
    size_t actual_capacity = 16;
    while (actual_capacity < capacity) 
    {
        if (actual_capacity > SIZE_MAX / 2) // Prevent overflow
        {
            free(dyn_array);
            return NULL;
        }
        actual_capacity <<= 1;
    }

    // Allocate memory for the array
    void *array = malloc(data_type_size * actual_capacity);
    if (!array) 
    {
        free(dyn_array);
        return NULL;
    }

    // Initialize the structure
    dyn_array->capacity = actual_capacity;
    dyn_array->size = 0;
    dyn_array->data_size = data_type_size;
    dyn_array->array = array;
    dyn_array->destructor = destruct_func;

    return dyn_array;
}


dyn_array_t *dyn_array_import(const void *const data, const size_t count, const size_t data_type_size, void (*destruct_func)(void *)) 
{
	// Validate inputs
	if (!data || count == 0) 
	{
		return NULL;
	}

	// Create a new dynamic array
	dyn_array_t *dyn_array = dyn_array_create(count, data_type_size, destruct_func);
	if (!dyn_array) 
	{
		return NULL;
	}

	// Copy data into the dynamic array
	if (dyn_shift_insert(dyn_array, 0, count, MODE_INSERT, data)) 
	{
		return dyn_array; // Success
	}

	// Cleanup in case of failure
	dyn_array_destroy(dyn_array);
	return NULL;
}


const void *dyn_array_export(const dyn_array_t *const dyn_array) 
{
    // Check for a valid input
    if (!dyn_array || !dyn_array->array || dyn_array->size == 0) 
    {
        return NULL;
    }

    // Allocate memory for a copy of the data
    void *export_data = malloc(dyn_array->size * dyn_array->data_size);
    if (!export_data) 
    {
        return NULL; // Memory allocation failed
    }

    // Copy the contents of the dynamic array into the newly allocated memory
    memcpy(export_data, dyn_array->array, dyn_array->size * dyn_array->data_size);

    // Return the pointer to the copied data
    return export_data;
}


void dyn_array_destroy(dyn_array_t *dyn_array) 
{
    if (!dyn_array) {
        return; // Nothing to do if the dynamic array is NULL
    }

    // Clear all elements in the dynamic array, applying the destructor if provided
    dyn_array_clear(dyn_array);

    // Free the internal array if it exists
    if (dyn_array->array) {
        free(dyn_array->array);
        dyn_array->array = NULL; // Avoid dangling pointers
    }

    // Free the dynamic array structure itself
    free(dyn_array);
    dyn_array = NULL; // Avoid dangling pointers
}





void *dyn_array_front(const dyn_array_t *const dyn_array) 
{
    // Check if the dynamic array is valid and non-empty
    if (dyn_array && dyn_array->size) 
    {
        // Ensure the internal array pointer is valid
        if (dyn_array->array) 
        {
            return (void *)dyn_array->array; // Return pointer to the first element
        }
    }

    // Return NULL for invalid or empty dynamic arrays
    return NULL;
}


bool dyn_array_push_front(dyn_array_t *const dyn_array, const void *const object) 
{
    // Validate inputs
    if (!dyn_array || !object) 
    {
        return false; // Invalid input
    }

    // Attempt to insert the object at the front of the array
    return dyn_shift_insert(dyn_array, 0, 1, MODE_INSERT, object);
}


bool dyn_array_pop_front(dyn_array_t *const dyn_array) 
{
    // Validate input
    if (!dyn_array) 
    {
        return false; // Invalid input
    }

    // Check if the array is empty
    if (dyn_array->size == 0) 
    {
        return false; // Nothing to pop
    }

    // Remove the object at the front of the array
    return dyn_shift_remove(dyn_array, 0, 1, MODE_ERASE, NULL);
}


bool dyn_array_extract_front(dyn_array_t *const dyn_array, void *const object) 
{
    // Validate inputs
    if (!dyn_array || !object) 
    {
        return false; // Invalid input
    }

    // Check if the array is empty
    if (dyn_array->size == 0) 
    {
        return false; // Nothing to extract
    }

    // Extract the object at the front of the array
    return dyn_shift_remove(dyn_array, 0, 1, MODE_EXTRACT, object);
}





void *dyn_array_back(const dyn_array_t *const dyn_array) 
{
    // Validate input
    if (!dyn_array || dyn_array->size == 0) 
    {
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
        return false; // Invalid parameters
    }

    // Insert the object at the back of the array
    return dyn_shift_insert(dyn_array, dyn_array->size, 1, MODE_INSERT, object);
}

bool dyn_array_pop_back(dyn_array_t *const dyn_array) 
{
    // Validate input
    if (!dyn_array || dyn_array->size == 0) 
    {
        return false; // Invalid input or empty array
    }

    // Remove the last element
    return dyn_shift_remove(dyn_array, dyn_array->size - 1, 1, MODE_ERASE, NULL);
}


bool dyn_array_extract_back(dyn_array_t *const dyn_array, void *const object) 
{
    // Validate input
    if (!dyn_array || dyn_array->size == 0 || !object) 
    {
        return false; // Invalid input or empty array
    }

    // Extract the last element
    return dyn_shift_remove(dyn_array, dyn_array->size - 1, 1, MODE_EXTRACT, object);
}



void *dyn_array_at(const dyn_array_t *const dyn_array, const size_t index) 
{
    // Validate inputs
    if (!dyn_array) 
    {
        return NULL; // Null array pointer
    }

    if (index >= dyn_array->size) 
    {
        return NULL; // Out-of-bounds index
    }

    // Return a pointer to the element at the given index
    return DYN_ARRAY_POSITION(dyn_array, index);
}


bool dyn_array_insert(dyn_array_t *const dyn_array, const size_t index, const void *const object) 
{
    // putting object at INDEX
    // so we shift a gap at INDEX
    return object && dyn_shift_insert(dyn_array, index, 1, MODE_INSERT, object);
}


bool dyn_array_erase(dyn_array_t *const dyn_array, const size_t index) 
{
    // Validate inputs
    if (!dyn_array || index >= dyn_array->size) 
    {
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
        return false;
    }
	return dyn_array && object && dyn_array->size > index
		   && dyn_shift_remove(dyn_array, index, 1, MODE_EXTRACT, object);
}


void dyn_array_clear(dyn_array_t *const dyn_array) 
{
	
	if (dyn_array && dyn_array->size) 
	{
		dyn_shift_remove(dyn_array, 0, dyn_array->size, MODE_ERASE, NULL);
	}
}

bool dyn_array_empty(const dyn_array_t *const dyn_array) 
{
	return dyn_array_size(dyn_array) == 0;
}

size_t dyn_array_size(const dyn_array_t *const dyn_array) 
{
	if (dyn_array) 
	{
		return dyn_array->size;
	}
	return 0;  // hmmmmm...
}

size_t dyn_array_capacity(const dyn_array_t *const dyn_array) 
{
	if (dyn_array) 
	{
		return dyn_array->capacity;
	}
	return 0;  // hmmmmm...
}

size_t dyn_array_data_size(const dyn_array_t *const dyn_array) 
{
	if (dyn_array) 
	{
		return dyn_array->data_size;
	}
	return 0;  // hmmmmm...
}



bool dyn_array_sort(dyn_array_t *const dyn_array, int (*const compare)(const void *, const void *)) 
{
	// hah, turns out there's a quicksort in cstdlib.
	// and it works exactly like we want it to
	if (dyn_array && dyn_array->size && compare) 
	{
		qsort(dyn_array->array, dyn_array->size, dyn_array->data_size, compare);
		return true;
	}
	return false;
}


bool dyn_array_insert_sorted(dyn_array_t *const dyn_array, const void *const object, int (*const compare)(const void *, const void *)) 
{
	if (dyn_array && compare && object) 
	{
		size_t ordered_position = 0;
		if (dyn_array->size) 
		{
			while (ordered_position < dyn_array->size
				   && compare(object, DYN_ARRAY_POSITION(dyn_array, ordered_position)) > 0) 
			{
				++ordered_position;
			}
		}
		return dyn_shift_insert(dyn_array, ordered_position, 1, MODE_INSERT, object);
	}
	return false;
}


bool dyn_array_for_each(dyn_array_t *const dyn_array, void (*const func)(void *const, void *), void *arg) 
{
	if (dyn_array && dyn_array->array && func) 
	{
		// So I just noticed we never check the data array ever
		// Which is both unsafe and potentially undefined behavior
		// Although we're the only ones that touch the pointer and we always validate it.
		// So it's questionable. We'll check it here.
		// I'm considering taking these out. Anything under our control that touches this pointer is safe
		// Not checking it will segfault, which is good for debugging, but not so much for the end user
		// but good for the tester. But the tester may not trigger this if it's a crazy edge case.
		// HMMMMMMMMM...
		uint8_t *data_walker = (uint8_t *) dyn_array->array;
		for (size_t idx = 0; idx < dyn_array->size; ++idx, data_walker += dyn_array->data_size) 
		{
			func((void *const) data_walker, arg);
		}
		return true;
	}
	return false;
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
