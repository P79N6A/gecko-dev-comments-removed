





#ifndef nsDOMDataChannel_h__
#define nsDOMDataChannel_h__




#include "nsIDOMDataChannel.h"

namespace mozilla {
   class DataChannel;
}

class nsPIDOMWindow;

nsresult
NS_NewDOMDataChannel(mozilla::DataChannel* dataChannel,
                     nsPIDOMWindow* aWindow,
                     nsIDOMDataChannel** domDataChannel);


void NS_DataChannelAppReady(nsIDOMDataChannel* domDataChannel);

#endif
