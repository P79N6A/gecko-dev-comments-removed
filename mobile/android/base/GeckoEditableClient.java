




package org.mozilla.gecko;

import android.os.Handler;
import android.text.Editable;




interface GeckoEditableClient {
    void sendEvent(GeckoEvent event);
    Editable getEditable();
    void setUpdateGecko(boolean update);
    void setSuppressKeyUp(boolean suppress);
    Handler getInputConnectionHandler();
    boolean setInputConnectionHandler(Handler handler);
}
