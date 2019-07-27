




package org.mozilla.gecko.prompts;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.CheckedTextView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;

public class Prompt implements OnClickListener, OnCancelListener, OnItemClickListener,
                               PromptInput.OnChangeListener, Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "GeckoPromptService";

    private String[] mButtons;
    private PromptInput[] mInputs;
    private AlertDialog mDialog;

    private final LayoutInflater mInflater;
    private final Context mContext;
    private PromptCallback mCallback;
    private String mGuid;
    private PromptListAdapter mAdapter;

    private static boolean mInitialized;
    private static int mInputPaddingSize;

    private int mTabId = Tabs.INVALID_TAB_ID;

    public Prompt(Context context, PromptCallback callback) {
        this(context);
        mCallback = callback;
    }

    private Prompt(Context context) {
        mContext = context;
        mInflater = LayoutInflater.from(mContext);

        if (!mInitialized) {
            Resources res = mContext.getResources();
            mInputPaddingSize = (int) (res.getDimension(R.dimen.prompt_service_inputs_padding));
            mInitialized = true;
        }
    }

    private View applyInputStyle(View view, PromptInput input) {
        
        if (input.canApplyInputStyle()) {
            view.setPadding(mInputPaddingSize, 0, mInputPaddingSize, 0);
        }
        return view;
    }

    public void show(JSONObject message) {
        String title = message.optString("title");
        String text = message.optString("text");
        mGuid = message.optString("guid");

        mButtons = getStringArray(message, "buttons");

        JSONArray inputs = getSafeArray(message, "inputs");
        mInputs = new PromptInput[inputs.length()];
        for (int i = 0; i < mInputs.length; i++) {
            try {
                mInputs[i] = PromptInput.getInput(inputs.getJSONObject(i));
                mInputs[i].setListener(this);
            } catch(Exception ex) { }
        }

        PromptListItem[] menuitems = PromptListItem.getArray(message.optJSONArray("listitems"));
        String selected = message.optString("choiceMode");

        int choiceMode = ListView.CHOICE_MODE_NONE;
        if ("single".equals(selected)) {
            choiceMode = ListView.CHOICE_MODE_SINGLE;
        } else if ("multiple".equals(selected)) {
            choiceMode = ListView.CHOICE_MODE_MULTIPLE;
        }

        if (message.has("tabId")) {
            mTabId = message.optInt("tabId", Tabs.INVALID_TAB_ID);
        }

        show(title, text, menuitems, choiceMode);
    }

     public void show(String title, String text, PromptListItem[] listItems, int choiceMode) {
        ThreadUtils.assertOnUiThread();

        try {
            create(title, text, listItems, choiceMode);
        } catch(IllegalStateException ex) {
            Log.i(LOGTAG, "Error building dialog", ex);
            return;
        }

        if (mTabId != Tabs.INVALID_TAB_ID) {
            Tabs.registerOnTabsChangedListener(this);

            final Tab tab = Tabs.getInstance().getTab(mTabId);
            if (Tabs.getInstance().getSelectedTab() == tab) {
                mDialog.show();
            }
        } else {
            mDialog.show();
        }
    }

    @Override
    public void onTabChanged(final Tab tab, final Tabs.TabEvents msg, final Object data) {
        if (tab != Tabs.getInstance().getTab(mTabId)) {
            return;
        }

        switch(msg) {
            case SELECTED:
                Log.i(LOGTAG, "Selected");
                mDialog.show();
                break;
            case UNSELECTED:
                Log.i(LOGTAG, "Unselected");
                mDialog.hide();
                break;
            case LOCATION_CHANGE:
                Log.i(LOGTAG, "Location change");
                mDialog.cancel();
                break;
        }
    }

    private void create(String title, String text, PromptListItem[] listItems, int choiceMode)
            throws IllegalStateException {
        final LayerView view = GeckoAppShell.getLayerView();
        if (view != null) {
            view.abortPanning();
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        if (!TextUtils.isEmpty(title)) {
            
            builder.setTitle(title.substring(0, Math.min(title.length(), 256)));
        }

        if (!TextUtils.isEmpty(text)) {
            builder.setMessage(text);
        }

        
        
        if (listItems != null && listItems.length > 0) {
            addListItems(builder, listItems, choiceMode);
        } else if (!addInputs(builder)) {
            throw new IllegalStateException("Could not add inputs to dialog");
        }

        int length = mButtons == null ? 0 : mButtons.length;
        if (length > 0) {
            builder.setPositiveButton(mButtons[0], this);
            if (length > 1) {
                builder.setNeutralButton(mButtons[1], this);
                if (length > 2) {
                    builder.setNegativeButton(mButtons[2], this);
                }
            }
        }

        mDialog = builder.create();
        mDialog.setOnCancelListener(Prompt.this);
    }

    public void setButtons(String[] buttons) {
        mButtons = buttons;
    }

    public void setInputs(PromptInput[] inputs) {
        mInputs = inputs;
    }

    




    private void addListResult(final JSONObject result, int which) {
        if (mAdapter == null) {
            return;
        }

        try {
            JSONArray selected = new JSONArray();

            
            ArrayList<Integer> selectedItems = mAdapter.getSelected();
            for (Integer item : selectedItems) {
                selected.put(item);
            }

            
            
            if (!result.has("button") || result.optInt("button") == -1) {
                if (!selectedItems.contains(which)) {
                    selected.put(which);
                }

                result.put("button", which);
            }

            result.put("list", selected);
        } catch(JSONException ex) { }
    }

    


    private void addInputValues(final JSONObject result) {
        try {
            if (mInputs != null) {
                for (int i = 0; i < mInputs.length; i++) {
                    result.put(mInputs[i].getId(), mInputs[i].getValue());
                }
            }
        } catch(JSONException ex) { }
    }

    



    private void addButtonResult(final JSONObject result, int which) {
        int button = -1;
        switch(which) {
            case DialogInterface.BUTTON_POSITIVE : button = 0; break;
            case DialogInterface.BUTTON_NEUTRAL  : button = 1; break;
            case DialogInterface.BUTTON_NEGATIVE : button = 2; break;
        }
        try {
            result.put("button", button);
        } catch(JSONException ex) { }
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        ThreadUtils.assertOnUiThread();
        closeDialog(which);
    }

    









    private void addListItems(AlertDialog.Builder builder, PromptListItem[] listItems, int choiceMode) {
        switch(choiceMode) {
            case ListView.CHOICE_MODE_MULTIPLE_MODAL:
            case ListView.CHOICE_MODE_MULTIPLE:
                addMultiSelectList(builder, listItems);
                break;
            case ListView.CHOICE_MODE_SINGLE:
                addSingleSelectList(builder, listItems);
                break;
            case ListView.CHOICE_MODE_NONE:
            default:
                addMenuList(builder, listItems);
        }
    }

    








    private void addMultiSelectList(AlertDialog.Builder builder, PromptListItem[] listItems) {
        ListView listView = (ListView) mInflater.inflate(R.layout.select_dialog_list, null);
        listView.setOnItemClickListener(this);
        listView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);

        mAdapter = new PromptListAdapter(mContext, R.layout.select_dialog_multichoice, listItems);
        listView.setAdapter(mAdapter);
        builder.setView(listView);
    }

    






    private void addSingleSelectList(AlertDialog.Builder builder, PromptListItem[] listItems) {
        mAdapter = new PromptListAdapter(mContext, R.layout.select_dialog_singlechoice, listItems);
        builder.setSingleChoiceItems(mAdapter, mAdapter.getSelectedIndex(), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                
                
                ArrayList<Integer> selected = mAdapter.getSelected();
                for (Integer sel : selected) {
                    mAdapter.toggleSelected(sel);
                }

                
                mAdapter.toggleSelected(which);
                closeIfNoButtons(which);
            }
        });
    }

    






    private void addMenuList(AlertDialog.Builder builder, PromptListItem[] listItems) {
        mAdapter = new PromptListAdapter(mContext, android.R.layout.simple_list_item_1, listItems);
        builder.setAdapter(mAdapter, this);
    }

    


    private View wrapInput(final PromptInput input) {
        final LinearLayout linearLayout = new LinearLayout(mContext);
        linearLayout.setOrientation(LinearLayout.VERTICAL);
        applyInputStyle(linearLayout, input);

        linearLayout.addView(input.getView(mContext));

        return linearLayout;
    }

    







    private boolean addInputs(AlertDialog.Builder builder) {
        int length = mInputs == null ? 0 : mInputs.length;
        if (length == 0) {
            return true;
        }

        try {
            View root = null;
            boolean scrollable = false; 

            if (length == 1) {
                root = wrapInput(mInputs[0]);
                scrollable |= mInputs[0].getScrollable();
            } else if (length > 1) {
                LinearLayout linearLayout = new LinearLayout(mContext);
                linearLayout.setOrientation(LinearLayout.VERTICAL);
                for (int i = 0; i < length; i++) {
                    View content = wrapInput(mInputs[i]);
                    linearLayout.addView(content);
                    scrollable |= mInputs[i].getScrollable();
                }
                root = linearLayout;
            }

            if (scrollable) {
                
                builder.setInverseBackgroundForced(true);
                builder.setView(root);
            } else {
                ScrollView view = new ScrollView(mContext);
                view.addView(root);
                builder.setView(view);
            }
        } catch(Exception ex) {
            Log.e(LOGTAG, "Error showing prompt inputs", ex);
            
            
            cancelDialog();
            return false;
        }

        return true;
    }

    


    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        ThreadUtils.assertOnUiThread();
        mAdapter.toggleSelected(position);

        
        
        
        closeIfNoButtons(position);
    }

    private boolean closeIfNoButtons(int selected) {
        ThreadUtils.assertOnUiThread();
        if (mButtons == null || mButtons.length == 0) {
            closeDialog(selected);
            return true;
        }
        return false;
    }

    






    @Override
    public void onCancel(DialogInterface aDialog) {
        ThreadUtils.assertOnUiThread();
        cancelDialog();
    }

    


    private void cancelDialog() {
        JSONObject ret = new JSONObject();
        try {
            ret.put("button", -1);
        } catch(Exception ex) { }
        addInputValues(ret);
        notifyClosing(ret);
    }

    


    private void closeDialog(int which) {
        JSONObject ret = new JSONObject();
        mDialog.dismiss();

        addButtonResult(ret, which);
        addListResult(ret, which);
        addInputValues(ret);

        notifyClosing(ret);
    }

    


    private void notifyClosing(JSONObject aReturn) {
        try {
            aReturn.put("guid", mGuid);
        } catch(JSONException ex) { }

        if (mTabId != Tabs.INVALID_TAB_ID) {
            Tabs.unregisterOnTabsChangedListener(this);
        }

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createNoOpEvent());

        if (mCallback != null) {
            mCallback.onPromptFinished(aReturn.toString());
        }
    }

    
    @Override
    public void onChange(PromptInput input) {
        
        
        
        closeIfNoButtons(-1);
    }

    private static JSONArray getSafeArray(JSONObject json, String key) {
        try {
            return json.getJSONArray(key);
        } catch (Exception e) {
            return new JSONArray();
        }
    }

    public static String[] getStringArray(JSONObject aObject, String aName) {
        JSONArray items = getSafeArray(aObject, aName);
        int length = items.length();
        String[] list = new String[length];
        for (int i = 0; i < length; i++) {
            try {
                list[i] = items.getString(i);
            } catch(Exception ex) { }
        }
        return list;
    }

    private static boolean[] getBooleanArray(JSONObject aObject, String aName) {
        JSONArray items = new JSONArray();
        try {
            items = aObject.getJSONArray(aName);
        } catch(Exception ex) { return null; }
        int length = items.length();
        boolean[] list = new boolean[length];
        for (int i = 0; i < length; i++) {
            try {
                list[i] = items.getBoolean(i);
            } catch(Exception ex) { }
        }
        return list;
    }

    public interface PromptCallback {
        public void onPromptFinished(String jsonResult);
    }
}
