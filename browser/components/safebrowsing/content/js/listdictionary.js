
























































function ListDictionary(name) {
  this.name_ = name;
  this.members_ = [];
}







ListDictionary.prototype.isMember = function(item) {
  for (var i=0; i < this.members_.length; i++)
    if (this.members_[i] == item)
      return true;
  return false;
}






ListDictionary.prototype.addMember = function(item) {
  this.members_.push(item);
}







ListDictionary.prototype.removeMember = function(item) {
  for (var i=0; i < this.members_.length; i++) {
    if (this.members_[i] == item) {
      for (var j=i; j < this.members_.length; j++)
        this.members_[j] = this.members_[j+1];

      this.members_.length--;
      return true;
    }
  }
  return false;
}







ListDictionary.prototype.forEach = function(func) {
  if (typeof func != "function")
    throw new Error("argument to forEach is not a function, it's a(n) " + 
                    typeof func);

  for (var i=0; i < this.members_.length; i++)
    func(this.members_[i]);
}
