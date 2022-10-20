#include "trace.h"
#include "heap.h"
#include "timer.h"
#include "atomic.h"
#include "mutex.h"
#include "fs.h"
 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h> 

typedef struct trace_t
{
	heap_t* heap;
	size_t max_events;
	size_t event_count;
	size_t start_timestamp;
	bool on;
	const char* path;
	char* buffer;
	stack_t* name_stack;
	mutex_t* mutex;
} trace_t;

typedef struct stack_t
{
	stack_element_t* tail;
} stack_t;

typedef struct stack_element_t
{
	const char* name;
	stack_element_t* prev;
	stack_element_t* next;
} stack_element_t;


trace_t* trace_create(heap_t* heap, int event_capacity)
{
	trace_t* trace = heap_alloc(heap, sizeof(trace_t), 8);
	trace->heap = heap;
	trace->max_events = (size_t)event_capacity;
	trace->event_count = 0;
	trace->start_timestamp = timer_ticks_to_us(timer_get_ticks());
	trace->on = false;
	trace->buffer = heap_alloc(heap, 4096, 8);
	const char* start_string = "{\n\"displayTimeUnit\": \"ns\", \"traceEvents\" : [\n";
	strcpy_s(trace->buffer, 4096, start_string);
	trace->name_stack = heap_alloc(heap, sizeof(stack_t), 8);
	trace->name_stack->tail = NULL;
	trace->mutex = mutex_create();
	return trace;
}

void trace_destroy(trace_t* trace)
{
	stack_element_t* current = trace->name_stack->tail;
	stack_element_t* prev;
	heap_free(trace->heap, trace->buffer);
	//Loop through the stack and free any existing elements
	while (current != NULL) 
	{
		prev = current->prev;
		heap_free(trace->heap, current);
		current = prev;
	}
	mutex_destroy(trace->mutex);
	heap_free(trace->heap, trace->name_stack);
	heap_free(trace->heap, trace);
}

void trace_duration_push(trace_t* trace, const char* name)
{
	if (!trace->on) return;
	atomic_increment(&(int)(trace->event_count));
	//Convert the pid, tid, and ts's into strings
	int pid_len = snprintf(NULL, 0, "%d", GetCurrentProcessId()) + 1;
	int tid_len = snprintf(NULL, 0, "%d", GetCurrentThreadId()) + 1;
	int cur_ts = (int)(timer_ticks_to_us(timer_get_ticks()) - trace->start_timestamp);
	int ts_len = snprintf(NULL, 0, "%d", cur_ts) + 1;
	char* pid = heap_alloc(trace->heap, pid_len, 8);
	char* tid = heap_alloc(trace->heap, tid_len, 8);
	char* ts = heap_alloc(trace->heap, ts_len, 8);
	snprintf(pid, pid_len, "%d", GetCurrentProcessId());
	snprintf(tid, tid_len, "%d", GetCurrentThreadId());
	snprintf(ts, ts_len, "%d", cur_ts);
	//Append a formatted event to the buffer
	mutex_lock(trace->mutex);
	strcat_s(trace->buffer, 4096, "{\"name\":\"");
	strcat_s(trace->buffer, 4096, name);
	strcat_s(trace->buffer, 4096, "\",\"ph\":\"B\",\"pid\":");
	strcat_s(trace->buffer, 4096, pid);
	strcat_s(trace->buffer, 4096, ",\"tid\":\"");
	strcat_s(trace->buffer, 4096, tid);
	strcat_s(trace->buffer, 4096, "\",\"ts\":\"");
	strcat_s(trace->buffer, 4096, ts);
	strcat_s(trace->buffer, 4096, "\"},\n");
	heap_free(trace->heap, pid);
	heap_free(trace->heap, tid);
	heap_free(trace->heap, ts);
	//Push this event onto the stack
	stack_element_t* entry = heap_alloc(trace->heap, sizeof(stack_element_t), 8);
	entry->name = name;
	if (trace->name_stack->tail == NULL)
	{
		trace->name_stack->tail = entry;
	}
	else
	{
		trace->name_stack->tail->next = entry;
		entry->prev = trace->name_stack->tail;
		trace->name_stack->tail = entry;
	}
	mutex_unlock(trace->mutex);
}

void trace_duration_pop(trace_t* trace)
{
	if (!trace->on) return;
	//Get the name, pid, and tid from the stack
	atomic_increment(&(int)(trace->event_count));
	//Convert the pid, tid, and ts's into strings
	int pid_len = snprintf(NULL, 0, "%d", GetCurrentProcessId()) + 1;
	int tid_len = snprintf(NULL, 0, "%d", GetCurrentThreadId()) + 1;
	int cur_ts = (int)(timer_ticks_to_us(timer_get_ticks()) - trace->start_timestamp);
	int ts_len = snprintf(NULL, 0, "%d", cur_ts) + 1;
	char* pid = heap_alloc(trace->heap, pid_len, 8);
	char* tid = heap_alloc(trace->heap, tid_len, 8);
	char* ts = heap_alloc(trace->heap, ts_len, 8);
	snprintf(pid, pid_len, "%d", GetCurrentProcessId());
	snprintf(tid, tid_len, "%d", GetCurrentThreadId());
	snprintf(ts, ts_len, "%d", cur_ts);
	//Append a formatted event to the buffer
	mutex_lock(trace->mutex);
	const char* name = trace->name_stack->tail->name;
	strcat_s(trace->buffer, 4096, "{\"name\":\"");
	strcat_s(trace->buffer, 4096, name);
	strcat_s(trace->buffer, 4096, "\",\"ph\":\"E\",\"pid\":");
	strcat_s(trace->buffer, 4096, pid);
	strcat_s(trace->buffer, 4096, ",\"tid\":\"");
	strcat_s(trace->buffer, 4096, tid);
	strcat_s(trace->buffer, 4096, "\",\"ts\":\"");
	strcat_s(trace->buffer, 4096, ts);
	strcat_s(trace->buffer, 4096, "\"},\n");

	heap_free(trace->heap, pid);
	heap_free(trace->heap, tid);
	heap_free(trace->heap, ts);
	//Pop the event off the stack
	stack_element_t* temp = trace->name_stack->tail;
	trace->name_stack->tail = trace->name_stack->tail->prev;
	heap_free(trace->heap, temp);
	mutex_unlock(trace->mutex);
}

void trace_capture_start(trace_t* trace, const char* path)
{
	trace->on = true;
	trace->path = path;
}

void trace_capture_stop(trace_t* trace)
{
	trace->on = false;
	//Remove an extra comma
	trace->buffer[strlen(trace->buffer) - 2] = ' ';
	strcat_s(trace->buffer, 4096, "]\n}");
	fs_t* fs = fs_create(trace->heap, 16);
	fs_work_t* write_work = fs_write(fs, trace->path, trace->buffer, strlen(trace->buffer), false);
	fs_work_destroy(write_work);
	fs_destroy(fs);
}
