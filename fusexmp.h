
struct entry
{
	unsigned char key[20];
        uint32_t hash_key;
	int index;
};

struct blistnode
{
	struct entry *curr;
	struct entry *next;
};


struct cache_entry
{
	unsigned char data_block[4096];
	struct entry *hash2;
	struct blistnode *block_list;
	int ref_count;
};

