






































NS_COM nsTAdoptingString_CharT&
nsTAdoptingString_CharT::operator=( const self_type& str )
  {
    
    
    self_type* mutable_str = const_cast<self_type*>(&str);

    if (str.mFlags & F_OWNED)
      {
        
        
        
        
        NS_ASSERTION(str.mData, "String with null mData?");
        Finalize();
        mData = str.mData;
        mLength = str.mLength;
        SetDataFlags(F_TERMINATED | F_OWNED);

        
        new (mutable_str) self_type();
      }
    else
      {
        Assign(str);

        mutable_str->Truncate();
      }

    return *this;
  }

nsTFixedString_CharT::nsTFixedString_CharT( char_type* data, size_type storageSize )
  : string_type(data, PRUint32(char_traits::length(data)), F_TERMINATED | F_FIXED | F_CLASS_FIXED)
    , mFixedCapacity(storageSize - 1)
    , mFixedBuf(data)
{}

nsTFixedString_CharT::nsTFixedString_CharT( char_type* data, size_type storageSize, size_type length )
  : string_type(data, length, F_TERMINATED | F_FIXED | F_CLASS_FIXED)
    , mFixedCapacity(storageSize - 1)
    , mFixedBuf(data)
{
  
  mFixedBuf[length] = char_type(0);
}
