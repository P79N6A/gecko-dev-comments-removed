



package org.mozilla.gecko.overlays.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.R;
import org.mozilla.gecko.overlays.service.sharemethods.ParcelableClientRecord;

import java.util.Collection;

import static org.mozilla.gecko.overlays.ui.SendTabList.*;

public class SendTabDeviceListArrayAdapter extends ArrayAdapter<ParcelableClientRecord> {
    private static final String LOGTAG = "GeckoSendTabAdapter";

    private State currentState;

    
    
    private String dummyRecordName;

    private final SendTabTargetSelectedListener listener;

    private Collection<ParcelableClientRecord> records;

    
    
    private AlertDialog dialog;

    public SendTabDeviceListArrayAdapter(Context context, SendTabTargetSelectedListener aListener) {
        super(context, R.layout.overlay_share_send_tab_item);

        listener = aListener;

        
        setNotifyOnChange(false);
    }

    



    public ParcelableClientRecord[] toArray() {
        return records.toArray(new ParcelableClientRecord[records.size()]);
    }

    public void setClientRecordList(Collection<ParcelableClientRecord> clientRecordList) {
        records = clientRecordList;
        updateRecordList();
    }

    



    public void updateRecordList() {
        if (currentState != State.LIST) {
            return;
        }

        clear();

        setNotifyOnChange(false);    
        if (AppConstants.Versions.feature11Plus) {
             addAll(records);
        } else {
            for (ParcelableClientRecord record : records) {
                add(record);
            }
        }

        notifyDataSetChanged();
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        final Context context = getContext();

        
        TextView row = (TextView) convertView;
        if (row == null) {
            row = (TextView) View.inflate(context, R.layout.overlay_share_send_tab_item, null);
        }

        if (currentState != State.LIST) {
            
            row.setText(dummyRecordName);
            row.setCompoundDrawablesWithIntrinsicBounds(R.drawable.overlay_send_tab_icon, 0, 0, 0);
        }

        
        if (currentState == State.SHOW_DEVICES) {
            row.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    dialog.show();
                }
            });

            return row;
        }

        
        final String listenerGUID;

        ParcelableClientRecord clientRecord = getItem(position);
        if (currentState == State.LIST) {
            row.setText(clientRecord.name);
            row.setCompoundDrawablesWithIntrinsicBounds(getImage(clientRecord), 0, 0, 0);

            listenerGUID = clientRecord.guid;
        } else {
            listenerGUID = null;
        }

        row.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                listener.onSendTabTargetSelected(listenerGUID);
            }
        });

        return row;
    }

    private static int getImage(ParcelableClientRecord record) {
        if ("mobile".equals(record.type)) {
            return R.drawable.sync_mobile;
        }

        return R.drawable.sync_desktop;
    }

    public void switchState(State newState) {
        if (currentState == newState) {
            return;
        }

        currentState = newState;

        switch (newState) {
            case LIST:
                updateRecordList();
                break;
            case NONE:
                showDummyRecord(getContext().getResources().getString(R.string.overlay_share_send_tab_btn_label));
                break;
            case SHOW_DEVICES:
                showDummyRecord(getContext().getResources().getString(R.string.overlay_share_send_other));
                break;
            default:
                Assert.isTrue(false, "Unexpected state transition: " + newState);
        }
    }

    


    private void showDummyRecord(String name) {
        dummyRecordName = name;
        clear();
        add(null);
        notifyDataSetChanged();
    }

    public void setDialog(AlertDialog aDialog) {
        dialog = aDialog;
    }
}
