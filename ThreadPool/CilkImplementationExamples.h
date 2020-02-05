#pragma once

#ifdef PLAYING_WITH_CILK

struct __cilkrts_stack_frame;

#define __CILKRTS_ABI_VERSION  2

struct __cilkrts_worker_sysdep_state;
using ls_magic_t = int;
using cilk_worker_type = int;

using   = int;
using = int;

using __cilkrts_pedigree = int;

using cilk_fiber = int;

using cilk_fiber_pool = int;

struct local_state  /* COMMON_PORTABLE */
{
	/** This value should be in the first field in any local_state */
#   define WORKER_MAGIC_0 ((ls_magic_t)0xe0831a4a940c60b8ULL)

	ls_magic_t worker_magic_0;
	  std::mutex lock;
	int do_not_steal;
	  std::mutex steal_lock;
	struct full_frame *frame_ff;
	struct full_frame *next_frame_ff;
	struct full_frame *last_full_frame;

	__attribute__((aligned(64)))
		__cilkrts_worker *team;

	cilk_worker_type type;

	__attribute__((aligned(64)))
		__cilkrts_stack_frame **ltq;

	cilk_fiber_pool fiber_pool;

	cilk_fiber* scheduling_fiber;

	__cilkrts_pedigree* original_pedigree_leaf;

	unsigned rand_seed;

	scheduling_stack_fcn_t post_suspend;

	__cilkrts_stack_frame *suspended_stack;

	cilk_fiber* fiber_to_free;

	struct pending_exception_info *pending_exception;

	struct free_list *free_list[FRAME_MALLOC_NBUCKETS];

	size_t bucket_potential[FRAME_MALLOC_NBUCKETS];

	statistics* stats;

	unsigned int steal_failure_count;

	int has_stolen;

	int work_stolen;

	FILE *record_replay_fptr;

	replay_entry_t *replay_list_root;

	replay_entry_t *replay_list_entry;
	char buf[64];

	signal_node_t *signal_node;

	/** This value should be in the last field in any local_state */
#   define WORKER_MAGIC_1 ((ls_magic_t)0x16164afb0ea0dff9ULL)

	ls_magic_t worker_magic_1;
};


struct __cilkrts_worker {
	__cilkrts_stack_frame *volatile *volatile tail;
	__cilkrts_stack_frame *volatile *volatile head;  /**< @copydoc tail */
	__cilkrts_stack_frame *volatile *volatile exc;   /**< @copydoc tail */

	__cilkrts_stack_frame *volatile *volatile protected_tail;

	__cilkrts_stack_frame *volatile *ltq_limit;

	int32_t self;

	global_state_t *g;

	local_state *l;

	cilkred_map *reducer_map;

	__cilkrts_stack_frame *current_stack_frame;

	void* reserved;

	__cilkrts_worker_sysdep_state *sysdep;

#if __CILKRTS_ABI_VERSION >= 1
	__cilkrts_pedigree   pedigree;
#endif  /* __CILKRTS_ABI_VERSION >= 1 */
};


struct __cilkrts_stack_frame
{
	uint32_t flags;
	int32_t size;	/** Not currently used.  Not initialized by Intel compiler. */

	__cilkrts_stack_frame *call_parent;
	__cilkrts_worker *worker;
	void *except_data;
	__CILK_JUMP_BUFFER ctx;

#if __CILKRTS_ABI_VERSION >= 1
	uint32_t mxcsr;
	uint16_t fpcsr;         /**< @copydoc mxcsr */

	uint16_t reserved;
	union
	{
		__cilkrts_pedigree spawn_helper_pedigree; /* Used in spawn helpers */
		__cilkrts_pedigree parent_pedigree;       /* Used in spawning funcs */
	};
#endif  /* __CILKRTS_ABI_VERSION >= 1 */
};




#include<mutex>
#endif