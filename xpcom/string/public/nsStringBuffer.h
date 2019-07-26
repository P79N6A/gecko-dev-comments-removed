





#ifndef nsStringBuffer_h__
#define nsStringBuffer_h__











class nsStringBuffer
  {
    private:
      friend class CheckStaticAtomSizes;

      int32_t  mRefCount;
      uint32_t mStorageSize;

    public:
      
      













      static nsStringBuffer* Alloc(size_t storageSize);

      










      static nsStringBuffer* Realloc(nsStringBuffer* buf, size_t storageSize);

      


      void NS_FASTCALL AddRef();

      



      void NS_FASTCALL Release();

      




      static nsStringBuffer* FromData(void* data)
        {
          return reinterpret_cast<nsStringBuffer*> (data) - 1;
        }

      


      void* Data() const
        {
          return const_cast<char*> (reinterpret_cast<const char*> (this + 1));
        }

      




      uint32_t StorageSize() const
        {
          return mStorageSize;
        }

      







      bool IsReadonly() const
        {
          return mRefCount > 1;
        }

      







      static nsStringBuffer* FromString(const nsAString &str);
      static nsStringBuffer* FromString(const nsACString &str);

      













      void ToString(uint32_t len, nsAString &str,
                           bool aMoveOwnership = false);
      void ToString(uint32_t len, nsACString &str,
                           bool aMoveOwnership = false);

      



      size_t SizeOfIncludingThisMustBeUnshared(nsMallocSizeOfFun aMallocSizeOf) const;

      


      size_t SizeOfIncludingThisIfUnshared(nsMallocSizeOfFun aMallocSizeOf) const;
  };

#endif 
