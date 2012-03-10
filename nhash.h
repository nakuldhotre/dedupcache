#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct
{
  uint64_t key;
  uint64_t val;
  uint64_t *pt;
}n_key_val;

typedef struct nhash_bucket_entry
{
  int used_entries;
  int max_entries;
  n_key_val *key_val;
}bucket_entry;


typedef struct nhash_table_hash
{
  uint32_t table_size;
  uint32_t array_mask;
  bucket_entry *bucket;
}nhash_table;

int nhash_init_table(nhash_table *table, uint32_t size);
int nhash_insert_key(nhash_table *table, const n_key_val *pt);
int nhash_delete_key(nhash_table *table, uint64_t key);
n_key_val * nhash_search_key(nhash_table *table, uint64_t key);
