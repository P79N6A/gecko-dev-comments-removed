




package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.List;

import org.mozilla.gecko.db.RemoteClient;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.util.SparseBooleanArray;











public class RemoteClientsDialogFragment extends DialogFragment {
    private static final String KEY_TITLE = "title";
    private static final String KEY_CHOICE_MODE = "choice_mode";
    private static final String KEY_POSITIVE_BUTTON_TEXT = "positive_button_text";
    private static final String KEY_CLIENTS = "clients";

    public interface RemoteClientsListener {
        
        public void onClients(List<RemoteClient> clients);
    }

    public enum ChoiceMode {
        SINGLE,
        MULTIPLE,
    }

    public static RemoteClientsDialogFragment newInstance(String title, String positiveButtonText, ChoiceMode choiceMode, ArrayList<RemoteClient> clients) {
        final RemoteClientsDialogFragment dialog = new RemoteClientsDialogFragment();
        final Bundle args = new Bundle();
        args.putString(KEY_TITLE, title);
        args.putString(KEY_POSITIVE_BUTTON_TEXT, positiveButtonText);
        args.putInt(KEY_CHOICE_MODE, choiceMode.ordinal());
        args.putParcelableArrayList(KEY_CLIENTS, clients);
        dialog.setArguments(args);
        return dialog;
    }

    public RemoteClientsDialogFragment() {
        
    }

    protected void notifyListener(List<RemoteClient> clients) {
        RemoteClientsListener listener;
        try {
            listener = (RemoteClientsListener) getTargetFragment();
        } catch (ClassCastException e) {
            try {
                listener = (RemoteClientsListener) getActivity();
            } catch (ClassCastException f) {
                throw new ClassCastException(getTargetFragment() + " or " + getActivity()
                        + " must implement RemoteClientsListener");
            }
        }
        listener.onClients(clients);
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final String title = getArguments().getString(KEY_TITLE);
        final String positiveButtonText = getArguments().getString(KEY_POSITIVE_BUTTON_TEXT);
        final ChoiceMode choiceMode = ChoiceMode.values()[getArguments().getInt(KEY_CHOICE_MODE)];
        final ArrayList<RemoteClient> clients = getArguments().getParcelableArrayList(KEY_CLIENTS);

        final Builder builder = new AlertDialog.Builder(getActivity());
        builder.setTitle(title);

        final String[] clientNames = new String[clients.size()];
        for (int i = 0; i < clients.size(); i++) {
            clientNames[i] = clients.get(i).name;
        }

        if (choiceMode == ChoiceMode.MULTIPLE) {
            builder.setMultiChoiceItems(clientNames, null, null);
            builder.setPositiveButton(positiveButtonText, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int which) {
                    if (which != Dialog.BUTTON_POSITIVE) {
                        return;
                    }

                    final AlertDialog dialog = (AlertDialog) dialogInterface;
                    final SparseBooleanArray checkedItemPositions = dialog.getListView().getCheckedItemPositions();
                    final ArrayList<RemoteClient> checked = new ArrayList<RemoteClient>();
                    for (int i = 0; i < clients.size(); i++) {
                        if (checkedItemPositions.get(i)) {
                            checked.add(clients.get(i));
                        }
                    }
                    notifyListener(checked);
                }
            });
        } else {
            builder.setItems(clientNames, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int index) {
                    final ArrayList<RemoteClient> checked = new ArrayList<RemoteClient>();
                    checked.add(clients.get(index));
                    notifyListener(checked);
                }
            });
        }

        return builder.create();
    }
}
