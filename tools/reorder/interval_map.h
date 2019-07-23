




































#ifndef interval_map_h__
#define interval_map_h__












#include <fstream>
#include <assert.h>

template<class coord, class T>
class interval_map {
protected:
    class const_iterator;
    friend class const_iterator;

    struct node {
        T      m_data;
        coord  m_min;
        coord  m_max;
        node  *m_before; 
        node  *m_within; 
        node  *m_after;  
        int    m_bal;
    };

public:
    



    class const_iterator {
    protected:
        const node  *m_node;
        const coord  m_point;

        friend class interval_map;
        const_iterator(const node *n, const coord &point)
            : m_node(n), m_point(point) {}

        void advance();

    public:
        const_iterator() : m_node(0), m_point(0) {}

        const_iterator(const const_iterator &iter)
            : m_node(iter.m_node), m_point(iter.m_point) {}

        const_iterator &
        operator=(const const_iterator &iter) {
            m_node = iter.m_node;
            m_point = iter.m_point; }

        const T &
        operator*() const { return m_node->m_data; }

        const T *
        operator->() const { return &m_node->m_data; }

        const_iterator &
        operator++() { advance(); return *this; }

        const_iterator
        operator++(int) {
            const_iterator temp(*this);
            advance();
            return temp; }

        bool
        operator==(const const_iterator &iter) const {
            return m_node == iter.m_node; }

        bool
        operator!=(const const_iterator &iter) const {
            return !iter.operator==(*this); }
    };

    interval_map() : m_root(0) {}

    ~interval_map() { delete m_root; }

    


    void put(coord min, coord max, const T &data) {
        put_into(&m_root, min, max, data);
#ifdef DEBUG
        verify(m_root, 0);
#endif
    }


    



    const_iterator get(coord point) const;

    


    const_iterator end() const {
        return const_iterator(0, 0); }

protected:
    void put_into(node **link, coord min, coord max, const T &data, bool *subsumed = 0);

    void left_rotate(node **link, node *node);
    void right_rotate(node **link, node *node);
#ifdef DEBUG
    void verify(node *node, int depth);
#endif
    node *m_root;
};

template<class coord, class T>
void
interval_map<coord, T>::put_into(node **root, coord min, coord max, const T &data, bool *subsumed)
{
    assert(min < max);
    node *interval = *root;

    if (interval) {
        bool before = min < interval->m_min;
        bool after = max > interval->m_max;

        if (!before || !after) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (before) {
                if (max > interval->m_min) {
                    put_into(&interval->m_within, interval->m_min, max, data);
                    max = interval->m_min;
                }

                bool was_subsumed = true;
                put_into(&interval->m_before, min, max, data, &was_subsumed);

                if (! was_subsumed) {
                    if (interval->m_bal < 0) {
                        if (interval->m_before->m_bal > 0)
                            left_rotate(&interval->m_before, interval->m_before);

                        right_rotate(root, interval);
                    }
                    else
                        --interval->m_bal;

                    if (subsumed)
                        *subsumed = (interval->m_bal == 0);
                }

                return;
            }

            if (after) {
                if (min < interval->m_max) {
                    put_into(&interval->m_within, min, interval->m_max, data);
                    min = interval->m_max;
                }

                bool was_subsumed = true;
                put_into(&interval->m_after, min, max, data, &was_subsumed);

                if (! was_subsumed) {
                    if (interval->m_bal > 0) {
                        if (interval->m_after->m_bal < 0)
                            right_rotate(&interval->m_after, interval->m_after);

                        left_rotate(root, interval);
                    }
                    else
                        ++interval->m_bal;

                    if (subsumed)
                        *subsumed = (interval->m_bal == 0);
                }

                return;
            }

            put_into(&interval->m_within, min, max, data);
            return;
        }

        
        
        
        
    }

    if (subsumed)
        *subsumed = false;

    node *n = new node();
    n->m_data   = data;
    n->m_before = n->m_after = 0;
    n->m_min    = min;
    n->m_max    = max;
    n->m_within = interval;
    n->m_bal    = 0;
    *root = n;
}










template<class coord, class T>
void
interval_map<coord, T>::left_rotate(node **link, node *x)
{
    node *y = x->m_after;
    x->m_after = y->m_before;
    *link = y;
    y->m_before = x;
    --x->m_bal;
    --y->m_bal;
}

template<class coord, class T>
void
interval_map<coord, T>::right_rotate(node **link, node *y)
{
    node *x = y->m_before;
    y->m_before = x->m_after;
    *link = x;
    x->m_after = y;
    ++y->m_bal;
    ++x->m_bal;
}

template<class coord, class T>
interval_map<coord, T>::const_iterator
interval_map<coord, T>::get(coord point) const
{
    node *interval = m_root;

    while (interval) {
        if (point < interval->m_min)
            interval = interval->m_before;
        else if (point > interval->m_max)
            interval = interval->m_after;
        else
            break;
    }

    return const_iterator(interval, point);
}


template<class coord, class T>
void
interval_map<coord, T>::const_iterator::advance()
{
    assert(m_node);

    m_node = m_node->m_within;

    while (m_node) {
        if (m_point < m_node->m_min)
            m_node = m_node->m_before;
        else if (m_point > m_node->m_max)
            m_node = m_node->m_after;
        else
            break;
    }
}

#ifdef DEBUG
template<class coord, class T>
void
interval_map<coord, T>::verify(node<coord, T> *node, int depth)
{
    if (node->m_after)
        verify(node->m_after, depth + 1);

    for (int i = 0; i < depth; ++i)
        cout << "  ";

    hex(cout);
    cout << node << "(";
    dec(cout);
    cout << node->m_bal << ")";
    hex(cout);
    cout << "[" << node->m_min << "," << node->m_max << "]";
    cout << "@" << node->m_data;
    cout << endl;

    if (node->m_before)
        verify(node->m_before, depth + 1);
}
#endif 

#endif 
