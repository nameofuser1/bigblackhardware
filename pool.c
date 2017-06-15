#include "pool.h"

#include <mem.h>
#include <os_type.h>
#include <espmissingincludes.h>


static void update_ptr(uint32_t *ptr, uint32_t size) {
	uint32_t p = *ptr;

	if(p == size - 1) {
		*ptr = 0;
	}
	else {
		*ptr = p+1;
	}
}


void pool_create(pool_t *pool, const uint32_t item_size, 
		const uint32_t max_pool_size) 
{
	pool->max_pool_size = max_pool_size;
	pool->item_size = item_size;
	pool->pool_size = 0;
	pool->free_items = 0;
	pool->data = os_malloc(max_pool_size * sizeof(void*));
}


BOOL pool_delete(pool_t *pool) {
	if(pool->free_items != pool->pool_size) {
		return FALSE;
	}

	void *data = NULL;
	for(uint32_t i=0; i<pool->pool_size; i++) {
		pool_get(pool, data);
		os_free(data);
	}

	return TRUE;
}


BOOL pool_get(pool_t *pool, void **data) {
	uint32_t pool_size = pool->pool_size;
	uint32_t pool_msize = pool->max_pool_size;
	uint32_t pool_free_items = pool->free_items;

	if((pool_free_items == 0) && (pool_size == pool_msize)) {
		return FALSE;
	}
	else if(pool_free_items == 0) {
		*data = os_malloc(pool->item_size);
		pool->pool_size++;
	}
	else {
		*data = pool->data[pool->free_items-1];
		pool->free_items--;
	}

	return TRUE;
}


BOOL pool_release(pool_t *pool, void *data) {
	uint32_t pool_size = pool->pool_size;
	uint32_t pool_free_items = pool->free_items;

	if(pool_free_items == pool_size) {
		return FALSE;
	}

	pool->data[pool->free_items++] = data;

	return TRUE;
}
