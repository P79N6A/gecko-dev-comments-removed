








































#ifndef js_inline_list_h__
#define js_inline_list_h__

namespace js {

template <typename T> class InlineForwardList;
template <typename T> class InlineForwardListIterator;

template <typename T>
class InlineForwardListNode
{
  public:
    InlineForwardListNode() : next(NULL)
    { }
    InlineForwardListNode(InlineForwardListNode<T> *n) : next(n)
    { }

  protected:
    friend class InlineForwardList<T>;
    friend class InlineForwardListIterator<T>;

    InlineForwardListNode<T> *next;
};

template <typename T>
class InlineForwardList : protected InlineForwardListNode<T>
{
    friend class InlineForwardListIterator<T>;

    typedef InlineForwardListNode<T> Node;

    Node *tail_;
#ifdef DEBUG
    uintptr_t modifyCount_;
#endif

    InlineForwardList<T> *thisFromConstructor() {
        return this;
    }

  public:
    InlineForwardList()
      : tail_(thisFromConstructor())
#ifdef DEBUG
      ,  modifyCount_(0)
#endif
    { }

  public:
    typedef InlineForwardListIterator<T> iterator;

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
    void pushBack(Node *t) {
#ifdef DEBUG
        modifyCount_++;
#endif
        tail_->next = t;
        t->next = NULL;
        tail_ = t;
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
        if (at == tail_)
            tail_ = item;
        item->next = at->next;
        at->next = item;
    }
    void removeAfter(Node *at, Node *item) {
#ifdef DEBUG
        modifyCount_++;
#endif
        if (item == tail_)
            tail_ = at;
        JS_ASSERT(at->next == item);
        at->next = item->next;
    }
    bool empty() const {
        return tail_ == this;
    }
};

template <typename T>
class InlineForwardListIterator
{
private:
    friend class InlineForwardList<T>;

    typedef InlineForwardListNode<T> Node;

    InlineForwardListIterator<T>(const InlineForwardList<T> *owner)
      : prev(const_cast<Node *>(static_cast<const Node *>(owner))),
        iter(owner ? owner->next : NULL)
#ifdef DEBUG
      , owner(owner),
        modifyCount(owner ? owner->modifyCount_ : 0)
#endif
    { }

public:
    InlineForwardListIterator<T> & operator ++() {
        JS_ASSERT(modifyCount == owner->modifyCount_);
        prev = iter;
        iter = iter->next;
        return *this;
    }
    InlineForwardListIterator<T> operator ++(int) {
        JS_ASSERT(modifyCount == owner->modifyCount_);
        InlineForwardListIterator<T> old(*this);
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
    bool operator !=(const InlineForwardListIterator<T> &where) const {
        return iter != where.iter;
    }
    bool operator ==(const InlineForwardListIterator<T> &where) const {
        return iter == where.iter;
    }

private:
    Node *prev;
    Node *iter;
#ifdef DEBUG
    const InlineForwardList<T> *owner;
    uintptr_t modifyCount;
#endif
};

template <typename T> class InlineList;
template <typename T> class InlineListIterator;
template <typename T> class InlineListReverseIterator;

template <typename T>
class InlineListNode : public InlineForwardListNode<T>
{
  public:
    InlineListNode() : InlineForwardListNode<T>(NULL), prev(NULL)
    { }
    InlineListNode(InlineListNode<T> *n, InlineListNode<T> *p)
      : InlineForwardListNode<T>(n),
        prev(p)
    { }

  protected:
    friend class InlineList<T>;
    friend class InlineListIterator<T>;
    friend class InlineListReverseIterator<T>;

    InlineListNode<T> *prev;
};

template <typename T>
class InlineList : protected InlineListNode<T>
{
    typedef InlineListNode<T> Node;

    
    InlineList<T> *thisFromConstructor() {
        return this;
    }

  public:
    InlineList() : InlineListNode<T>(thisFromConstructor(), thisFromConstructor())
    { }

  public:
    typedef InlineListIterator<T> iterator;
    typedef InlineListReverseIterator<T> reverse_iterator;

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
class InlineListIterator
{
  private:
    friend class InlineList<T>;

    typedef InlineListNode<T> Node;

    InlineListIterator(const Node *iter)
      : iter(const_cast<Node *>(iter))
    { }

  public:
    InlineListIterator<T> & operator ++() {
        iter = iter->next;
        return *iter;
    }
    InlineListIterator<T> operator ++(int) {
        InlineListIterator<T> old(*this);
        iter = static_cast<Node *>(iter->next);
        return old;
    }
    InlineListIterator<T> operator --(int) {
        InlineListIterator<T> old(*this);
        iter = iter->prev;
        return old;
    }
    T * operator *() const {
        return static_cast<T *>(iter);
    }
    T * operator ->() const {
        return static_cast<T *>(iter);
    }
    bool operator !=(const InlineListIterator<T> &where) const {
        return iter != where.iter;
    }
    bool operator ==(const InlineListIterator<T> &where) const {
        return iter == where.iter;
    }

  private:
    Node *iter;
};

template <typename T>
class InlineListReverseIterator
{
  private:
    friend class InlineList<T>;

    typedef InlineListNode<T> Node;

    InlineListReverseIterator(const Node *iter)
      : iter(const_cast<Node *>(iter))
    { }

  public:
    InlineListReverseIterator<T> & operator ++() {
        iter = iter->prev;
        return *iter;
    }
    InlineListReverseIterator<T> operator ++(int) {
        InlineListReverseIterator<T> old(*this);
        iter = iter->prev;
        return old;
    }
    T * operator *() {
        return static_cast<T *>(iter);
    }
    T * operator ->() {
        return static_cast<T *>(iter);
    }
    bool operator !=(const InlineListReverseIterator<T> &where) const {
        return iter != where.iter;
    }
    bool operator ==(const InlineListReverseIterator<T> &where) const {
        return iter == where.iter;
    }

  private:
    Node *iter;
};

} 

#endif 
