



#include "chrome/common/property_bag.h"

PropertyBag::PropertyBag() {
}

PropertyBag::PropertyBag(const PropertyBag& other) {
  operator=(other);
}

PropertyBag::~PropertyBag() {
}

PropertyBag& PropertyBag::operator=(const PropertyBag& other) {
  props_.clear();

  
  for (PropertyMap::const_iterator i = other.props_.begin();
       i != other.props_.end(); ++i)
    props_[i->first] = linked_ptr<Prop>(i->second->copy());
  return *this;
}

void PropertyBag::SetProperty(PropID id, Prop* prop) {
  props_[id] = linked_ptr<Prop>(prop);
}

PropertyBag::Prop* PropertyBag::GetProperty(PropID id) {
  PropertyMap::const_iterator found = props_.find(id);
  if (found == props_.end())
    return NULL;
  return found->second.get();
}

const PropertyBag::Prop* PropertyBag::GetProperty(PropID id) const {
  PropertyMap::const_iterator found = props_.find(id);
  if (found == props_.end())
    return NULL;
  return found->second.get();
}

void PropertyBag::DeleteProperty(PropID id) {
  PropertyMap::iterator found = props_.find(id);
  if (found == props_.end())
    return;  
  props_.erase(found);
}

PropertyAccessorBase::PropertyAccessorBase() {
  static PropertyBag::PropID next_id = 1;
  prop_id_ = next_id++;
}
