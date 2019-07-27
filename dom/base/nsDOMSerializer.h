





#ifndef nsDOMSerializer_h_
#define nsDOMSerializer_h_

#include "nsIDOMSerializer.h"
#include "nsWrapperCache.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/XMLSerializerBinding.h"
#include "nsAutoPtr.h"

class nsINode;

class nsDOMSerializer final : public nsIDOMSerializer,
                              public nsWrapperCache
{
public:
  nsDOMSerializer();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMSerializer)

  
  NS_DECL_NSIDOMSERIALIZER

  
  static already_AddRefed<nsDOMSerializer>
  Constructor(const mozilla::dom::GlobalObject& aOwner,
              mozilla::ErrorResult& rv)
  {
    nsRefPtr<nsDOMSerializer> domSerializer = new nsDOMSerializer(aOwner.GetAsSupports());
    return domSerializer.forget();
  }

  void
  SerializeToString(nsINode& aRoot, nsAString& aStr,
                    mozilla::ErrorResult& rv);

  void
  SerializeToStream(nsINode& aRoot, nsIOutputStream* aStream,
                    const nsAString& aCharset, mozilla::ErrorResult& rv);

  nsISupports* GetParentObject() const
  {
    return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override
  {
    return mozilla::dom::XMLSerializerBinding::Wrap(aCx, this, aGivenProto);
  }

private:
  virtual ~nsDOMSerializer();

  explicit nsDOMSerializer(nsISupports* aOwner) : mOwner(aOwner)
  {
    MOZ_ASSERT(aOwner);
  }

  nsCOMPtr<nsISupports> mOwner;
};


#endif

