








































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
    };

  public:
    iterator begin() {
        return iterator(head.next);
    }

    iterator end() {
        return iterator(&head);
    }

    iterator removeAt(iterator &where) {
        Node *node = where.iter;
        iterator iter(where);
        iter++;

        node->prev->next = node->next;
        node->next->prev = node->prev;

        return iter;
    }

    void insert(Node *t) {
        t->prev = head.prev;
        t->next = &head;
        head.prev->next = t;
        head.prev = t;
    }
};

} 

#endif 

