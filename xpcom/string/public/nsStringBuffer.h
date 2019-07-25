





































#ifndef nsStringBuffer_h__
#define nsStringBuffer_h__











class nsStringBuffer
  {
    private:
      friend class CheckStaticAtomSizes;

      PRInt32  mRefCount;
      PRUint32 mStorageSize;

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

      




      PRUint32 StorageSize() const
        {
          return mStorageSize;
        }

      







      bool IsReadonly() const
        {
          return mRefCount > 1;
        }

      







      static nsStringBuffer* FromString(const nsAString &str);
      static nsStringBuffer* FromString(const nsACString &str);

      













      void ToString(PRUint32 len, nsAString &str,
                           bool aMoveOwnership = false);
      void ToString(PRUint32 len, nsACString &str,
                           bool aMoveOwnership = false);
  };

#endif 
