








































#ifndef js_inline_list_h__
#define js_inline_list_h__

namespace js {

template <typename T> class InlineList;

template <typename T>
class InlineListNode
{
    friend class InlineList<T>;
  public:
    InlineListNode() : next(NULL), prev(NULL)
    { }
    InlineListNode(InlineListNode *n, InlineListNode *p) : next(n), prev(p)
    { }

  protected:
    InlineListNode *next;
    InlineListNode *prev;
};

template <typename T>
class InlineList
{
    typedef InlineListNode<T> Node;

    Node head;

  public:
    InlineList() : head(&head, &head)
    { }

  public:
    class iterator
    {
        friend class InlineList;
        Node *iter;
      public:
        iterator(Node *iter) : iter(iter) { }

        iterator & operator ++() {
            iter = iter->next;
            return *iter;
        }
        iterator operator ++(int) {
            iterator old(*this);
            iter = iter->next;
            return old;
        }
        iterator operator --(int) {
            iterator old(*this);
            iter = iter->prev;
            return old;
        }
        T * operator *() {
            return static_cast<T *>(iter);
        }
        T * operator ->() {
            return static_cast<T *>(iter);
        }
        bool operator != (const iterator &where) const {
            return iter != where.iter;
        }
        bool operator == (const iterator &where) const {
            return iter == where.iter;
        }
        iterator prev() {
            iterator p(iter->prev);
            return p;
        }
    };

    class reverse_iterator
    {
        friend class InlineList;
        Node *iter;
      public:
        reverse_iterator(Node *iter) : iter(iter) { }

        reverse_iterator & operator ++() {
            iter = iter->prev;
            return *iter;
        }
        reverse_iterator operator ++(int) {
            reverse_iterator old(*this);
            iter = iter->prev;
            return old;
        }
        T * operator *() {
            return static_cast<T *>(iter);
        }
        T * operator ->() {
            return static_cast<T *>(iter);
        }
        bool operator != (const reverse_iterator &where) const {
            return iter != where.iter;
        }
        bool operator == (const reverse_iterator &where) const {
            return iter == where.iter;
        }
    };

    class const_iterator
    {
        friend class InlineList;
        const Node *iter;
      public:
        const_iterator(const Node *iter) : iter(iter) { }

        const_iterator & operator ++() {
            iter = iter->next;
            return *iter;
        }
        const_iterator operator ++(int) {
            const_iterator old(*this);
            iter = iter->next;
            return old;
        }
        T * operator *() const {
            return const_cast<T *>(static_cast<const T *>(iter));
        }
        T * operator ->() {
            return const_cast<T *>(static_cast<const T *>(iter));
        }
        bool operator != (const const_iterator &where) const {
            return iter != where.iter;
        }
        bool operator == (const const_iterator &where) const {
            return iter == where.iter;
        }
    };

  public:
    iterator begin() {
        return iterator(head.next);
    }
    iterator end() {
        return iterator(&head);
    }
    reverse_iterator rbegin() {
        return reverse_iterator(head.prev);
    }
    reverse_iterator rend() {
        return reverse_iterator(&head);
    }
    const_iterator begin() const {
        return const_iterator(head.next);
    }
    const_iterator rbegin() const {
        return const_iterator(head.prev);
    }
    const_iterator end() const {
        return const_iterator(&head);
    }

    iterator removeAt(iterator &where) {
        Node *node = where.iter;
        iterator iter(where);
        iter++;

        remove(node);

        
        
        
        
        where.iter = NULL;

        return iter;
    }

    void insertBefore(Node *at, Node *item) {
        item->next = at;
        item->prev = at->prev;
        at->prev->next = item;
        at->prev = item;
    }

    void insertAfter(Node *at, Node *item) {
        item->next = at->next;
        item->prev = at;
        at->next->prev = item;
        at->next = item;
    }

    void insert(Node *t) {
        t->prev = head.prev;
        t->next = &head;
        head.prev->next = t;
        head.prev = t;
    }

    void remove(Node *t) {
        t->prev->next = t->next;
        t->next->prev = t->prev;
        t->next = t->prev = NULL;
    }

    bool empty() const {
        return begin() == end();
    }

    T *pop() {
        iterator iter = end();
        iter--;
        T *t = *iter;
        remove(*iter);
        return t;
    }
    T *peekBack() {
        iterator iter = end();
        iter--;
        return *iter;
    }
};

} 

#endif 

