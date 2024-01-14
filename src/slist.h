#pragma once

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef struct slist_head SListHead;
typedef struct slist_info SListInfo;

struct slist_head {
	SListHead *next;
};

struct slist_info {
	SListHead *head, **tail_ref;
};

static inline
void _slist_push(SListHead **head_ref, SListHead *new_head)
{
	new_head->next = *head_ref;
	*head_ref = new_head;
}

static inline
void slist_init(SListInfo *info)
{
	info->head = NULL;
	info->tail_ref = &info->head;
}

static inline
void slist_append(SListInfo *info, SListHead *new_tail)
{
	_slist_push(info->tail_ref, new_tail);
	info->tail_ref = &((*info->tail_ref)->next);
}