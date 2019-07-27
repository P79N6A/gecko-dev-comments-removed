





void
nsTPromiseFlatString_CharT::Init(const substring_type& str)
{
  if (str.IsTerminated()) {
    mData = const_cast<char_type*>(static_cast<const char_type*>(str.Data()));
    mLength = str.Length();
    mFlags = str.mFlags & (F_TERMINATED | F_LITERAL);
    
  } else {
    Assign(str);
  }
}
