



package org.mozilla.gecko.overlays.ui;

import static org.mozilla.gecko.overlays.ui.SendTabList.State.LOADING;
import static org.mozilla.gecko.overlays.ui.SendTabList.State.SHOW_DEVICES;

import java.util.Arrays;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.RemoteClient;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.AttributeSet;
import android.widget.ListAdapter;
import android.widget.ListView;





















public class SendTabList extends ListView {
    @SuppressWarnings("unused")
    private static final String LOGTAG = "GeckoSendTabList";

    
    
    public static final int MAXIMUM_INLINE_ELEMENTS = 2;

    private SendTabDeviceListArrayAdapter clientListAdapter;

    
    private SendTabTargetSelectedListener listener;

    private final State currentState = LOADING;

    


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

    public void setSyncClients(final RemoteClient[] c) {
        final RemoteClient[] clients = c == null ? new RemoteClient[0] : c;

        clientListAdapter.setRemoteClientsList(Arrays.asList(clients));
    }

    




    public AlertDialog getDialog() {
        final Context context = getContext();

        final AlertDialog.Builder builder;
        if (Versions.feature11Plus) {
            builder = new AlertDialog.Builder(context, R.style.Gecko_Dialog);
        } else {
            builder = new AlertDialog.Builder(context);
        }

        final RemoteClient[] records = clientListAdapter.toArray();
        final String[] dialogElements = new String[records.length];

        for (int i = 0; i < records.length; i++) {
            dialogElements[i] = records[i].name;
        }

        builder.setTitle(R.string.overlay_share_select_device)
               .setItems(dialogElements, new DialogInterface.OnClickListener() {
                   @Override
                   public void onClick(DialogInterface dialog, int index) {
                       listener.onSendTabTargetSelected(records[index].guid);
                   }
                })
               .setOnCancelListener(new DialogInterface.OnCancelListener() {
                   @Override
                   public void onCancel(DialogInterface dialogInterface) {
                       Telemetry.sendUIEvent(TelemetryContract.Event.CANCEL, TelemetryContract.Method.SHARE_OVERLAY, "device_selection_cancel");
                   }
               });

        return builder.create();
    }
}
