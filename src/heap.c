#include "heap.h"

#include "debug.h"
#include "tlsf/tlsf.h"

#include <stddef.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <DbgHelp.h>

typedef struct arena_t
{
	pool_t pool;
	struct arena_t* next;
} arena_t;

typedef struct alloc_info_t
{
	size_t size;
	void* address;
	char* stack[256];
	int stack_frames;
	struct alloc_info_t* next;
} alloc_info_t;

typedef struct heap_t
{
	tlsf_t tlsf;
	size_t grow_increment;
	arena_t* arena;
	alloc_info_t* first_alloc;
} heap_t;

heap_t* heap_create(size_t grow_increment)
{
	heap_t* heap = VirtualAlloc(NULL, sizeof(heap_t) + tlsf_size(),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!heap)
	{
		debug_print(
			k_print_error,
			"OUT OF MEMORY!\n");
		return NULL;
	}

	heap->grow_increment = grow_increment;
	heap->tlsf = tlsf_create(heap + 1);
	heap->arena = NULL;
	heap->first_alloc= NULL;

	return heap;
}

void* heap_alloc(heap_t* heap, size_t size, size_t alignment)
{
	void* address = tlsf_memalign(heap->tlsf, alignment, size);
	if (!address)
	{
		size_t arena_size =
			__max(heap->grow_increment, size * 2) +
			sizeof(arena_t);
		arena_t* arena = VirtualAlloc(NULL,
			arena_size + tlsf_pool_overhead(),
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!arena)
		{
			debug_print(
				k_print_error,
				"OUT OF MEMORY!\n");
			return NULL;
		}

		arena->pool = tlsf_add_pool(heap->tlsf, arena + 1, arena_size);

		arena->next = heap->arena;
		heap->arena = arena;

		address = tlsf_memalign(heap->tlsf, alignment, size);

	}

	alloc_info_t* info = VirtualAlloc(NULL, sizeof(alloc_info_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!info)
	{
		debug_print(
			k_print_error,
			"OUT OF MEMORY!\n");
		return NULL;
	}
	info->next = heap->first_alloc;
	heap->first_alloc = info;
	heap->first_alloc->size = size;
	heap->first_alloc->address = address;

	info->stack_frames = CaptureStackBackTrace(1, 256, info->stack, NULL);

	return address;
}

void heap_free(heap_t* heap, void* address)
{
	alloc_info_t* current = heap->first_alloc;
	if(current != NULL && current->address == address)
	{
		heap->first_alloc = current->next;
		VirtualFree(current, 0, MEM_RELEASE);
	}
	else if(current != NULL)
	{
		while (current->next) 
		{
			if(current->next->address == address)
			{
				alloc_info_t* temp = current->next->next;
				VirtualFree(current->next, 0, MEM_RELEASE);
				current->next = temp;
				break;
			}
			current = current->next;
		}
	}
	tlsf_free(heap->tlsf, address);
}

void heap_destroy(heap_t* heap)
{

	tlsf_destroy(heap->tlsf);
	alloc_info_t* current = heap->first_alloc;

	while (current)
	{
		DWORD  error;
		HANDLE h_process;

		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

		h_process = GetCurrentProcess();

		if (!SymInitialize(h_process, NULL, TRUE))
		{
			// SymInitialize failed
			error = GetLastError();
			debug_print(
				k_print_error,
				"SymInitialize returned error : %d\n",
				error);
			return;
		}

		char symbol_mem[sizeof(IMAGEHLP_SYMBOL64) + 256];
		IMAGEHLP_SYMBOL64* symbol = (IMAGEHLP_SYMBOL64*)symbol_mem;
		symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		symbol->MaxNameLength = 255;

		printf("Memory leak of size %zu bytes with callstack:\n", current->size);
		for(int i = 0; i < current->stack_frames; ++i)
		{
			SymGetSymFromAddr64(h_process, (DWORD64)current->stack[i], NULL, symbol);
			printf("[%d] %s\n", i, symbol->Name);
		}

		alloc_info_t* temp = current;
		current = current->next;
		VirtualFree(temp, 0, MEM_RELEASE);
		SymCleanup(h_process);
	}
	
	arena_t* arena = heap->arena;
	while (arena)
	{
		arena_t* next = arena->next;
		VirtualFree(arena, 0, MEM_RELEASE);
		arena = next;
	}

	VirtualFree(heap, 0, MEM_RELEASE);
}
