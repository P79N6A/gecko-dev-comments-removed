






































  




class nsTObsoleteAString_CharT
  {
    public:
      



      NS_COM static const void *sCanonicalVTable;

        





      enum nsFragmentRequest { kPrevFragment, kFirstFragment, kLastFragment, kNextFragment, kFragmentAt };

        











      struct nsReadableFragment
        {
          const CharT*  mStart;
          const CharT*  mEnd;
          const void*   mFragmentIdentifier;

          nsReadableFragment() : mStart(0), mEnd(0), mFragmentIdentifier(0) {}
        };


        











      struct nsWritableFragment
        {
          CharT*    mStart;
          CharT*    mEnd;
          void*     mFragmentIdentifier;

          nsWritableFragment() : mStart(0), mEnd(0), mFragmentIdentifier(0) {}
        };

    protected:

      typedef CharT                                char_type;

      typedef void                                 buffer_handle_type;
      typedef void                                 shared_buffer_handle_type;
      typedef nsReadableFragment                   const_fragment_type;
      typedef nsWritableFragment                   fragment_type;

      typedef nsTAString_CharT                     abstract_string_type;

      typedef PRUint32                             size_type;
      typedef PRUint32                             index_type;

    protected:

      friend class nsTAString_CharT;
      friend class nsTSubstring_CharT;

      

      virtual ~nsTObsoleteAString_CharT() { }

      virtual PRUint32                          GetImplementationFlags() const = 0;
      virtual const        buffer_handle_type*  GetFlatBufferHandle()    const = 0;
      virtual const        buffer_handle_type*  GetBufferHandle()        const = 0;
      virtual const shared_buffer_handle_type*  GetSharedBufferHandle()  const = 0;

      virtual size_type Length() const = 0;

      virtual PRBool IsVoid() const = 0;
      virtual void SetIsVoid( PRBool ) = 0;

      virtual void SetCapacity( size_type ) = 0;
      virtual void SetLength( size_type ) = 0;

      virtual void Cut( index_type cutStart, size_type cutLength ) = 0;

      virtual void do_AssignFromReadable( const abstract_string_type& ) = 0;
      virtual void do_AssignFromElementPtr( const char_type* ) = 0;
      virtual void do_AssignFromElementPtrLength( const char_type*, size_type ) = 0;
      virtual void do_AssignFromElement( char_type ) = 0;

      virtual void do_AppendFromReadable( const abstract_string_type& ) = 0;
      virtual void do_AppendFromElementPtr( const char_type* ) = 0;
      virtual void do_AppendFromElementPtrLength( const char_type*, size_type ) = 0;
      virtual void do_AppendFromElement( char_type ) = 0;

      virtual void do_InsertFromReadable( const abstract_string_type&, index_type ) = 0;
      virtual void do_InsertFromElementPtr( const char_type*, index_type ) = 0;
      virtual void do_InsertFromElementPtrLength( const char_type*, index_type, size_type ) = 0;
      virtual void do_InsertFromElement( char_type, index_type ) = 0;

      virtual void do_ReplaceFromReadable( index_type, size_type, const abstract_string_type& ) = 0;

      virtual const char_type* GetReadableFragment( const_fragment_type&, nsFragmentRequest, PRUint32 = 0 ) const = 0;
      virtual       char_type* GetWritableFragment(       fragment_type&, nsFragmentRequest, PRUint32 = 0 ) = 0;
  };

  
class nsTObsoleteAStringThunk_CharT;
