





































void
nsTPromiseFlatString_CharT::Init(const substring_type& str)
  {
#ifdef MOZ_V1_STRING_ABI
    
    
    mVTable = nsTObsoleteAString_CharT::sCanonicalVTable;
#endif

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

  
#ifdef MOZ_V1_STRING_ABI
void
nsTPromiseFlatString_CharT::Init(const abstract_string_type& readable)
  {
    if (readable.mVTable == nsTObsoleteAString_CharT::sCanonicalVTable)
      Init(*readable.AsSubstring());
    else
      Init(readable.ToSubstring());
  }
#endif
