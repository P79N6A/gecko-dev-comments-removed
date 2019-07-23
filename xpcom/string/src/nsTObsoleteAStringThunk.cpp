






































class nsTObsoleteAStringThunk_CharT : public nsTObsoleteAString_CharT
  {
    public:
      typedef nsTObsoleteAStringThunk_CharT    self_type; 
      typedef nsTSubstring_CharT               substring_type;

    public:

      nsTObsoleteAStringThunk_CharT() {}


      static const void* get_vptr()
        {
          const void* result;
          new (&result) self_type();
          return result;
        }


        



            substring_type* concrete_self()       { return reinterpret_cast<substring_type*>(this); }
      const substring_type* concrete_self() const { return reinterpret_cast<const substring_type*>(this); }


        



      virtual ~nsTObsoleteAStringThunk_CharT()
        {
          concrete_self()->Finalize();
        }

      virtual PRUint32 GetImplementationFlags() const
        {
          return 0;
        }

      virtual const buffer_handle_type* GetFlatBufferHandle() const
        {
          return (const buffer_handle_type*) (concrete_self()->IsTerminated() != PR_FALSE);
        }

      virtual const buffer_handle_type*  GetBufferHandle() const
        {
          return 0;
        }

      virtual const shared_buffer_handle_type* GetSharedBufferHandle() const
        {
          return 0;
        }

      virtual size_type Length() const
        {
          return concrete_self()->Length();
        }

      virtual PRBool IsVoid() const
        {
          return concrete_self()->IsVoid();
        }

      virtual void SetIsVoid(PRBool val)
        {
          concrete_self()->SetIsVoid(val);
        }

      virtual void SetCapacity(size_type size)
        {
          concrete_self()->SetCapacity(size);
        }

      virtual void SetLength(size_type size)
        {
          concrete_self()->SetLength(size);
        }

      virtual void Cut(index_type cutStart, size_type cutLength)
        {
          concrete_self()->Cut(cutStart, cutLength);
        }

      virtual void do_AssignFromReadable(const abstract_string_type &s)
        {
          concrete_self()->Assign(s);
        }

      virtual void do_AssignFromElementPtr(const char_type *data)
        {
          concrete_self()->Assign(data);
        }

      virtual void do_AssignFromElementPtrLength(const char_type *data, size_type length)
        {
          concrete_self()->Assign(data, length);
        }

      virtual void do_AssignFromElement(char_type c)
        {
          concrete_self()->Assign(c);
        }

      virtual void do_AppendFromReadable(const abstract_string_type &s)
        {
          concrete_self()->Append(s);
        }

      virtual void do_AppendFromElementPtr(const char_type *data)
        {
          concrete_self()->Append(data);
        }

      virtual void do_AppendFromElementPtrLength(const char_type *data, size_type length)
        {
          concrete_self()->Append(data, length);
        }

      virtual void do_AppendFromElement(char_type c)
        {
          concrete_self()->Append(c);
        }

      virtual void do_InsertFromReadable(const abstract_string_type &s, index_type pos)
        {
          concrete_self()->Insert(s, pos);
        }

      virtual void do_InsertFromElementPtr(const char_type *data, index_type pos)
        {
          concrete_self()->Insert(data, pos);
        }

      virtual void do_InsertFromElementPtrLength(const char_type *data, index_type pos, size_type length)
        {
          concrete_self()->Insert(data, pos, length);
        }

      virtual void do_InsertFromElement(char_type c, index_type pos)
        {
          concrete_self()->Insert(c, pos);
        }

      virtual void do_ReplaceFromReadable(index_type cutStart, size_type cutLength, const abstract_string_type &s)
        {
          concrete_self()->Replace(cutStart, cutLength, s);
        }

      virtual const char_type *GetReadableFragment(const_fragment_type& frag, nsFragmentRequest which, PRUint32 offset) const
        {
          const substring_type* s = concrete_self();
          switch (which)
            {
              case kFirstFragment:
              case kLastFragment:
              case kFragmentAt:
                frag.mStart = s->Data();
                frag.mEnd = frag.mStart + s->Length();
                return frag.mStart + offset;
              case kPrevFragment:
              case kNextFragment:
              default:
                return 0;
            }
        }

      virtual char_type *GetWritableFragment(fragment_type& frag, nsFragmentRequest which, PRUint32 offset)
        {
          substring_type* s = concrete_self();
          switch (which)
            {
              case kFirstFragment:
              case kLastFragment:
              case kFragmentAt:
                char_type* start;
                s->BeginWriting(start);
                frag.mStart = start;
                frag.mEnd = start + s->Length();
                return frag.mStart + offset;
              case kPrevFragment:
              case kNextFragment:
              default:
                return 0;
            }
        }
  };


  



const void *nsTObsoleteAString_CharT::sCanonicalVTable = nsTObsoleteAStringThunk_CharT::get_vptr();
