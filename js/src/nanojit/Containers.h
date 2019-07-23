






































#ifndef __nanojit_Containers__
#define __nanojit_Containers__

namespace nanojit
{
    



    class BitSet {
        Allocator &allocator;
        int cap;
        int64_t *bits;
        static const int64_t ONE = 1;
        static const int SHIFT = 6;

        inline int bitnum2word(int i) {
            return i >> 6;
        }
        inline int64_t bitnum2mask(int i) {
            return ONE << (i & 63);
        }

        
        void grow(int w);

    public:
        BitSet(Allocator& allocator, int nbits=128);

        
        void reset();

        

        bool setFrom(BitSet& other);

        
        bool get(int i) {
            NanoAssert(i >= 0);
            int w = bitnum2word(i);
            if (w < cap)
                return (bits[w] & bitnum2mask(i)) != 0;
            return false;
        }

        
        void set(int i) {
            NanoAssert(i >= 0);
            int w = bitnum2word(i);
            if (w >= cap)
                grow(w);
            bits[w] |= bitnum2mask(i);
        }

        
        void clear(int i) {
            NanoAssert(i >= 0);
            int w = bitnum2word(i);
            if (w < cap)
                bits[w] &= ~bitnum2mask(i);
        }
    };

    
    template<class T> class Seq {
    public:
        Seq(T head, Seq<T>* tail=NULL) : head(head), tail(tail) {}
        T       head;
        Seq<T>* tail;
    };

    



    template<class T> class SeqBuilder {
    public:
        SeqBuilder(Allocator& allocator)
            : allocator(allocator)
            , items(NULL)
            , last(NULL)
        { }

        
        void insert(T item) {
            Seq<T>* e = new (allocator) Seq<T>(item, items);
            if (last == NULL)
                last = e;
            items = e;
        }

        
        void add(T item) {
            Seq<T>* e = new (allocator) Seq<T>(item);
            if (last == NULL)
                items = e;
            else
                last->tail = e;
            last = e;
        }

        
        Seq<T>* get() const {
            return items;
        }

        
        bool isEmpty() const {
            return items == NULL;
        }

        
        void clear() {
            items = last = NULL;
        }

    private:
        Allocator& allocator;
        Seq<T>* items;
        Seq<T>* last;
    };

    






    template<class K, class T> class HashMap {
        Allocator& allocator;
        uint32_t nbuckets;
        class Node {
        public:
            K key;
            T value;
            Node(K k, T v) : key(k), value(v) { }
        };
        Seq<Node>** buckets;

        
        uint32_t hash(K k) {
            uintptr_t h = (uintptr_t) k;
            h = (h>>3) ^ (h<<29); 
            return uint32_t(h) % nbuckets;
        }

        
        Node* find(K k, uint32_t &i) {
            i = hash(k);
            for (Seq<Node>* p = buckets[i]; p != NULL; p = p->tail) {
                if (p->head.key == k)
                    return &p->head;
            }
            return NULL;
        }
    public:
        HashMap(Allocator& a, uint32_t nbuckets = 16)
            : allocator(a)
            , nbuckets(nbuckets)
            , buckets(new (a) Seq<Node>*[nbuckets])
        {
            clear();
        }

        

        void clear() {
            VMPI_memset(buckets, 0, sizeof(Seq<Node>*) * nbuckets);
        }

        
        void put(K k, T v) {
            uint32_t i;
            Node* n = find(k, i);
            if (n) {
                n->value = v;
                return;
            }
            buckets[i] = new (allocator) Seq<Node>(Node(k,v), buckets[i]);
        }

        
        T get(K k) {
            uint32_t i;
            Node* n = find(k, i);
            return n ? n->value : 0;
        }

        
        bool containsKey(K k) {
            uint32_t i;
            return find(k, i) != 0;
        }

        

        void remove(K k) {
            uint32_t i = hash(k);
            Seq<Node>** prev = &buckets[i];
            for (Seq<Node>* p = buckets[i]; p != NULL; p = p->tail) {
                if (p->head.key == k) {
                    (*prev) = p->tail;
                    return;
                }
                prev = &p->tail;
            }
        }

        











        class Iter {
            friend class HashMap;
            const HashMap<K,T> &map;
            int bucket;
            const Seq<Node>* current;

        public:
            Iter(HashMap<K,T>& map) : map(map), bucket(map.nbuckets-1), current(NULL)
            { }

            
            bool next() {
                if (current)
                    current = current->tail;
                while (bucket >= 0 && !current)
                    current = map.buckets[bucket--];
                return current != NULL;
            }

            
            K key() const {
                NanoAssert(current != NULL);
                return current->head.key;
            }

             
            T value() const {
                NanoAssert(current != NULL);
                return current->head.value;
            }
        };

        
        bool isEmpty() {
            Iter iter(*this);
            return !iter.next();
        }
    };

    



    template<class K, class T> class TreeMap {
        Allocator& alloc;
        class Node {
        public:
            Node* left;
            Node* right;
            K key;
            T value;
            Node(K k, T v) : left(NULL), right(NULL), key(k), value(v)
            { }
        };
        Node* root;

        



        void insert(Node* &n, K k, T v) {
            if (!n)
                n = new (alloc) Node(k, v);
            else if (k == n->key)
                n->value = v;
            else if (k < n->key)
                insert(n->left, k, v);
            else
                insert(n->right, k, v);
        }

        



        Node* find(Node* n, K k) {
            if (!n)
                return NULL;
            if (k == n->key)
                return n;
            if (k < n->key)
                return find(n->left, k);
            if (n->right)
                return find(n->right, k);
            return n;
        }

    public:
        TreeMap(Allocator& alloc) : alloc(alloc), root(NULL)
        { }

        
        void put(K k, T v) {
            insert(root, k, v);
        }

        

        K findNear(K k) {
            Node* n = find(root, k);
            return n ? n->key : 0;
        }

        
        T get(K k) {
            Node* n = find(root, k);
            return (n && n->key == k) ? n->value : 0;
        }

        
        bool containsKey(K k) {
            Node* n = find(root, k);
            return n && n->key == k;
        }

        
        void clear() {
            root = NULL;
        }
    };
}
#endif 
