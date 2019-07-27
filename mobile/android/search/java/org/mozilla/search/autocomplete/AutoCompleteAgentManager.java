



package org.mozilla.search.autocomplete;

import android.app.Activity;
import android.database.Cursor;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import java.util.ArrayList;







class AutoCompleteAgentManager {

    private final Handler mainUiHandler;
    private final Handler localHandler;
    private final AutoCompleteWordListAgent autoCompleteWordListAgent;

    public AutoCompleteAgentManager(Activity activity, Handler mainUiHandler) {
        HandlerThread thread = new HandlerThread("org.mozilla.search.autocomplete.SuggestionAgent");
        
        thread.start();
        Log.i("AUTOCOMPLETE", "Starting thread");
        this.mainUiHandler = mainUiHandler;
        localHandler = new SuggestionMessageHandler(thread.getLooper());
        autoCompleteWordListAgent = new AutoCompleteWordListAgent(activity);
    }

    


    public void search(String queryString) {
        
        localHandler.sendMessage(localHandler.obtainMessage(0, queryString));
    }

    




    private class SuggestionMessageHandler extends Handler {

        private SuggestionMessageHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (null == msg.obj) {
                return;
            }

            Cursor cursor =
                    autoCompleteWordListAgent.getWordMatches(((String) msg.obj).toLowerCase());
            ArrayList<AutoCompleteModel> res = new ArrayList<AutoCompleteModel>();

            if (null == cursor) {
                return;
            }

            for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                res.add(new AutoCompleteModel(cursor.getString(
                        cursor.getColumnIndex(AutoCompleteWordListAgent.COL_WORD))));
            }


            mainUiHandler.sendMessage(Message.obtain(mainUiHandler, 0, res));
        }

    }

}
