






#ifndef jsion_ion_fixed_list_h__
#define jsion_ion_fixed_list_h__

namespace js {
namespace ion {


template <typename T>
class FixedList
{
    size_t length_;
    T *list_;

  private:
    FixedList(const FixedList&); 
    void operator= (const FixedList*); 

  public:
    FixedList()
      : length_(0)
    { }

    
    bool init(size_t length) {
        length_ = length;
        if (length == 0)
            return true;

        list_ = (T *)GetIonContext()->temp->allocate(length * sizeof(T));
        return list_ != NULL;
    }

    size_t length() const {
        return length_;
    }

    T &operator[](size_t index) {
        JS_ASSERT(index < length_);
        return list_[index];
    }
    const T &operator [](size_t index) const {
        JS_ASSERT(index < length_);
        return list_[index];
    };
};

} 
} 

#endif 

