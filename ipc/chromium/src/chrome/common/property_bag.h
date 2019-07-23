



#ifndef CHROME_COMMON_PROPERTY_BAG_H_
#define CHROME_COMMON_PROPERTY_BAG_H_

#include <map>

#include "base/basictypes.h"
#include "base/linked_ptr.h"

class PropertyAccessorBase;





























class PropertyBag {
 public:
  
  typedef int PropID;
  enum { NULL_PROP_ID = -1 };  

  
  
  class Prop {
   public:
    virtual ~Prop() {}

    
    
    virtual Prop* copy() = 0;
  };

  PropertyBag();
  PropertyBag(const PropertyBag& other);
  virtual ~PropertyBag();

  PropertyBag& operator=(const PropertyBag& other);

 private:
  friend class PropertyAccessorBase;

  typedef std::map<PropID, linked_ptr<Prop> > PropertyMap;

  
  
  
  void SetProperty(PropID id, Prop* prop);

  
  
  
  Prop* GetProperty(PropID id);
  const Prop* GetProperty(PropID id) const;

  
  void DeleteProperty(PropID id);

  PropertyMap props_;

  
};





class PropertyAccessorBase {
 public:
  PropertyAccessorBase();
  virtual ~PropertyAccessorBase() {}

  
  void DeleteProperty(PropertyBag* bag) {
    bag->DeleteProperty(prop_id_);
  }

 protected:
  void SetPropertyInternal(PropertyBag* bag, PropertyBag::Prop* prop) {
    bag->SetProperty(prop_id_, prop);
  }
  PropertyBag::Prop* GetPropertyInternal(PropertyBag* bag) {
    return bag->GetProperty(prop_id_);
  }
  const PropertyBag::Prop* GetPropertyInternal(const PropertyBag* bag) const {
    return bag->GetProperty(prop_id_);
  }

 private:
  
  PropertyBag::PropID prop_id_;

  DISALLOW_COPY_AND_ASSIGN(PropertyAccessorBase);
};







template<class T>
class PropertyAccessor : public PropertyAccessorBase {
 public:
  PropertyAccessor() : PropertyAccessorBase() {}
  virtual ~PropertyAccessor() {}

  
  void SetProperty(PropertyBag* bag, const T& prop) {
    SetPropertyInternal(bag, new Container(prop));
  }

  
  
  T* GetProperty(PropertyBag* bag) {
    PropertyBag::Prop* prop = GetPropertyInternal(bag);
    if (!prop)
      return NULL;
    return static_cast<Container*>(prop)->get();
  }
  const T* GetProperty(const PropertyBag* bag) const {
    const PropertyBag::Prop* prop = GetPropertyInternal(bag);
    if (!prop)
      return NULL;
    return static_cast<const Container*>(prop)->get();
  }

  

 private:
  class Container : public PropertyBag::Prop {
   public:
    Container(const T& data) : data_(data) {}

    T* get() { return &data_; }
    const T* get() const { return &data_; }

   private:
    virtual Prop* copy() {
      return new Container(data_);
    }

    T data_;
  };

  DISALLOW_COPY_AND_ASSIGN(PropertyAccessor);
};

#endif  
