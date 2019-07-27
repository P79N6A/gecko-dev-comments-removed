





















































































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _MSC_VER
#pragma warning (disable: 4018)
#endif

#include "sphinxbase/hash_table.h"
#include "sphinxbase/err.h"
#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/case.h"


#if 0
static void
prime_sieve(int32 max)
{
    char *notprime;
    int32 p, pp;

    notprime = (char *) ckd_calloc(max + 1, 1);
    p = 2;
    for (;;) {
        printf("%d\n", p);
        for (pp = p + p; pp <= max; pp += p)
            notprime[pp] = 1;
        for (++p; (p <= max) && notprime[p]; p++);
        if (p > max)
            break;
    }
}
#endif







const int32 prime[] = {
    101, 211, 307, 401, 503, 601, 701, 809, 907,
    1009, 1201, 1601, 2003, 2411, 3001, 4001, 5003, 6007, 7001, 8009,
    9001,
    10007, 12007, 16001, 20011, 24001, 30011, 40009, 50021, 60013,
    70001, 80021, 90001,
    100003, 120011, 160001, 200003, 240007, 300007, 400009, 500009,
    600011, 700001, 800011, 900001,
    -1
};





static int32
prime_size(int32 size)
{
    int32 i;

    for (i = 0; (prime[i] > 0) && (prime[i] < size); i++);
    if (prime[i] <= 0) {
        E_WARN("Very large hash table requested (%d entries)\n", size);
        --i;
    }
    return (prime[i]);
}


hash_table_t *
hash_table_new(int32 size, int32 casearg)
{
    hash_table_t *h;

    h = (hash_table_t *) ckd_calloc(1, sizeof(hash_table_t));
    h->size = prime_size(size + (size >> 1));
    h->nocase = (casearg == HASH_CASE_NO);
    h->table = (hash_entry_t *) ckd_calloc(h->size, sizeof(hash_entry_t));
    

    return h;
}






static uint32
key2hash(hash_table_t * h, const char *key)
{

    register const char *cp;

    








    
    register unsigned char c;
    register int32 s;
    register uint32 hash;

    hash = 0;
    s = 0;

    if (h->nocase) {
        for (cp = key; *cp; cp++) {
            c = *cp;
            c = UPPER_CASE(c);
            hash += c << s;
            s += 5;
            if (s >= 25)
                s -= 24;
        }
    }
    else {
        for (cp = key; *cp; cp++) {
            hash += (*cp) << s;
            s += 5;
            if (s >= 25)
                s -= 24;
        }
    }

    return (hash % h->size);
}


static char *
makekey(uint8 * data, size_t len, char *key)
{
    size_t i, j;

    if (!key)
        key = (char *) ckd_calloc(len * 2 + 1, sizeof(char));

    for (i = 0, j = 0; i < len; i++, j += 2) {
        key[j] = 'A' + (data[i] & 0x000f);
        key[j + 1] = 'J' + ((data[i] >> 4) & 0x000f);
    }
    key[j] = '\0';

    return key;
}


static int32
keycmp_nocase(hash_entry_t * entry, const char *key)
{
    char c1, c2;
    int32 i;
    const char *str;

    str = entry->key;
    for (i = 0; i < entry->len; i++) {
        c1 = *(str++);
        c1 = UPPER_CASE(c1);
        c2 = *(key++);
        c2 = UPPER_CASE(c2);
        if (c1 != c2)
            return (c1 - c2);
    }

    return 0;
}


static int32
keycmp_case(hash_entry_t * entry, const char *key)
{
    char c1, c2;
    int32 i;
    const char *str;

    str = entry->key;
    for (i = 0; i < entry->len; i++) {
        c1 = *(str++);
        c2 = *(key++);
        if (c1 != c2)
            return (c1 - c2);
    }

    return 0;
}






static hash_entry_t *
lookup(hash_table_t * h, uint32 hash, const char *key, size_t len)
{
    hash_entry_t *entry;

    entry = &(h->table[hash]);
    if (entry->key == NULL)
        return NULL;

    if (h->nocase) {
        while (entry && ((entry->len != len)
                         || (keycmp_nocase(entry, key) != 0)))
            entry = entry->next;
    }
    else {
        while (entry && ((entry->len != len)
                         || (keycmp_case(entry, key) != 0)))
            entry = entry->next;
    }

    return entry;
}


