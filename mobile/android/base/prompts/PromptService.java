




package org.mozilla.gecko.prompts;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.util.GeckoEventResponder;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONObject;

import android.content.Context;

import java.util.concurrent.ConcurrentLinkedQueue;

public class PromptService implements GeckoEventResponder {
    private static final String LOGTAG = "GeckoPromptService";

    private final ConcurrentLinkedQueue<String> mPromptQueue;
    private final Context mContext;

    public PromptService(Context context) {
        GeckoAppShell.getEventDispatcher().registerEventListener("Prompt:Show", this);
        GeckoAppShell.getEventDispatcher().registerEventListener("Prompt:ShowTop", this);
        mPromptQueue = new ConcurrentLinkedQueue<String>();
        mContext = context;
    }

    public void destroy() {
        GeckoAppShell.getEventDispatcher().unregisterEventListener("Prompt:Show", this);
        GeckoAppShell.getEventDispatcher().unregisterEventListener("Prompt:ShowTop", this);
    }

    public void show(final String aTitle, final String aText, final Prompt.PromptListItem[] aMenuList,
                     final boolean aMultipleSelection, final Prompt.PromptCallback callback) {
        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                Prompt p;
                if (callback != null) {
                    p = new Prompt(mContext, callback);
                } else {
                    p = new Prompt(mContext, mPromptQueue);
                }
                p.show(aTitle, aText, aMenuList, aMultipleSelection);
            }
        });
    }

    
    @Override
    public void handleMessage(String event, final JSONObject message) {
        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                boolean isAsync = message.optBoolean("async");
                Prompt p;
                if (isAsync) {
                    p = new Prompt(mContext, new Prompt.PromptCallback() {
                        public void onPromptFinished(String jsonResult) {
                            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Prompt:Reply", jsonResult));
                        }
                    });
                } else {
                    p = new Prompt(mContext, mPromptQueue);
                }
                p.show(message);
            }
        });
    }

    
    @Override
    public String getResponse(final JSONObject origMessage) {
        if (origMessage.optBoolean("async")) {
            return "";
        }

        
        
        String result;
        while (null == (result = mPromptQueue.poll())) {
            GeckoAppShell.processNextNativeEvent(true);
        }
        return result;
    }
}
