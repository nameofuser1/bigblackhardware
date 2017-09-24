#include "pool.h"
#include "logging.h"

static void update_ptr(_u32 *ptr, const _u32 size) {
	_u32 p = *ptr;

	if(p == size - 1) {
		*ptr = 0;
	}
	else {
		*ptr = p+1;
	}
}


void pool_create(pool_t *pool, pool_constructor constructor, const _u32 item_size,
		const _u32 max_pool_size)
{
    pool->obj_constructor = constructor;
	pool->max_pool_size = max_pool_size;
	pool->item_size = item_size;
	pool->pool_size = 0;
	pool->free_items = 0;
	pool->data = mem_Malloc(max_pool_size * sizeof(void*));
}


_i8 pool_delete(pool_t *pool) {
	if(pool->free_items != pool->pool_size) {
		return -1;
	}

	void *data = NULL;
	for(_u32 i=0; i<pool->pool_size; i++) {
		pool_get(pool, data);
		mem_Free(data);
	}

	return 0;
}


_i8 pool_get(pool_t *pool, void **data) {
	_u32 pool_size = pool->pool_size;
	_u32 pool_msize = pool->max_pool_size;
	_u32 pool_free_items = pool->free_items;
	pool_constructor constructor = pool->obj_constructor;

	if((pool_free_items == 0) && (pool_size == pool_msize)) {
		return -1;
	}
	else if(pool_free_items == 0) {
       *data = mem_Malloc(pool->item_size);
        pool->pool_size++;

        if(constructor != NULL) {
            constructor(*data);
        }
	}
	else {
		*data = pool->data[pool->free_items-1];
		pool->free_items--;
	}

	return 0;
}


_i8 pool_release(pool_t *pool, void *data) {
	_u32 pool_size = pool->pool_size;
	_u32 pool_free_items = pool->free_items;

	if(pool_free_items == pool_size) {
		return -1;
	}

	pool->data[pool->free_items++] = data;

	return 0;
}
