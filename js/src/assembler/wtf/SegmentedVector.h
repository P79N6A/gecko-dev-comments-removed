



























#ifndef SegmentedVector_h
#define SegmentedVector_h

#include "jsprvtd.h"
#include "js/Vector.h"

namespace WTF {

    
    template <typename T, size_t SegmentSize> class SegmentedVector;
    template <typename T, size_t SegmentSize> class SegmentedVectorIterator {
    private:
        friend class SegmentedVector<T, SegmentSize>;
    public:
        typedef SegmentedVectorIterator<T, SegmentSize> Iterator;

        ~SegmentedVectorIterator() { }

        T& operator*() const { return (*m_vector.m_segments[m_segment])[m_index]; }
        T* operator->() const { return &(*m_vector.m_segments[m_segment])[m_index]; }

        
        Iterator& operator++()
        {
            ASSERT(m_index != SegmentSize);
            ++m_index;
            
            if (m_index >= m_vector.m_segments[m_segment]->length())  {
                
                if (m_segment + 1 < m_vector.m_segments.length()) {
                    
                    ASSERT(m_vector.m_segments[m_segment]->length() > 0);
                    ++m_segment;
                    m_index = 0;
                } else {
                    
                    m_segment = 0;
                    m_index = SegmentSize;
                }
            }
            return *this;
        }

        bool operator==(const Iterator& other) const
        {
            return (m_index == other.m_index && m_segment = other.m_segment && &m_vector == &other.m_vector);
        }

        bool operator!=(const Iterator& other) const
        {
            return (m_index != other.m_index || m_segment != other.m_segment || &m_vector != &other.m_vector);
        }

        SegmentedVectorIterator& operator=(const SegmentedVectorIterator<T, SegmentSize>& other)
        {
            m_vector = other.m_vector;
            m_segment = other.m_segment;
            m_index = other.m_index;
            return *this;
        }

    private:
        SegmentedVectorIterator(SegmentedVector<T, SegmentSize>& vector, size_t segment, size_t index)
            : m_vector(vector)
            , m_segment(segment)
            , m_index(index)
        {
        }

        SegmentedVector<T, SegmentSize>& m_vector;
        size_t m_segment;
        size_t m_index;
    };

    
    
    
    template <typename T, size_t SegmentSize> class SegmentedVector {
        friend class SegmentedVectorIterator<T, SegmentSize>;
    public:
        typedef SegmentedVectorIterator<T, SegmentSize> Iterator;

        SegmentedVector()
            : m_size(0)
        {
            m_segments.append(&m_inlineSegment);
        }

        ~SegmentedVector()
        {
            deleteAllSegments();
        }

        size_t size() const { return m_size; }
        bool isEmpty() const { return !size(); }

        T& at(size_t index)
        {
            if (index < SegmentSize)
                return m_inlineSegment[index];
            return segmentFor(index)->at(subscriptFor(index));
        }

        T& operator[](size_t index)
        {
            return at(index);
        }

        T& last()
        {
            return at(size() - 1);
        }

        template <typename U> void append(const U& value)
        {
            ++m_size;

            if (m_size <= SegmentSize) {
                
                m_inlineSegment.append(value);
                return;
            }

            if (!segmentExistsFor(m_size - 1))
                m_segments.append(new Segment);
            
            segmentFor(m_size - 1)->append(value);
        }

        T& alloc()
        {
            append<T>(T());
            return last();
        }

        void removeLast()
        {
            if (m_size <= SegmentSize)
                m_inlineSegment.removeLast();
            else
                segmentFor(m_size - 1)->removeLast();
            --m_size;
        }

        void grow(size_t size)
        {
            ASSERT(size > m_size);
            ensureSegmentsFor(size);
            m_size = size;
        }

        void clear()
        {
            deleteAllSegments();
            m_segments.resize(1);
            m_inlineSegment.clear();
            m_size = 0;
        }

        Iterator begin()
        {
            return Iterator(*this, 0, m_size ? 0 : SegmentSize);
        }

        Iterator end()
        {
            return Iterator(*this, 0, SegmentSize);
        }

    private:
        typedef js::Vector<T, SegmentSize ,js::SystemAllocPolicy > Segment;

        void deleteAllSegments()
        {
            
            
            
            for (size_t i = 1; i < m_segments.length(); i++)
                delete m_segments[i];
        }

        bool segmentExistsFor(size_t index)
        {
            
            return index / SegmentSize < m_segments.length();
        }

        Segment* segmentFor(size_t index)
        {
            return m_segments[index / SegmentSize];
        }

        size_t subscriptFor(size_t index)
        {
            return index % SegmentSize;
        }

        void ensureSegmentsFor(size_t size)
        {
            size_t segmentCount = m_size / SegmentSize;
            if (m_size % SegmentSize)
                ++segmentCount;
            
            segmentCount = segmentCount > 1 ? segmentCount : 1; 

            size_t neededSegmentCount = size / SegmentSize;
            if (size % SegmentSize)
                ++neededSegmentCount;

            
            size_t end = neededSegmentCount - 1;
            for (size_t i = segmentCount - 1; i < end; ++i)
                ensureSegment(i, SegmentSize);

            
            ensureSegment(end, subscriptFor(size - 1) + 1);
        }

        void ensureSegment(size_t segmentIndex, size_t size)
        {
            ASSERT(segmentIndex <= m_segments.size());
            if (segmentIndex == m_segments.size())
                m_segments.append(new Segment);
            m_segments[segmentIndex]->grow(size);
        }

        size_t m_size;
        Segment m_inlineSegment;
        js::Vector<Segment*, 32 ,js::SystemAllocPolicy > m_segments;
    };

} 

using WTF::SegmentedVector;

#endif 
