#include "fs.h"

#include "event.h"
#include "heap.h"
#include "queue.h"
#include "thread.h"

#include <string.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "lz4/lz4.h"

typedef struct fs_t
{
	heap_t* heap;
	queue_t* file_queue;
	queue_t* compression_queue;
	queue_t* decompression_queue;
	thread_t* file_thread;
	thread_t* compression_thread;
	thread_t* decompression_thread;
} fs_t;

typedef enum fs_work_op_t
{
	k_fs_work_op_read,
	k_fs_work_op_write,
} fs_work_op_t;

typedef struct fs_work_t
{
	heap_t* heap;
	fs_work_op_t op;
	char path[1024];
	bool null_terminate;
	bool use_compression;
	void* buffer;
	size_t size;
	event_t* done;
	int result;
	int compression_size;
	fs_t* fs;
} fs_work_t;

static int file_thread_func(void* user);
static int compression_thread_func(void* user);
static int decompression_thread_func(void* user);

fs_t* fs_create(heap_t* heap, int queue_capacity)
{
	fs_t* fs = heap_alloc(heap, sizeof(fs_t), 8);
	fs->heap = heap;
	fs->file_queue = queue_create(heap, queue_capacity);
	fs->compression_queue = queue_create(heap, queue_capacity);
	fs->decompression_queue = queue_create(heap, queue_capacity);
	fs->file_thread = thread_create(file_thread_func, fs);
	fs->compression_thread = thread_create(compression_thread_func, fs);
	fs->decompression_thread = thread_create(decompression_thread_func, fs);
	return fs;
}

void fs_destroy(fs_t* fs)
{
	queue_push(fs->file_queue, NULL);
	queue_push(fs->compression_queue, NULL);
	queue_push(fs->decompression_queue, NULL);
	thread_destroy(fs->file_thread);
	thread_destroy(fs->compression_thread);
	thread_destroy(fs->decompression_thread);
	queue_destroy(fs->file_queue);
	queue_destroy(fs->compression_queue);
	queue_destroy(fs->decompression_queue);
	heap_free(fs->heap, fs);
}

fs_work_t* fs_read(fs_t* fs, const char* path, heap_t* heap, bool null_terminate, bool use_compression)
{
	fs_work_t* work = heap_alloc(fs->heap, sizeof(fs_work_t), 8);
	work->heap = heap;
	work->op = k_fs_work_op_read;
	strcpy_s(work->path, sizeof(work->path), path);
	work->buffer = NULL;
	work->size = 0;
	work->done = event_create();
	work->result = 0;
	work->null_terminate = null_terminate;
	work->use_compression = use_compression;
	work->fs = fs;
	queue_push(fs->file_queue, work);
	return work;
}

fs_work_t* fs_write(fs_t* fs, const char* path, const void* buffer, size_t size, bool use_compression)
{
	fs_work_t* work = heap_alloc(fs->heap, sizeof(fs_work_t), 8);
	work->heap = fs->heap;
	work->op = k_fs_work_op_write;
	strcpy_s(work->path, sizeof(work->path), path);
	work->buffer = (void*)buffer;
	work->size = size;
	work->done = event_create();
	work->result = 0;
	work->null_terminate = false;
	work->use_compression = use_compression;
	work->fs = fs;

	if (use_compression)
	{
		// HOMEWORK 2: Queue file write work on compression queue!
		queue_push(fs->compression_queue, work);
	}
	else
	{
		queue_push(fs->file_queue, work);
	}

	return work;
}

bool fs_work_is_done(fs_work_t* work)
{
	return work ? event_is_raised(work->done) : true;
}

void fs_work_wait(fs_work_t* work)
{
	if (work)
	{
		event_wait(work->done);
	}
}

int fs_work_get_result(fs_work_t* work)
{
	fs_work_wait(work);
	return work ? work->result : -1;
}

void* fs_work_get_buffer(fs_work_t* work)
{
	fs_work_wait(work);
	return work ? work->buffer : NULL;
}

size_t fs_work_get_size(fs_work_t* work)
{
	fs_work_wait(work);
	return work ? work->size : 0;
}

void fs_work_destroy(fs_work_t* work)
{
	if (work)
	{
		event_wait(work->done);
		event_destroy(work->done);
		heap_free(work->heap, work);
	}
}

