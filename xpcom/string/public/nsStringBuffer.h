





































#ifndef nsStringBuffer_h__
#define nsStringBuffer_h__










class nsStringBuffer
  {
    private:

      PRInt32  mRefCount;
      PRUint32 mStorageSize;

    public:
      
      













      NS_COM static nsStringBuffer* Alloc(size_t storageSize);

      










      NS_COM static nsStringBuffer* Realloc(nsStringBuffer* buf, size_t storageSize);

      


      NS_COM void NS_FASTCALL AddRef();

      



      NS_COM void NS_FASTCALL Release();

      




      static nsStringBuffer* FromData(void* data)
        {
          return (nsStringBuffer*) ( ((char*) data) - sizeof(nsStringBuffer) );
        }

      


      void* Data() const
        {
          return (void*) ( ((char*) this) + sizeof(nsStringBuffer) );
        }

      




      PRUint32 StorageSize() const
        {
          return mStorageSize;
        }

      







      PRBool IsReadonly() const
        {
          return mRefCount > 1;
        }

      







      NS_COM static nsStringBuffer* FromString(const nsAString &str);
      NS_COM static nsStringBuffer* FromString(const nsACString &str);

      













      NS_COM void ToString(PRUint32 len, nsAString &str);
      NS_COM void ToString(PRUint32 len, nsACString &str);
  };

#endif 