int32
hash_table_lookup(hash_table_t * h, const char *key, void ** val)
{
    hash_entry_t *entry;
    uint32 hash;
    size_t len;

    hash = key2hash(h, key);
    len = strlen(key);

    entry = lookup(h, hash, key, len);
    if (entry) {
        if (val)
            *val = entry->val;
        return 0;
    }
    else
        return -1;
}

int32
hash_table_lookup_int32(hash_table_t * h, const char *key, int32 *val)
{
    void *vval;
    int32 rv;

    rv = hash_table_lookup(h, key, &vval);
    if (rv != 0)
        return rv;
    if (val)
        *val = (int32)(long)vval;
    return 0;
}


int32
hash_table_lookup_bkey(hash_table_t * h, const char *key, size_t len, void ** val)
{
    hash_entry_t *entry;
    uint32 hash;
    char *str;

    str = makekey((uint8 *) key, len, NULL);
    hash = key2hash(h, str);
    ckd_free(str);

    entry = lookup(h, hash, key, len);
    if (entry) {
        if (val)
            *val = entry->val;
        return 0;
    }
    else
        return -1;
}

int32
hash_table_lookup_bkey_int32(hash_table_t * h, const char *key, size_t len, int32 *val)
{
    void *vval;
    int32 rv;

    rv = hash_table_lookup_bkey(h, key, len, &vval);
    if (rv != 0)
        return rv;
    if (val)
        *val = (int32)(long)vval;
    return 0;
}


static void *
enter(hash_table_t * h, uint32 hash, const char *key, size_t len, void *val, int32 replace)
{
    hash_entry_t *cur, *new;

    if ((cur = lookup(h, hash, key, len)) != NULL) {
        void *oldval;
        
        oldval = cur->val;
        if (replace) {
            


            cur->key = key;
            cur->val = val;
        }
        return oldval;
    }

    cur = &(h->table[hash]);
    if (cur->key == NULL) {
        
        cur->key = key;
        cur->len = len;
        cur->val = val;

        
        cur->next = NULL;

    }
    else {
        
        new = (hash_entry_t *) ckd_calloc(1, sizeof(hash_entry_t));
        new->key = key;
        new->len = len;
        new->val = val;
        new->next = cur->next;
        cur->next = new;
    }
    ++h->inuse;

    return val;
}


static void *
delete(hash_table_t * h, uint32 hash, const char *key, size_t len)
{
    hash_entry_t *entry, *prev;
    void *val;

    prev = NULL;
    entry = &(h->table[hash]);
    if (entry->key == NULL)
        return NULL;

    if (h->nocase) {
        while (entry && ((entry->len != len)
                         || (keycmp_nocase(entry, key) != 0))) {
            prev = entry;
            entry = entry->next;
        }
    }
    else {
        while (entry && ((entry->len != len)
                         || (keycmp_case(entry, key) != 0))) {
            prev = entry;
            entry = entry->next;
        }
    }

    if (entry == NULL)
        return NULL;

    


    val = entry->val;

    if (prev == NULL) {
        
        
        prev = entry;
        if (entry->next) {      
            entry = entry->next;
            prev->key = entry->key;
            prev->len = entry->len;
            prev->val = entry->val;
            prev->next = entry->next;
            ckd_free(entry);
        }
        else {                  
            prev->key = NULL;
            prev->len = 0;
            prev->next = NULL;
        }

    }
    else {                      
        prev->next = entry->next;
        ckd_free(entry);
    }

    

    --h->inuse;

    return val;
}

void
hash_table_empty(hash_table_t *h)
{
    hash_entry_t *e, *e2;
    int32 i;

    for (i = 0; i < h->size; i++) {
        
        for (e = h->table[i].next; e; e = e2) {
            e2 = e->next;
            ckd_free((void *) e);
        }
        memset(&h->table[i], 0, sizeof(h->table[i]));
    }
    h->inuse = 0;
}


void *
hash_table_enter(hash_table_t * h, const char *key, void *val)
{
    uint32 hash;
    size_t len;

    hash = key2hash(h, key);
    len = strlen(key);
    return (enter(h, hash, key, len, val, 0));
}

