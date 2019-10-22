/**
 * Implementace My MALloc
 * Demonstracni priklad pro 1. ukol IPS/2019
 * Ales Smrcka
 */

#include "mmal.h"
#include <sys/mman.h> // mmap
#include <stdbool.h> // bool

#ifdef NDEBUG
/**
 * The structure header encapsulates data of a single memory block.
 */
typedef struct header Header;
struct header {

    /**
     * Pointer to the next header. Cyclic list. If there is no other block,
     * points to itself.
     */
    Header *next;

    /// size of the block
    size_t size;

    /**
     * Size of block in bytes allocated for program. asize=0 means the block 
     * is not used by a program.
     */
    size_t asize;
};

typedef enum { MMAP_ERROR,
               FIRST_ARENA_ERROR,
               OTHER_ERROR
             } error_codes; 
/**
 * The arena structure.
 */
typedef struct arena Arena;
struct arena {

    /**
     * Pointer to the next arena. Single-linked list.
     */
    Arena *next;

    /// Arena size.
    size_t size;
};

#define PAGE_SIZE 128*1024

#endif


//define global variables
Arena *first_arena = NULL;


static
Header *find_space(size_t size)
{
	if ( first_arena == NULL )
	{
		exit(3);
	}

}


/**
 * Return size alligned to PAGE_SIZE
 */
static
size_t allign_page(size_t size)
{
    size_t target_size = PAGE_SIZE;
    while (size > target_size)
    {
        target_size += PAGE_SIZE;
    }

    return target_size;
}

/**
 * Allocate a new arena using mmap.
 * @param req_size requested size in bytes. Should be alligned to PAGE_SIZE.
 * @return pointer to a new arena, if successfull. NULL if error.
 */

/**
 *   +-----+------------------------------------+
 *   |Arena|....................................|
 *   +-----+------------------------------------+
 *
 *   |--------------- Arena.size ---------------|
 */
static
Arena *arena_alloc(size_t req_size)
{
    // FIXME
    size_t size = allign_page(req_size + sizeof(Header));
    Arena * region = (Arena*) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    region->next = NULL;
    region->size = size;
    if (region == MAP_FAILED)
    {
    	return NULL;
    }
    region->size = size;
    return region;
}

/**
 * Header structure constructor (alone, not used block).
 * @param hdr       pointer to block metadata.
 * @param size      size of free block
 */
/**
 *   +-----+------+------------------------+----+
 *   | ... |Header|........................| ...|
 *   +-----+------+------------------------+----+
 *
 *                |-- Header.size ---------|
 */
static
void hdr_ctor(Header *hdr, size_t size)
{
	(void)hdr;
	(void)size;
}

/**
 * Splits one block into two.
 * @param hdr       pointer to header of the big block
 * @param req_size  requested size of data in the (left) block.
 * @pre   (req_size % PAGE_SIZE) = 0
 * @pre   (hdr->size >= req_size + 2*sizeof(Header))
 * @return pointer to the new (right) block header.
 */
/**
 * Before:        |---- hdr->size ---------|
 *
 *    -----+------+------------------------+----
 *         |Header|........................|
 *    -----+------+------------------------+----
 *            \----hdr->next---------------^
 */
/**
 * After:         |- req_size -|
 *
 *    -----+------+------------+------+----+----
 *     ... |Header|............|Header|....|
 *    -----+------+------------+------+----+----
 *             \---next--------^  \--next--^
 */
static
Header *hdr_split(Header *hdr, size_t req_size)
{		
		//printf("-----SPLIT----\n");
		//printf("OLD HEADER	:%d\n",hdr);
		//printf("OLD SIZE	:%d\n",hdr->size);
		//printf("OLD ASIZE	:%d\n",hdr->asize);
		//printf("OLD NEXT	:%d\n\n",hdr->next);
    	
    	//get tmp values of next and size
    	Header *tmp = hdr->next;
    	size_t size = hdr->size;

    	//modify old header
    	hdr->next = (char*) hdr + req_size + sizeof(Header);
    	hdr->size = req_size;

    	//create new header
    	Header *hnew = hdr->next;
    	hnew->size = size - req_size - sizeof(Header);
    	hnew->next = (char*) tmp;
    	
    	//printf("LEFT HEADER	:%d\n",hdr);
		//printf("LEFT SIZE:	%d\n",hdr->size);
		//printf("LEFT NEXT	:%d\n\n",hdr->next);

		//printf("RIGHT HEADER:%d\n",hnew);
		//printf("RIGHT SIZE	:%d\n",hnew->size);
		//printf("RIGHT NEXT	:%d\n",hnew->next);
    	//printf("--------------\n"); 
    return hnew;
}

/**
 * Detect if two blocks adjacent blocks could be merged.
 * @param left      left block
 * @param right     right block
 * @return true if two block are free and adjacent in the same arena.
 */