static void file_read(fs_work_t* work)
{
	wchar_t wide_path[1024];
	if (MultiByteToWideChar(CP_UTF8, 0, work->path, -1, wide_path, sizeof(wide_path)) <= 0)
	{
		work->result = -1;
		event_signal(work->done);
		return;
	}

	HANDLE handle = CreateFile(wide_path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		work->result = GetLastError();
		event_signal(work->done);
		return;
	}

	if (!GetFileSizeEx(handle, (PLARGE_INTEGER)&work->size))
	{
		work->result = GetLastError();
		CloseHandle(handle);
		event_signal(work->done);
		return;
	}

	work->buffer = heap_alloc(work->heap, work->null_terminate ? work->size + 1 : work->size, 8);

	DWORD bytes_read = 0;

	if (work->use_compression)
	{
		if (!ReadFile(handle, &work->compression_size, (DWORD)4, &bytes_read, NULL))
		{
			work->result = GetLastError();
			CloseHandle(handle);
			return;
		}

		if (!ReadFile(handle, &work->size, (DWORD)4, &bytes_read, NULL))
		{
			work->result = GetLastError();
			CloseHandle(handle);
			return;
		}

		if (!ReadFile(handle, work->buffer, (DWORD)work->compression_size, &bytes_read, NULL))
		{
			work->result = GetLastError();
			CloseHandle(handle);
			return;
		}
	}
	else
	{
		if (!ReadFile(handle, work->buffer, (DWORD)work->size, &bytes_read, NULL))
		{
			work->result = GetLastError();
			CloseHandle(handle);
			return;
		}
		work->size = bytes_read;
		if (work->null_terminate)
		{
			((char*)work->buffer)[bytes_read] = 0;
		}
	}

	CloseHandle(handle);

	if (work->use_compression)
	{
		// HOMEWORK 2: Queue file read work on decompression queue!
		queue_push(work->fs->decompression_queue, work);
	}
	else
	{
		event_signal(work->done);
	}

}

static void file_write(fs_work_t* work)
{
	wchar_t wide_path[1024];
	if (MultiByteToWideChar(CP_UTF8, 0, work->path, -1, wide_path, sizeof(wide_path)) <= 0)
	{
		work->result = -1;
		event_signal(work->done);
		return;
	}

	HANDLE handle = CreateFile(wide_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		work->result = GetLastError();
		event_signal(work->done);
		return;
	}
	
	DWORD bytes_written = 0;
	if (work->use_compression) 
	{
		//Write the compressed size of the file to the front of the file
		if (!WriteFile(handle, &work->compression_size, (DWORD)4, &bytes_written, NULL))
		{
			work->result = GetLastError();
			CloseHandle(handle);
			return;
		}

		//Write the decompressed size of the file to the front of the file
		if (!WriteFile(handle, &work->size, (DWORD)4, &bytes_written, NULL))
		{
			work->result = GetLastError();
			CloseHandle(handle);
			return;
		}

		if (!WriteFile(handle, work->buffer, (DWORD)work->compression_size, &bytes_written, NULL))
		{
			work->result = GetLastError();
			CloseHandle(handle);
			return;
		}

		heap_free(work->heap, work->buffer);

	}
	else 
	{
		if (!WriteFile(handle, work->buffer, (DWORD)work->size, &bytes_written, NULL))
		{
			work->result = GetLastError();
			CloseHandle(handle);
			return;
		}

		work->size = bytes_written;
	}


	CloseHandle(handle);

	event_signal(work->done);

}

static void file_compress(fs_work_t* work)
{
	char* temp_buffer = work->buffer;
	int dest_size = LZ4_compressBound((int)work->size);
	work->buffer = heap_alloc(work->heap, (size_t)(dest_size), 8);
	int compressed_size = LZ4_compress_default(temp_buffer, work->buffer, (int)work->size, dest_size);
	work->compression_size = compressed_size;
	file_write(work);
}

static void file_decompress(fs_work_t* work)
{
	char* dest_buffer = heap_alloc(work->heap, work->size, 8);
	char* temp_buffer = work->buffer;
	LZ4_decompress_safe(work->buffer, dest_buffer, work->compression_size, (int)work->size);
	work->buffer = dest_buffer;
	heap_free(work->heap, temp_buffer);
	if (work->null_terminate) 
	{
		((char*)work->buffer)[work->size] = 0;
	}
	
	event_signal(work->done);
}

static int file_thread_func(void* user)
{
	fs_t* fs = user;
	while (true)
	{
		fs_work_t* work = queue_pop(fs->file_queue);
		if (work == NULL)
		{
			break;
		}
		
		switch (work->op)
		{
		case k_fs_work_op_read:
			file_read(work);
			break;
		case k_fs_work_op_write:
			file_write(work);
			break;
		}
	}
	return 0;
}

static int compression_thread_func(void* user)
{
	fs_t* fs = user;
	while (true)
	{
		fs_work_t* work = queue_pop(fs->compression_queue);
		if (work == NULL)
		{
			break;
		}

		file_compress(work);
	}
	return 0;
}

static int decompression_thread_func(void* user) 
{
	fs_t* fs = user;
	while (true)
	{
		fs_work_t* work = queue_pop(fs->decompression_queue);
		if (work == NULL)
		{
			break;
		}

		file_decompress(work);
	}
	return 0;
}