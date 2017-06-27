#ifndef _POOL_H_
#define _POOL_H_

#include "simplelink.h"


typedef void (*pool_constructor)(void *obj);

typedef struct _pool {
    pool_constructor  obj_constructor;

	void    **data;
	_u32 	item_size;
	_u32    pool_size;
	_u32	free_items;
	_u32    max_pool_size;
	_u32	wr_ptr;
	_u32    rd_ptr;
} pool_t;




void	pool_create(pool_t *pool, pool_constructor construct, const _u32 item_size, const _u32 max_pool_size);
_i8	pool_delete(pool_t *pool);
_i8	pool_get(pool_t *pool, void **data);
_i8	pool_release(pool_t *pool, void *data);


#endif
