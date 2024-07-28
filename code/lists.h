#if !defined(EB_LISTS_H)
#define EB_LISTS_H

#include "core.h"
#include "allocator.h"
#include "util.h"

#if defined(Lang_CPP)
C_Linkage_Start
#endif

#define hash_creation_func_def(name) u64 name(void *data, u64 data_size)
typedef hash_creation_func_def(hash_creation_func);

#define hash_list_cmp_def(name) bool name(const void *pa, const void *pb)
typedef hash_list_cmp_def(hash_list_cmp_func);

#ifndef DefaultHashListBucketCount
#define DefaultHashListBucketCount 4096
#endif

typedef struct ElementNode ElementNode;
typedef struct HashList    HashList;
typedef struct DataNode    DataNode;
static  hash_creation_func *default_hash_func = util_node_hash;
typedef struct HashListInitOption HashListInitOption;

void hash_list_add(ArenaAllocator *arena, HashList *list, Str key, DataNode *data);
void  hash_list_add_struct_arg(ArenaAllocator *arena, HashList *list, Str key, DataNode data);
HashList hash_list_create(ArenaAllocator *arena, HashListInitOption *option);
HashList hash_list_create_default_func(ArenaAllocator *arena);
DataNode* hash_list_contains_cstr(HashList *list, char *cstr_var);
DataNode* hash_list_contains(HashList *list, Str key);
DataNode* hash_list_get_first_element(HashList *list);
DataNode* hash_list_get_last_element(HashList *list);

struct HashListInitOption
{
  hash_creation_func *hash_function;
};

struct DataNode 
{
    void *data;
    u64  data_size;
};
struct ElementNode
{
    Str         key;
    DataNode    data;
    u64         hash;
    ElementNode *hash_next;
    ElementNode *next;
};
struct HashList
{
  ElementNode       *first;
  ElementNode       *end;
  ElementNode       **buckets;
  u64                 bucket_count;
  u64                 count;
  HashListInitOption *option; 
};

#define DataNodeNew(d,d_sz) &(DataNode){.data=(d),.data_size=(d_sz)}
#define DataNodeFromStr(str) DataNodeNew(STR_DATA(str),STR_SIZE(str))
#define StrFromDataNodeStruct(d)  cstr_sized((d).data, (d).data_size)
#define StrFromDataNodePtr(d) (d) ? cstr_sized((d)->data,(d)->data_size):STR_INVALID
#define GetDataNode(e) (e) ? &(e)->data : 0
#define GetKey(e) (e) ? (e)->key : STR_INVALID

#define StrFromDataNode(d) _Generic((d),\
DataNode:   StrFromDataNodeStruct,\
DataNode*:  StrFromDataNodePtr\
)(d)

#define hash_list_new(a,...) hash_list_create(a, &(HashListInitOption){__VA_ARGS__}) 

#define HashListLinearLoop(l,el_name) for(ElementNode *el_name = (l)->first; el_name; (el_name) = (el_name)->next)

#define hash_list_get_data(l,k) _Generic((k),\
const char*: hash_list_contains_cstr,\
char*      : hash_list_contains_cstr,\
Str        : hash_list_contains\
)(l,k)

#define hash_list_add_data(a,l,k,d) _Generic((d),\
DataNode: hash_list_add_struct_arg,\
DataNode*: hash_list_add\
)(a,l,k,d)

#define hash_list_push(a,l,k,d,d_sz) hash_list_add(a,l,k, &(DataNode){.data=(d),.data_size=(d_sz)})

#if defined(Lang_CPP)
C_Linkage_End
#endif
#endif // EB_LISTS_H

#ifdef EB_LISTS_IMPLEMENTATION
#if defined(Lang_CPP)
C_Linkage_Start
#endif
void 
hash_list_add_struct_arg(ArenaAllocator *arena, HashList *list, Str key, DataNode data)
{
    return hash_list_add(arena, list, key, &data);
}

priv_func
ElementNode **
hash_list_get_slot(HashList *list, Str search)
{
ElementNode **slot = 0;
    if(list->bucket_count > 0) {
      u64 hash    = list->option->hash_function(STR_DATA(search), STR_SIZE(search));
      u64 bucket  = hash % list->bucket_count;
      slot        = &list->buckets[bucket];
    }
    return slot;
}

HashList 
hash_list_create_default_func(ArenaAllocator *arena)
{
  return hash_list_create(arena, 0);
}

HashList 
hash_list_create(ArenaAllocator *arena, HashListInitOption *option)
{
HashList list = {0};

  list.bucket_count = DefaultHashListBucketCount;
  list.buckets      = push_arena_array_zero(arena, ElementNode *, list.bucket_count);
  if(!option) { 
    HashListInitOption *opt = push_arena_array(arena, HashListInitOption, 1);
    opt->hash_function = default_hash_func;
    list.option = opt;
  } else if(!option->hash_function) {
    option->hash_function = default_hash_func;
    list.option = option;
  } else {
    list.option = option;
  }
  return list;
}

priv_func
void
hash_list_add_priv(HashList *list, ElementNode *node)
{
  SLLQueuePush(list->first, list->end, node);
  list->count++;
}

priv_func
ElementNode*
hash_list_find_node_priv(ElementNode **slot, Str key)
{
ElementNode *result = 0;
  for(ElementNode *current = *slot; current; current = current->hash_next) {
      if(str_is_match(current->key, key)) {
          result = current;
          break;
      }
  }
  return result;
}

void 
hash_list_add(ArenaAllocator *arena, HashList *list, Str key, DataNode *data)
{
ElementNode **slot = 0;
ElementNode *existing_node;

  if(list && list->buckets && list->bucket_count > 0) {
      slot          = hash_list_get_slot(list, key); 
      existing_node = hash_list_find_node_priv(slot, key);
      if(!existing_node) {
          ElementNode *var = push_arena_array_zero(arena, ElementNode, 1);
          var->hash_next = *slot;
          var->hash      = list->option->hash_function(STR_DATA(key), STR_SIZE(key));
          var->key       = key;
          MemoryCopy(&var->data, data, sizeof(var->data));
          *slot          = var;
          hash_list_add_priv(list, var);
      }
  }
}

DataNode*
hash_list_get_first_element(HashList *list)
{
ElementNode *node;
DataNode *data = 0;
    node = list->first;
    if(node) {
        data = &node->data;
    }
    return data;
}

DataNode*
hash_list_get_last_element(HashList *list)
{
ElementNode *node;
DataNode *data = 0;
    node = list->end;
    if(node) {
        data = &node->data;
    }
    return data;
}

DataNode*
hash_list_contains_cstr(HashList *list, char *cstr_var)
{
  return hash_list_contains(list, cstr(cstr_var));
}

DataNode*
hash_list_contains(HashList *list, Str key)
{
ElementNode *node;
DataNode    *data = 0;
  node = hash_list_find_node_priv(hash_list_get_slot(list, key), key);
  if(node) {
      data = &node->data;
  }
  return data;
}

#if defined(Lang_CPP)
C_Linkage_End
#endif
#endif //EB_LISTS_IMPLEMENTATION
