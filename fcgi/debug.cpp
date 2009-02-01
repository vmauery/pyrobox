#include <debug.hpp>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef DEBUG

int gettid(void)
{
	return syscall(__NR_gettid);
}

typedef struct {
	int lock;
	int tid;
	int count;
} spinlock_t;

static inline unsigned long cmpxchg(volatile void *ptr, int oldv, int newv) {
	if (*(unsigned long*)ptr == oldv) {
		*(unsigned long*)ptr = newv;
		return oldv;
	}
	return *(unsigned long*)ptr;
	unsigned long prev = 0;
	asm volatile("lock cmpxchgl %k1,%2"
			: "=a"(prev)
			: "r"(newv), "m"(ptr), "0"(oldv)
			: "memory");
	return prev;
}

static inline void spin_lock(spinlock_t *l) {
	int tid = gettid();
	if (l->tid == tid) {
		l->count++;
		return;
	}
	while (cmpxchg(&l->lock, 0, 1) != 0);
	l->count = 1;
	l->tid = tid;
}

static inline void spin_unlock(spinlock_t *l) {
	if (--l->count == 0) {
		l->tid = 0;
		l->lock = 0;
	}
}

static spinlock_t lock = { 0 };

static void *(*old_malloc_hook)(size_t, const void *);
static void *(*old_realloc_hook)(void *, size_t, const void *);
static void (*old_free_hook)(void *, const void *);

static void debug_mem_start();
static void debug_mem_stop();

static int mallocs = 0;
static int reallocs = 0;
static int frees = 0;

enum memstate {
	mem_unused = 0,
	mem_used,
	mem_freed,
};
struct meminfo {
	void *addr;
	const void *caller;
	size_t size;
	enum memstate state;
};

#define INFO_COUNT 4096
static struct meminfo info[INFO_COUNT];

#define PAD_SIZE 4096
#define PAD_SIZE2 (PAD_SIZE*2)

void *my_malloc(size_t size, const void *caller) {
	spin_lock(&lock);
	int id = mallocs++;
	struct meminfo * inf = &info[id];
	void *result;
	size_t new_size = PAD_SIZE2 + size;
	debug_mem_stop();
	result = malloc(new_size);
	inf->size = (size_t)size;
	inf->state = mem_used;
	inf->addr = (void*)((char*)result + PAD_SIZE);
	inf->caller = caller;
	fprintf(stderr, "malloc(%d) = %#x\n", size, inf->addr);
	debug_mem_start();
	memset(result, 23, new_size);
	spin_unlock(&lock);
	return inf->addr;
}

void *my_realloc(void *addr, size_t size, const void *caller) {
	spin_lock(&lock);
	reallocs++;
	void *result;
	size_t new_size = PAD_SIZE2 + size;
	struct meminfo *inf = info + INFO_COUNT;
	while (--inf >= info) {
		if (inf->addr == addr)
			break;
	}
	if (info > inf) {
		fprintf(stderr, "realloc called with %#x, which was never malloced\n");
	}
	debug_mem_stop();
	result = malloc(new_size);
	inf->size = (size_t)size;
	inf->state = mem_used;
	inf->addr = (void*)((char*)result + PAD_SIZE);
	inf->caller = caller;
	memset(result, 23, new_size);
	memcpy(inf->addr, addr, size);
	free((void*)((char*)addr - PAD_SIZE));
	debug_mem_start();
	spin_unlock(&lock);
	return inf->addr;
}

void my_free(void *addr, const void *caller) {
	spin_lock(&lock);
	frees++;
	void *real_addr;
	unsigned char *f, *b;
	debug_mem_stop();
	fprintf(stderr, "free(%#x)\n", addr);
	if (addr == NULL) {
		debug_mem_start();
		spin_unlock(&lock);
		return;
	}
	struct meminfo *inf = info + INFO_COUNT;
	while (--inf >= info) {
		if (inf->addr == addr)
			break;
	}
	if (info > inf) {
		fprintf(stderr, "free called with %#x, which was never malloced\n", addr);
		debug_mem_start();
		spin_unlock(&lock);
		return;
	}
	if (inf->state != mem_used) {
		const char *msg;
		switch (inf->state) {
			case mem_unused: msg = "unallocated"; break;
			case mem_freed: msg = "freed"; break;
			default: msg = "unknown"; break;
		}
		fprintf(stderr, "free called with %#x, which is is in state %s\n", msg);
	}
	real_addr = (void*)((char*)addr - PAD_SIZE);
	f = (unsigned char*)real_addr;
	b = (unsigned char*)((char*)addr + inf->size);

	for (int i=0; i<PAD_SIZE; i++, f++, b++) {
		if (*f != 23)
			fprintf(stderr, "memory corruption at %#x (-%#x) = %#2x\n", addr, (unsigned long)addr-(unsigned long)f, *f);
		if (*b != 23)
			fprintf(stderr, "memory corruption at %#x (+%#x) = %#2x\n", addr, (unsigned long)b-(unsigned long)addr, *b);
	}
	free(real_addr);
	debug_mem_start();
	spin_unlock(&lock);
}

static void debug_mem_start() {
	__malloc_hook = my_malloc;
	__realloc_hook = my_realloc;
	__free_hook = my_free;
}

static void debug_mem_stop() {
	__malloc_hook = old_malloc_hook;
	__realloc_hook = old_realloc_hook;
	__free_hook = old_free_hook;
}

void debug_mem_init() {
	old_malloc_hook = __malloc_hook;
	old_realloc_hook = __realloc_hook;
	old_free_hook = __free_hook;
	debug_mem_start();
}

void debug_mem_fini() {
	fprintf(stderr, "debug_mem_stats: malloc: %d, realloc: %d, free: %d\n", mallocs, reallocs, frees);
}

 /* Override initializing hook from the C library. */
void (*__malloc_initialize_hook) (void) = debug_mem_init;


#endif


