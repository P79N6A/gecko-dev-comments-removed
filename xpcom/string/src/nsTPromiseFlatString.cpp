





































void
nsTPromiseFlatString_CharT::Init(const substring_type& str)
  {
    if (str.IsTerminated())
      {
        mData = const_cast<char_type*>(str.Data());
        mLength = str.Length();
        mFlags = F_TERMINATED; 
      }
    else
      {
        Assign(str);
      }
  }
