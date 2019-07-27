





nsTAdoptingString_CharT&
nsTAdoptingString_CharT::operator=(const self_type& str)
{
  
  
  self_type* mutable_str = const_cast<self_type*>(&str);

  if (str.mFlags & F_OWNED) {
    
    
    
    
    NS_ASSERTION(str.mData, "String with null mData?");
    Finalize();
    mData = str.mData;
    mLength = str.mLength;
    SetDataFlags(F_TERMINATED | F_OWNED);

    
    new (mutable_str) self_type();
  } else {
    Assign(str);

    mutable_str->Truncate();
  }

  return *this;
}

void
nsTString_CharT::Rebind(const char_type* data, size_type length)
{
  
  Finalize();

  mData = const_cast<char_type*>(data);
  mLength = length;
  SetDataFlags(F_TERMINATED);
  AssertValidDepedentString();
}

