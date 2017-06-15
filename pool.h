#ifndef _POOL_H_
#define _POOL_H_

#include "simplelink.h"

typedef struct _pool {
	void 		**data;
	_u32 	    item_size;
	_u32    	pool_size;
	_u32	    free_items;
	_u32      	max_pool_size;
	_u32	    wr_ptr;
	_u32    	rd_ptr;
} pool_t;


void	pool_create(pool_t *pool, const uint32_t item_size, const uint32_t max_pool_size);
BOOL	pool_delete(pool_t *pool);
BOOL	pool_get(pool_t *pool, void **data);
BOOL	pool_release(pool_t *pool, void *data);


#endif
