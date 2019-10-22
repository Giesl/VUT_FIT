#include <stdio.h>
#include <assert.h>
#include "mmal.h"
#include <unistd.h>

void debug_hdr(Header *h, int idx)
{
    printf("+- Header %d @ %p, data @ %p\n", idx, h, &h[1]);
    printf("|    | next           | size     | asize    |\n");
    printf("|    | %-14p | %-8lu | %-8lu |\n", h->next, h->size, h->asize);
}

void debug_arena(Arena *a, int idx)
{
    printf("Arena %d @ %p\n", idx, a);
    printf("|\n");
    char *arena_stop = (char*)a + a->size;
    Header *h = (Header*)&a[1];
    int i = 1;

    while ((char*)h >= (char*)a && (char*)h < arena_stop)
    {
        debug_hdr(h, i);
        i++;
        h = h->next;
        if (h == (Header*)&a[1])
            break;
    }
}

#ifdef NDEBUG
void debug_arenas()
{
    printf("==========================================\n");
    Arena *a = first_arena;
    for (int i = 1; a; i++)
    {
        debug_arena(a, i);
    }
    printf("NULL\n");
}
#else
#define debug_arenas()
#endif

int main()
{
    Header h = {(void*)&main, 1234124, 131072};
    debug_hdr(&h, 1);
    assert(first_arena == NULL);

    /***********************************************************************/
    // Prvni alokace
    // Mela by alokovat novou arenu, pripravit hlavicku v ni a prave jeden
    // blok.
    void *p1 = mmalloc(42);
    printf("TOTAL-ARENA-SIZE:%d\n",first_arena->size);
    /**
     *   v----- first_arena
     *   +-----+------+----+------+----------------------------+
     *   |Arena|Header|XXXX|Header|............................|
     *   +-----+------+----+------+----------------------------+
     *       p1-------^
     */
    assert(first_arena != NULL);
    assert(first_arena->next == NULL);
    assert(first_arena->size > 0);
    assert(first_arena->size <= PAGE_SIZE);
    Header *h1 = (Header*)(&first_arena[1]);
    Header *h2 = h1->next;
    assert(h1->asize == 42);
    assert((char*)h2 > (char*)h1);
    assert(h2->next == h1);
    assert(h2->asize == 0);
    
    debug_arenas();

    /***********************************************************************/
    // Druha alokace
    char *p2 = mmalloc(42);
    /**
     *   v----- first_arena
     *   +-----+------+----+------+----+------+----------------+
     *   |Arena|Header|XXXX|Header|XXXX|Header|................|
     *   +-----+------+----+------+----+------+----------------+
     *       p1-------^           ^
     *       p2-------------------/
     */
    Header *h3 = h2->next;
    assert(h3 != h1);
    assert(h2 != h3);
    assert(h3->next == h1);
    
    //printf("----HEADERS---\n");
    //printf("h1:%d\n",h1);
    //printf("h2:%d\n",h2);
    //printf("h3:%d\n",h3);
    //printf("----POINTERS----\n");
    //printf("p1:%d\n",p1);
    //printf("p2:%d\n",p2);
    
    assert((char*)h2 < p2);
    assert(p2 < (char*)h3);


    debug_arenas();
    
    /***********************************************************************/
    // Treti alokace
    void *p3 = mmalloc(16);

    /**
     *                p1          p2          p3
     *   +-----+------+----+------+----+------+-----+------+---+
     *   |Arena|Header|XXXX|Header|XXXX|Header|XXXXX|Header|...|
     *   +-----+------+----+------+----+------+-----+------+---+
     */
    // insert assert here
    Header *h4 = h3->next;
    
    

    assert(h3 != h1);
    assert(h2 != h3);
    assert(h4->next == h1);
    
    
    
    assert((char*)h2 < p2);
    assert(p2 < (char*)h3);
    assert((char*)h3 < p3);

    debug_arenas();


    /***********************************************************************/
    // Uvolneni prvniho bloku

    mfree(p1);

    //BREAK --TODO--
    
    /**
     *                p1          p2          p3
     *   +-----+------+----+------+----+------+-----+------+---+
     *   |Arena|Header|....|Header|XXXX|Header|XXXXX|Header|...|
     *   +-----+------+----+------+----+------+-----+------+---+
     */
    // insert assert here
    debug_arenas();

    Header *first = &first_arena[1];
    Header *next = first->next;
    //printf("\n------HEADER(0)------\n");
    //printf("h0          :%d\n",first);
    //printf("h0->size    :%d\n",first->size);
    //printf("h0->ASIZE   :%d\n",first->asize);
    //for(int i = 1;next != first;i++)
    //{
    //    printf("\n------HEADER(%d)------\n",i);
    //    printf("h%d          :%d\n",i,next);
    //    printf("h%d->size    :%d\n",i,next->size);
    //    printf("h%d->ASIZE   :%d\n",i,next->asize);
    //    //printf("h%d->ptr    :%d\n",i,&next);
    //    next = next->next;
    //}
    /***********************************************************************/
    // Uvolneni posledniho zabraneho bloku
    mfree(p3);
    /**
     *                p1          p2          p3
     *   +-----+------+----+------+----+------+----------------+
     *   |Arena|Header|....|Header|XXXX|Header|................|
     *   +-----+------+----+------+----+------+----------------+
     */
    // insert assert here
    debug_arenas();
    
    //first = &first_arena[1];
    //next = first->next;
    //printf("\n------HEADER(0)------\n");
    //printf("h0          :%d\n",first);
    //printf("h0->size    :%d\n",first->size);
    //printf("h0->ASIZE   :%d\n",first->asize);
    //for(int i = 1;next != first;i++)
    //{
    //    printf("\n------HEADER(%d)------\n",i);
    //    printf("h%d          :%d\n",i,next);
    //    printf("h%d->size    :%d\n",i,next->size);
    //    printf("h%d->ASIZE   :%d\n",i,next->asize);
    //    //printf("h%d->ptr    :%d\n",i,&next);
    //    next = next->next;
    //}

    /***********************************************************************/
    // Uvolneni prostredniho bloku
    mfree(p2);
    /**
     *                p1          p2          p3
     *   +-----+------+----------------------------------------+
     *   |Arena|Header|........................................|
     *   +-----+------+----------------------------------------+
     */
    // insert assert here
    debug_arenas();

    //first = &first_arena[1];
    //next = first->next;
    //printf("\n------HEADER(0)------\n");
    //printf("h0          :%d\n",first);
    //printf("h0->size    :%d\n",first->size);
    //printf("h0->ASIZE   :%d\n",first->asize);
    //for(int i = 1;next != first;i++)
    //{
    //    printf("\n------HEADER(%d)------\n",i);
    //    printf("h%d          :%d\n",i,next);
    //    printf("h%d->size    :%d\n",i,next->size);
    //    printf("h%d->ASIZE   :%d\n",i,next->asize);
    //    //printf("h%d->ptr    :%d\n",i,&next);
    //    next = next->next;
    //}
    
    // Dalsi alokace se nevleze do existujici areny
    printf("p4\n");
    void *p4 = mmalloc(PAGE_SIZE*2);
    /**
     *   /-- first_arena
     *   v            p1          p2          p3
     *   +-----+------+----------------------------------------+
     *   |Arena|Header|........................................|
     *   +-----+------+----------------------------------------+
     *      \ next
     *       v            p4
     *       +-----+------+---------------------------+------+-----+
     *       |Arena|Header|XXXXXXXXXXXXXXXXXXXXXXXXXXX|Header|.....|
     *       +-----+------+---------------------------+------+-----+
     */

    Arena *nextArena = first_arena;
    printf("NEXT ARENA: %d\n",nextArena);
    for(int j; nextArena != NULL;j++)
    {

        printf("\n-----------ARENA(%d)----------\n",j);
        first = &nextArena[1];
        next = first->next;
        
        printf("\n------HEADER(0)------\n");
        printf("h0          :%d\n",first);
        printf("h0->size    :%d\n",first->size);
        printf("h0->ASIZE   :%d\n",first->asize);

        for(int i = 1;next != first;i++)
        {
            printf("\n------HEADER(%d)------\n",i);
            printf("h%d          :%d\n",i,next);
            printf("h%d->size    :%d\n",i,next->size);
            printf("h%d->ASIZE   :%d\n",i,next->asize);
            //printf("h%d->ptr    :%d\n",i,&next);
            next = next->next;
            
        }
        nextArena = nextArena->next;
        
    }
    /***********************************************************************/
    printf("REALOCCCCC\n");
    p4 = mrealloc(p4, PAGE_SIZE*2 + 2);
    /**
     *                    p4
     *       +-----+------+-----------------------------+------+---+
     *       |Arena|Header|XXXXXXXXXXXXXXXXXXXXXXXXXXXxx|Header|...|
     *       +-----+------+-----------------------------+------+---+
     */
    // insert assert here
    debug_arenas();
    
    /***********************************************************************/
    mfree(p4);


    return 0;
}
