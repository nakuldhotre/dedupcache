#define MAX_MEMORY_COUNT 100000
#define MAX_BINODE_COUNT 5000000
#define SUCCESS 0
#define FAILURE 1

typedef struct 
{
	uint64_t inode_block;
	unsigned char hash_key[16];
}HASH_TABLE_INODE_N_BLOCK;


/*
typedef struct 
{

	hash_key;
	int index;
        UT_hash_handle hh;
}HASH_TABLE_MD5_CACHE;
*/
typedef struct 
{
        unsigned char hash_key[16];
        unsigned char *data_block;
}HASH_TABLE_MEMORY_CACHE;

HASH_TABLE_INODE_N_BLOCK *block_inode=NULL;
// HASH_TABLE_MD5_CACHE *md5=NULL;
HASH_TABLE_MEMORY_CACHE *memory=NULL;

static int count_block_inode=0,count_memory_cache=0,case1=0,case2=0,case3=0,case4=0;

static int case5=0,case6=0;
static int binode_cache_miss, memory_cache_miss;
static int binode_cache_total, memory_cache_total;
static int binode_eviction_done=0,memory_eviction_done=0;

//char cases[3][10]={"Ranjan","nakul","india"};

typedef struct
{
	char name[70];
	int count;
}STATS;

#define FUSEXMP_DATA fuse_get_context()->private_data
