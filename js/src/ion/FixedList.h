






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

    void shrink(size_t num) {
        JS_ASSERT(num < length_);
        length_ -= num;
    }

    bool growBy(size_t num) {
        T *list = (T *)GetIonContext()->temp->allocate((length_ + num) * sizeof(T));
        if (!list)
            return false;

        for (size_t i = 0; i < length_; i++)
            list[i] = list_[i];

        length_ += num;
        list_ = list;
        return true;
    }

    T &operator[](size_t index) {
        JS_ASSERT(index < length_);
        return list_[index];
    }
    const T &operator [](size_t index) const {
        JS_ASSERT(index < length_);
        return list_[index];
    }
};

} 
} 

#endif 