void *
hash_table_replace(hash_table_t * h, const char *key, void *val)
{
    uint32 hash;
    size_t len;

    hash = key2hash(h, key);
    len = strlen(key);
    return (enter(h, hash, key, len, val, 1));
}

void *
hash_table_delete(hash_table_t * h, const char *key)
{
    uint32 hash;
    size_t len;

    hash = key2hash(h, key);
    len = strlen(key);

    return (delete(h, hash, key, len));
}

void *
hash_table_enter_bkey(hash_table_t * h, const char *key, size_t len, void *val)
{
    uint32 hash;
    char *str;

    str = makekey((uint8 *) key, len, NULL);
    hash = key2hash(h, str);
    ckd_free(str);

    return (enter(h, hash, key, len, val, 0));
}

void *
hash_table_replace_bkey(hash_table_t * h, const char *key, size_t len, void *val)
{
    uint32 hash;
    char *str;

    str = makekey((uint8 *) key, len, NULL);
    hash = key2hash(h, str);
    ckd_free(str);

    return (enter(h, hash, key, len, val, 1));
}

void *
hash_table_delete_bkey(hash_table_t * h, const char *key, size_t len)
{
    uint32 hash;
    char *str;

    str = makekey((uint8 *) key, len, NULL);
    hash = key2hash(h, str);
    ckd_free(str);

    return (delete(h, hash, key, len));
}

void
hash_table_display(hash_table_t * h, int32 showdisplay)
{
    hash_entry_t *e;
    int i, j;
    j = 0;

    printf("Hash with chaining representation of the hash table\n");

    for (i = 0; i < h->size; i++) {
        e = &(h->table[i]);
        if (e->key != NULL) {
            printf("|key:");
            if (showdisplay)
                printf("%s", e->key);
            else
                printf("%p", e->key);

            printf("|len:%zd|val=%ld|->", e->len, (long)e->val);
            if (e->next == NULL) {
                printf("NULL\n");
            }
            j++;

            for (e = e->next; e; e = e->next) {
                printf("|key:");
                if (showdisplay)
                    printf("%s", e->key);

                printf("|len:%zd|val=%ld|->", e->len, (long)e->val);
                if (e->next == NULL) {
                    printf("NULL\n");
                }
                j++;
            }
        }
    }

    printf("The total number of keys =%d\n", j);
}


glist_t
hash_table_tolist(hash_table_t * h, int32 * count)
{
    glist_t g;
    hash_entry_t *e;
    int32 i, j;

    g = NULL;

    j = 0;
    for (i = 0; i < h->size; i++) {
        e = &(h->table[i]);

        if (e->key != NULL) {
            g = glist_add_ptr(g, (void *) e);
            j++;

            for (e = e->next; e; e = e->next) {
                g = glist_add_ptr(g, (void *) e);
                j++;
            }
        }
    }

    if (count)
        *count = j;

    return g;
}

hash_iter_t *
hash_table_iter(hash_table_t *h)
{
	hash_iter_t *itor;

	itor = ckd_calloc(1, sizeof(*itor));
	itor->ht = h;
	return hash_table_iter_next(itor);
}

hash_iter_t *
hash_table_iter_next(hash_iter_t *itor)
{
	
	if (itor->ent)
		itor->ent = itor->ent->next;
	

	if (itor->ent == NULL) {
		while (itor->idx < itor->ht->size
		       && itor->ht->table[itor->idx].key == NULL) 
			++itor->idx;
		

		if (itor->idx == itor->ht->size) {
			hash_table_iter_free(itor);
			return NULL;
		}
		
		itor->ent = itor->ht->table + itor->idx;
		
		++itor->idx;
	}
	return itor;
}

void
hash_table_iter_free(hash_iter_t *itor)
{
	ckd_free(itor);
}

void
hash_table_free(hash_table_t * h)
{
    hash_entry_t *e, *e2;
    int32 i;

    if (h == NULL)
        return;

    
    for (i = 0; i < h->size; i++) {
        for (e = h->table[i].next; e; e = e2) {
            e2 = e->next;
            ckd_free((void *) e);
        }
    }

    ckd_free((void *) h->table);
    ckd_free((void *) h);
}
