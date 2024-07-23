#if !defined(EB_STR_LIST_H)
#define EB_STR_LIST_H

#include "core.h"
#include "str.h"
#include "allocator.h"

#if defined(Lang_CPP)
C_Linkage_Start
#endif

typedef struct StrNode StrNode;
typedef struct StrList StrList;

// ern: String List
struct StrNode 
{
  Str     value;
  u64     hash;
  StrNode *next;
  StrNode *hash_next;
};

struct StrList 
{
  StrNode *first;
  StrNode *last;
  StrNode **buckets;
  u32     bucket_count;
  u32     count;
};

#define STR_LIST_FOR_EACH(list, node_name) for(StrNode *(node_name)=(list)->first;\
                                          (node_name);\
                                          (node_name) = (node_name)->next)

StrNode* str_list_contains(StrList *list, Str str);
void str_list_add(StrList *list, StrNode *node);
StrList str_list_create(ArenaAllocator *arena, char **cstr_list, int list_size);
StrList str_list_init_empty(ArenaAllocator *arena);
void str_list_add_from_cstr(ArenaAllocator *arena, StrList *list, char *cstr);
void str_list_add_from_str(ArenaAllocator *arena, StrList *list, Str str);

#if defined(Lang_CPP)
C_Linkage_End
#endif

#endif // EB_STR_LIST_H

#if defined(EB_STR_LIST_IMPLEMENTATION)
#if defined(Lang_CPP)
C_Linkage_Start
#endif

priv_func u64 
str_node_hash(Str s)
{
  u64 result = 5381;
  for(u64 i = 0; i < STR_SIZE(s); i += 1)
  {
    result = ((result << 5) + result) + STR_DATA(s)[i];
  }
  return result;
}
StrNode*
slot_find_str_node(StrNode **slot, Str s)
{
  StrNode *result = 0;
  for(StrNode *curr = *slot; curr; curr = curr->hash_next) {
    (void*)0;
    if(str_is_match(curr->value, s)) {
      result = curr;
      break;
    }
  }
  return result;
}
StrNode **
str_list_get_slot(StrList *list, Str str)
{
  StrNode **slot = 0;
  if(list->bucket_count > 0) {
    u64 hash    = str_node_hash(str);
    u64 bucket  = hash % list->bucket_count;
    slot        = &list->buckets[bucket];
  }
  return slot;
}
void
str_list_add(StrList *list, StrNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count++;
}
StrNode *
str_list_contains(StrList *list, Str str)
{
  return slot_find_str_node(str_list_get_slot(list, str), str);
}
StrList 
str_list_create(ArenaAllocator *arena, char **cstr_list, int list_size)
{
StrList result = {0};
  if(cstr_list && list_size > 0) {
    result = str_list_init_empty(arena); 
  
    for(int i = 0; i < list_size; i++) {
        Str s = cstr(cstr_list[i]);
        StrNode **slot = str_list_get_slot(&result, s);
        StrNode *existing_elem = slot_find_str_node(slot, s);
        if(!existing_elem) {
            StrNode *var = push_arena_array(arena, StrNode, 1);
            var->hash_next = *slot;
            var->hash      = str_node_hash(s);
            var->value     = s;
            *slot          = var;
            str_list_add(&result, var); 
        }
    }
  }
  return result; 
}
StrList
str_list_init_empty(ArenaAllocator *arena)
{
StrList result = {0};
  result.bucket_count = 4096;
  result.buckets = (StrNode**)push_arena_array_zero(arena, StrNode *,
                                               result.bucket_count);
  return result;
}
void
str_list_add_from_str(ArenaAllocator *arena, StrList *list, Str str)
{
  StrNode **slot = str_list_get_slot(list, str);
  StrNode *existing_elem = slot_find_str_node(slot, str);
  if(!existing_elem) {
      StrNode *var = push_arena_array(arena, StrNode, 1);
      if(var) {
        var->hash_next = *slot;
        var->hash      = str_node_hash(str);
        var->value     = str;
        *slot          = var;
        str_list_add(list, var); 
      }
  }
}

void
str_list_add_from_cstr(ArenaAllocator *arena, StrList *list, char *c_str)
{
  if(c_str) {
        Str s = cstr(c_str);
        str_list_add_from_str(arena, list, s); 
  }
}

#if defined(Lang_CPP)
C_Linkage_End
#endif

#endif // Implementation
