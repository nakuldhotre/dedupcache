
typedef struct 
{
	unsigned char key[20];

	int index;
        UT_hash_handle hh;
}HASH_TABLE_INODE_N_BLOCK;


typedef struct 
{
	int index;	
	uint32_t hash_key;
        unsigned char data_block[4096];
        UT_hash_handle hh;
}HASH_TABLE_MD5_CACHE;

HASH_TABLE_INODE_N_BLOCK *block_inode=NULL;
HASH_TABLE_MD5_CACHE *md5=NULL;
static int count_block_inode=0,count_md5_cache=0;