static
bool hdr_can_merge(Header *left, Header *right)
{
    if (left->asize == 0 && right->asize == 0 && left->next == right) return true;
    return false;
}


/**
 * Merge two adjacent free blocks.
 * @param left      left block
 * @param right     right block
 */
static
void hdr_merge(Header *left, Header *right)
{
    if(hdr_can_merge(left,right))
    {
    	left->next = right->next;
    	left->size = left->size + right->size + sizeof(Header);
    	right = -1;
    }
}

/**
 * Allocate memory. Use best-fit search of available block.
 * @param size      requested size for program
 * @return pointer to allocated data or NULL if error.
 */
void *mmalloc(size_t size)
{
    if (first_arena == NULL)
    {
    	first_arena = arena_alloc(size);
    	//pointer to page header
    	Header *hArea = (Arena*) (first_arena);
    	Header *hPage = (Header*)(&first_arena[1]);

    	hPage->asize = size;
    	hPage->size = size;
    	hPage->next = (char*) hPage + hPage->size + sizeof(Header);
    	Header *hPage2 = hPage->next;
    	hPage2->size = hArea->size - hPage->size - 2*sizeof(Header);
    	hPage2->asize = 0;
    	hPage2->next = (char*) hPage;
    	
    	return (char*) hPage + sizeof(Header); 
    		
    }
    else
    {
    	printf("malloc\n");
    	Arena *arena = (Arena*) (first_arena);
    	Header *first = (Header*) (&arena[1]);
		//--TODO -- best fit
    	size_t	bestSize = PAGE_SIZE*32;
    	Header *best = NULL;
		//find best page
		Header *h = first;
    	while((h->size < size || h->asize != 0) && h->next != first)
    	{
    		h = h->next;
    		printf("h:%d\n",h);
    		printf("f:%d\n",first);
    		if(h == first)
    		{
    			printf("neco\n");
    			//no next arena
    			if(arena->next == NULL)
    			{
    				//Creating new arena
    				printf("-------------CREATING NEW ARENA---------------\n");
    				

    				Arena *tmp = arena_alloc(size);
    				Header *hArea = (Arena*) (tmp);
			    	Header *hPage = (Header*)(&tmp[1]);
					

			    	hPage->asize = size;
			    	hPage->size = size;
			    	hPage->next = (char*) hPage + hPage->size + sizeof(Header);
			    	Header *hPage2 = hPage->next;
			    	hPage2->size = hArea->size - hPage->size - 2*sizeof(Header);
			    	hPage2->asize = 0;
			    	hPage2->next = (char*) hPage;
			    	
			    	arena->next = tmp;
			    	//printf("CREATED\n");
			    	return (char*) hPage + sizeof(Header);
    			}
    			h = (Header*) (&arena[1]);
    		}

    		if(h->size < bestSize)
    			{
    				bestSize = h->size;
    				best = h;
    			}

    	}

    	
    	
    	

   	printf("----BEST FIT SEARCH----\n");
    printf("BEST SIZE:%d\n",bestSize);
    printf("BEST HEAD:%d\n",best);
    hdr_split(h,size);
    h->asize=size;
    return (char*) h + sizeof(Header);
    }

    return NULL;
}

/**
 * Free memory block.
 * @param ptr       pointer to previously allocated data
 */
void mfree(void *ptr)
{

    Header *h = (char*) ptr - sizeof(Header);
    //printf("-----FREE-----\n");
    //printf("*ptr	:%d\n",ptr);
    //printf("HEADER	:%d\n",h);
	//printf("SIZE	:%d\n",h->size);
	//printf("ASIZE	:%d\n",h->asize);
	//printf("NEXT	:%d\n",h->next);
	//printf("-------------\n");

	h->asize = 0;
	Header *next = h->next;

	//merge blocks on right
	while(next->asize == 0 && next->next != h)
	{	
		printf("----merge to right----\n");
		//printf("left	:%d\n",h);
		//printf("right	:%d\n",next);
		Header *tmp = next->next;
		hdr_merge(h,next);
		next = tmp;
	}
	
	//find previous header
	while(next->next != h)
	{
		next = next->next;
	}
	Header *prev = next;
	if (prev->next != h)
	{
		while(prev->asize == 0 && prev != h)
		{
			printf("----merge to left----\n");
			//printf("left	:%d\n",prev);
			//printf("right	:%d\n",h);
			hdr_merge(prev,h);
			h = prev;
			next = prev->next;
			while(next->next != prev)
			{
				next = next->next;
			}
			prev = next;
		}
	}
    // FIXME
}

/**
 * Reallocate previously allocated block.
 * @param ptr       pointer to previously allocated data
 * @param size      a new requested size. Size can be greater, equal, or less
 * then size of previously allocated block.
 * @return pointer to reallocated space.
 */
void *mrealloc(void *ptr, size_t size)
{
	printf("REALLOC\n");
	mfree(ptr);
    return mmalloc(size);
}
