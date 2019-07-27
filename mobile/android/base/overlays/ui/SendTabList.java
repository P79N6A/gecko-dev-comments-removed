



package org.mozilla.gecko.overlays.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.ListAdapter;
import android.widget.ListView;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.R;
import org.mozilla.gecko.overlays.service.sharemethods.ParcelableClientRecord;

import java.util.Arrays;

import static org.mozilla.gecko.overlays.ui.SendTabList.State.LIST;
import static org.mozilla.gecko.overlays.ui.SendTabList.State.LOADING;
import static org.mozilla.gecko.overlays.ui.SendTabList.State.NONE;
import static org.mozilla.gecko.overlays.ui.SendTabList.State.SHOW_DEVICES;





















public class SendTabList extends ListView {
    private static final String LOGTAG = "SendTabList";

    
    
    public static final int MAXIMUM_INLINE_ELEMENTS = 2;

    private SendTabDeviceListArrayAdapter clientListAdapter;

    
    private SendTabTargetSelectedListener listener;

    private State currentState = LOADING;

    


    public enum State {
        
        
        NONE,

        
        
        LOADING,

        
        LIST,

        
        SHOW_DEVICES
    }

    public SendTabList(Context context) {
        super(context);
    }

    public SendTabList(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void setAdapter(ListAdapter adapter) {
        Assert.isTrue(adapter instanceof SendTabDeviceListArrayAdapter);

        clientListAdapter = (SendTabDeviceListArrayAdapter) adapter;
        super.setAdapter(adapter);
    }

    public void setSendTabTargetSelectedListener(SendTabTargetSelectedListener aListener) {
        listener = aListener;
    }

    public void switchState(State state) {
        if (state == currentState) {
            return;
        }

        clientListAdapter.switchState(state);
        if (state == SHOW_DEVICES) {
            clientListAdapter.setDialog(getDialog());
        }
    }

    public void setSyncClients(ParcelableClientRecord[] clients) {
        if (clients == null) {
            clients = new ParcelableClientRecord[0];
        }

        int size = clients.length;
        if (size == 0) {
            
            switchState(NONE);
            return;
        }

        clientListAdapter.setClientRecordList(Arrays.asList(clients));

        if (size <= MAXIMUM_INLINE_ELEMENTS) {
            
            switchState(LIST);
            return;
        }

        
        switchState(SHOW_DEVICES);
    }

    




    public AlertDialog getDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());

        final ParcelableClientRecord[] records = clientListAdapter.toArray();
        final String[] dialogElements = new String[records.length];

        for (int i = 0; i < records.length; i++) {
            dialogElements[i] = records[i].name;
        }

        builder.setTitle(R.string.overlay_share_select_device)
               .setItems(dialogElements, new DialogInterface.OnClickListener() {
                   public void onClick(DialogInterface dialog, int index) {
                       listener.onSendTabTargetSelected(records[index].guid);
                   }
               });

        return builder.create();
    }

    


    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (ev.getAction() == MotionEvent.ACTION_MOVE) {
            return true;
        }

        return super.dispatchTouchEvent(ev);
    }
}
