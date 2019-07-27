





#ifndef nsDOMDataChannelDeclarations_h
#define nsDOMDataChannelDeclarations_h




#include "nsCOMPtr.h"
#include "nsIDOMDataChannel.h"

namespace mozilla {
   class DataChannel;
}

class nsPIDOMWindow;

nsresult
NS_NewDOMDataChannel(already_AddRefed<mozilla::DataChannel>&& dataChannel,
                     nsPIDOMWindow* aWindow,
                     nsIDOMDataChannel** domDataChannel);


void NS_DataChannelAppReady(nsIDOMDataChannel* domDataChannel);

#endif 
