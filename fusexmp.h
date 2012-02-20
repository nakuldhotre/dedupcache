#define MAX_MD5_COUNT 100000
#define MAX_BINODE_COUNT 100000
#define SUCCESS 0
#define FAILURE 1
typedef struct 
{
	unsigned char key[20];
	int index;
        UT_hash_handle hh;
}HASH_TABLE_INODE_N_BLOCK;


typedef struct 
{

	uint32_t hash_key;
	int index;
        UT_hash_handle hh;
}HASH_TABLE_MD5_CACHE;


typedef struct 
{
        int index;
        unsigned char data_block[4096];
        UT_hash_handle hh;
}HASH_TABLE_MEMORY_CACHE;

HASH_TABLE_INODE_N_BLOCK *block_inode=NULL;
HASH_TABLE_MD5_CACHE *md5=NULL;
HASH_TABLE_MEMORY_CACHE *memory=NULL;

static int count_block_inode=0,count_md5_cache=0;
