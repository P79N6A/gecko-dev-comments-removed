




package org.mozilla.javaaddons;

import android.content.Context;
import org.json.JSONObject;

public interface JavaAddonInterfaceV1 {
    






    interface EventCallback {
        





        public void sendSuccess(Object response);

        





        public void sendError(Object response);
    }

    interface EventDispatcher {
        void registerEventListener(EventListener listener, String... events);
        void unregisterEventListener(EventListener listener);

        void sendRequestToGecko(String event, JSONObject message, RequestCallback callback);
    }

    interface EventListener {
        public void handleMessage(final Context context, final String event, final JSONObject message, final EventCallback callback);
    }

    interface RequestCallback {
        void onResponse(final Context context, JSONObject jsonObject);
    }
}
