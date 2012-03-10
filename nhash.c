#include "nhash.h"

int nhash_init_table(nhash_table *table, uint32_t size)
{
  int i;
  uint32_t mask = 0; 
  for (i =1; i< 32; i++)
  {
    mask = ((mask<<1) + 1);
    if ((size >> i)==0)
    {
       break;
    }
  }

  table->array_mask = mask;
  table->table_size = mask+1;  /* We cant have arbitary size tables */
  table->bucket = malloc (sizeof(bucket_entry)*(mask+1));
  if(table->bucket == NULL)
  {
    fprintf(stderr, "nhash_init_table: Malloc failed!\n");
    exit(1);
  }
  memset(table->bucket, 0, sizeof(bucket_entry)*(mask+1));
  return 0;
}


int nhash_insert_key(nhash_table *table, uint64_t key, uint64_t val)
{

  uint32_t arr_index;
//  uint64_t key,val;
  bucket_entry *bucket=NULL;
  n_key_val *temp = NULL;
  int i;

//  key = kv->key;
//  val = kv->val;
  
  arr_index = ((uint32_t)table->array_mask & key);
  bucket = &table->bucket[arr_index];
  if (bucket->max_entries == 0)
  {
    bucket->key_val = malloc(sizeof (n_key_val) * 5);
    bucket->max_entries = 5;
  }
  else if (bucket->max_entries == bucket->used_entries)
  {
    temp = malloc(sizeof(n_key_val) * ( bucket->max_entries+5));
    memcpy(temp,bucket->key_val, sizeof(n_key_val)*(bucket->max_entries));
    free(bucket->key_val);
    bucket->key_val = temp;
    bucket->max_entries +=5;
  }
  i = bucket->used_entries;
  bucket->key_val[i].key = key;
  bucket->key_val[i].val = val;

  bucket->used_entries++;
  return 0;
}

int nhash_delete_key(nhash_table *table, uint64_t key)
{
  int deleted=0;
  uint32_t arr_index;
  bucket_entry *bucket=NULL;
  int i,j;

  arr_index = (table->array_mask & key);
  bucket = &table->bucket[arr_index];
  if (bucket->max_entries == 0)
  {
    return 1; /* Key not found */
  }
  else
  {
    for(i=0;i<bucket->used_entries; i++)
    {
      if (key == bucket->key_val[i].key)
      {
        for (j=i+1;j<bucket->used_entries;j++)
        {
          bucket->key_val[j-1] = bucket->key_val[j];
        }
        bucket->used_entries--;
        deleted = 1;
        break;
      }
    }
  }
  if (deleted)
    return 0;
  else
    return 1;

}

n_key_val * nhash_search_key(nhash_table *table, uint64_t key)
{
  int i;
  uint32_t arr_index;
  bucket_entry *bucket=NULL;

  arr_index = ((uint32_t)table->array_mask & key);
  bucket = &table->bucket[arr_index];
  if (bucket->max_entries == 0)
  {
    return NULL; /* Key not found */
  }
  else
  {
    for(i=0;i<bucket->used_entries; i++)
    {
      if (key == bucket->key_val[i].key)
      {
        return &bucket->key_val[i];
      }
    }
  }
  return NULL;
}
