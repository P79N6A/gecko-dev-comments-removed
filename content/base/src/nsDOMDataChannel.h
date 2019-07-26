





#ifndef nsDOMDataChannel_h__
#define nsDOMDataChannel_h__




#include "nsCOMPtr.h"
#include "nsIDOMDataChannel.h"

namespace mozilla {
   class DataChannel;
}

class nsPIDOMWindow;

nsresult
NS_NewDOMDataChannel(already_AddRefed<mozilla::DataChannel> dataChannel,
                     nsPIDOMWindow* aWindow,
                     nsIDOMDataChannel** domDataChannel);


void NS_DataChannelAppReady(nsIDOMDataChannel* domDataChannel);

#endif
