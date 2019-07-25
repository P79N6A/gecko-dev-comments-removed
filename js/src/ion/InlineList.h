








































#ifndef js_inline_list_h__
#define js_inline_list_h__

namespace js {

template <typename T> class InlineForwardList;

template <typename T>
class InlineForwardListNode
{
    friend class InlineForwardList<T>;

  public:
    InlineForwardListNode() : next(NULL)
    { }
    InlineForwardListNode(InlineForwardListNode *n) : next(n)
    { }

  protected:
    InlineForwardListNode *next;
};

template <typename T>
class InlineForwardList : protected InlineForwardListNode<T>
{
    typedef InlineForwardListNode<T> Node;

#ifdef DEBUG
    uintptr_t modifyCount_;

  public:
    InlineForwardList() : modifyCount_(0)
    { }
#endif

  public:
    class iterator;

  public:
    iterator begin() const {
        return iterator(this);
    }
    iterator end() const {
        return iterator(NULL);
    }
    iterator removeAt(iterator &where) {
        iterator iter(where);
        iter++;
        iter.prev = where.prev;
#ifdef DEBUG
        iter.modifyCount++;
#endif

        
        
        
        
        removeAfter(where.prev, where.iter);
        where.prev = where.iter = NULL;

        return iter;
    }
    void pushFront(Node *t) {
        insertAfter(this, t);
    }
    T *popFront() {
        JS_ASSERT(!empty());
        T* result = static_cast<T *>(this->next);
        removeAfter(this, result);
        return result;
    }
    void insertAfter(Node *at, Node *item) {
#ifdef DEBUG
        modifyCount_++;
#endif
        item->next = at->next;
        at->next = item;
    }
    void removeAfter(Node *at, Node *item) {
#ifdef DEBUG
        modifyCount_++;
#endif
        JS_ASSERT(at->next == item);
        at->next = item->next;
    }
    bool empty() const {
        return begin() == end();
    }
};

template <typename T>
class InlineForwardList<T>::iterator
{
    friend class InlineForwardList;
    Node *prev;
    Node *iter;
#ifdef DEBUG
    const InlineForwardList<T> *owner;
    uintptr_t modifyCount;
#endif

    iterator(const InlineForwardList<T> *owner)
      : prev(const_cast<Node *>(static_cast<const Node *>(owner))),
        iter(owner ? owner->next : NULL)
#ifdef DEBUG
      , owner(owner),
        modifyCount(owner ? owner->modifyCount_ : 0)
#endif
    { }

public:
    iterator & operator ++() {
        JS_ASSERT(modifyCount == owner->modifyCount_);
        prev = iter;
        iter = iter->next;
        return *this;
    }
    iterator operator ++(int) {
        JS_ASSERT(modifyCount == owner->modifyCount_);
        iterator old(*this);
        prev = iter;
        iter = iter->next;
        return old;
    }
    T * operator *() const {
        JS_ASSERT(modifyCount == owner->modifyCount_);
        return static_cast<T *>(iter);
    }
    T * operator ->() const {
        JS_ASSERT(modifyCount == owner->modifyCount_);
        return static_cast<T *>(iter);
    }
    bool operator !=(const iterator &where) const {
        return iter != where.iter;
    }
    bool operator ==(const iterator &where) const {
        return iter == where.iter;
    }
};

template <typename T> class InlineList;

template <typename T>
class InlineListNode : public InlineForwardListNode<T>
{
    friend class InlineList<T>;
  public:
    InlineListNode() : InlineForwardListNode<T>(NULL), prev(NULL)
    { }
    InlineListNode(InlineListNode *n, InlineListNode *p) : InlineForwardListNode<T>(n), prev(p)
    { }

  protected:
    InlineListNode *prev;
};

template <typename T>
class InlineList : protected InlineListNode<T>
{
    typedef InlineListNode<T> Node;

  public:
    InlineList() : InlineListNode<T>(this, this)
    { }

  public:
    class iterator;
    class reverse_iterator;

  public:
    iterator begin() const {
        return iterator(static_cast<Node *>(this->next));
    }
    iterator end() const {
        return iterator(this);
    }
    reverse_iterator rbegin() {
        return reverse_iterator(this->prev);
    }
    reverse_iterator rend() {
        return reverse_iterator(this);
    }
    iterator removeAt(iterator &where) {
        iterator iter(where);
        iter++;

        
        
        
        
        remove(where.iter);
        where.iter = NULL;

        return iter;
    }
    void pushFront(Node *t) {
        insertAfter(this, t);
    }
    void pushBack(Node *t) {
        insertBefore(this, t);
    }
    T *popFront() {
        JS_ASSERT(!empty());
        T *t = static_cast<T *>(this->next);
        remove(t);
        return t;
    }
    T *popBack() {
        JS_ASSERT(!empty());
        T *t = static_cast<T *>(this->prev);
        remove(t);
        return t;
    }
    T *peekBack() const {
        iterator iter = end();
        iter--;
        return *iter;
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
        static_cast<Node *>(at->next)->prev = item;
        at->next = item;
    }
    void remove(Node *t) {
        t->prev->next = t->next;
        static_cast<Node *>(t->next)->prev = t->prev;
        t->next = t->prev = NULL;
    }
    bool empty() const {
        return begin() == end();
    }
};

template <typename T>
class InlineList<T>::iterator
{
    friend class InlineList;
    Node *iter;

    iterator(const Node *iter) : iter(const_cast<Node *>(iter))
    { }

  public:
    iterator & operator ++() {
        iter = iter->next;
        return *iter;
    }
    iterator operator ++(int) {
        iterator old(*this);
        iter = static_cast<Node *>(iter->next);
        return old;
    }
    iterator operator --(int) {
        iterator old(*this);
        iter = iter->prev;
        return old;
    }
    T * operator *() const {
        return static_cast<T *>(iter);
    }
    T * operator ->() const {
        return static_cast<T *>(iter);
    }
    bool operator !=(const iterator &where) const {
        return iter != where.iter;
    }
    bool operator ==(const iterator &where) const {
        return iter == where.iter;
    }
};

template <typename T>
class InlineList<T>::reverse_iterator
{
    friend class InlineList;
    Node *iter;

    reverse_iterator(const Node *iter) : iter(const_cast<Node *>(iter))
    { }

  public:
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
    bool operator !=(const reverse_iterator &where) const {
        return iter != where.iter;
    }
    bool operator ==(const reverse_iterator &where) const {
        return iter == where.iter;
    }
};

} 

#endif 
