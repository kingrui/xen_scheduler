#ifndef __BITMAP_H__
#define __BITMAP_H__ 1024

#include<malloc.h>
#include<string.h>

#define MAP_UNIT_LEN 8

#if MAP_UNIT_LEN == 8
typedef uint8_t *bitmap;
#elif MAP_UNIT_LEN == 16
typedef uint16_t *bitmap
#elif MAP_UNIT_LEN == 32
typedef uint32_t *bitmap
#elif MAP_UNIT_LEN == 64
typedef uint64_t *bitmap
#else
typedef uint8_t *bitmap;
#undef MAP_UNIT_LEN
#define MAP_UNIT_LEN 8
#endif

#define set_bit(bitmap, nr) (bitmap[(nr) / MAP_UNIT_LEN] |= 1 << ((nr) % MAP_UNIT_LEN))

#define get_bit(bitmap, nr) ((bitmap[(nr) / MAP_UNIT_LEN] & 1 << ((nr) % MAP_UNIT_LEN)) != 0)

#define cal_bitmap_len(nr) ((unsigned)((nr) + MAP_UNIT_LEN - 1) / MAP_UNIT_LEN)

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

static inline void * alloc_bitmap(uint32_t nr)
{
	uint32_t map_len = cal_bitmap_len(nr);
	void *ptr = malloc(map_len * (MAP_UNIT_LEN / 8));
	if(ptr)
		memset(ptr, 0, map_len);
	return ptr;	
}

static inline int bitmap_copy(bitmap dst, bitmap src, uint32_t map_len)
{
	int i;
	if(unlikely(dst == NULL||src == NULL))
		return -1;
	for(i = 0; i < map_len; i++)
		dst[i] = src[i];
	return 0;
}

#endif
