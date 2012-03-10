#include "nhash.h"

int main(int argc, char *argv[])
{

  int count_found=0, count_notfound=0;
  int inserted_keys = 0;
  int i=0,j=0; 

  n_key_val kv1;
  nhash_table *table;
  n_key_val *kv = NULL;

  table = malloc(sizeof(nhash_table));

  nhash_init_table(table, 10000);

  for (i=0;i<100000;i=i+3)
  {
    j = i * 2;
    inserted_keys++;
    kv1.key = i;
    kv1.val = j;
    kv1.pt = i+j;
    nhash_insert_key(table,&kv1);
  }

  for (j=0;j<100;j++)
  for (i=0;i<100000;i++)
  {
    kv = nhash_search_key(table,i);
    if (kv != NULL)
    {
      count_found++;
    }
    else
      count_notfound++;
  }
  printf("We found %d and didn't found %d keys \n", count_found, count_notfound);
  printf("Total Keys inserted = %d\n", inserted_keys);
  return 0;
}
