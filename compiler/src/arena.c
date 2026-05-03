#include <stdlib.h>
#include <string.h>

#include "../include/arena.h"
#include "../include/util.h"

#define JZ_ARENA_DEFAULT_BLOCK_SIZE 4096

void jz_arena_init(JZArena *arena, size_t block_size)
{
    if (!arena) {
        return;
    }
    arena->head = NULL;
    arena->block_size = block_size ? block_size : JZ_ARENA_DEFAULT_BLOCK_SIZE;
}

void *jz_arena_alloc(JZArena *arena, size_t size)
{
    if (!arena || size == 0) {
        return NULL;
    }

    /* Ensure alignment to pointer size. */
    if (jz_size_align_up_checked(size, sizeof(void *), &size) != 0) {
        return NULL;
    }

    JZArenaBlock *block = arena->head;
    JZArenaBlock *head = arena->head;
    if (!block) {
        /* handled below */
    } else {
        size_t used_plus_size = 0;
        if (jz_size_add_checked(block->used, size, &used_plus_size) != 0 ||
            used_plus_size > block->capacity) {
            block = NULL;
        }
    }
    if (!block) {
        size_t min_bytes = 0;
        if (jz_size_add_checked(size, sizeof(JZArenaBlock), &min_bytes) != 0) {
            return NULL;
        }
        size_t req = arena->block_size > min_bytes ? arena->block_size : min_bytes;
        JZArenaBlock *new_block = (JZArenaBlock *)malloc(req);
        if (!new_block) {
            return NULL;
        }
        new_block->next = head;
        new_block->used = 0;
        new_block->capacity = req - sizeof(JZArenaBlock);
        arena->head = new_block;
        block = new_block;
    }

    void *ptr = block->data + block->used;
    if (jz_size_add_checked(block->used, size, &block->used) != 0) {
        return NULL;
    }
    return ptr;
}

void jz_arena_reset(JZArena *arena)
{
    if (!arena) {
        return;
    }
    /* Keep only the most recent block (if any) to avoid thrashing. */
    JZArenaBlock *block = arena->head;
    if (!block) {
        return;
    }
    JZArenaBlock *next = block->next;
    while (next) {
        JZArenaBlock *tmp = next->next;
        free(next);
        next = tmp;
    }
    block->next = NULL;
    block->used = 0;
}

void jz_arena_free(JZArena *arena)
{
    if (!arena) {
        return;
    }
    JZArenaBlock *block = arena->head;
    while (block) {
        JZArenaBlock *next = block->next;
        free(block);
        block = next;
    }
    arena->head = NULL;
}
