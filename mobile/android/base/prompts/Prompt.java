




package org.mozilla.gecko.prompts;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.util.ThreadUtils;

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

public class Prompt implements OnClickListener, OnCancelListener, OnItemClickListener {
    private static final String LOGTAG = "GeckoPromptService";

    private String[] mButtons;
    private PromptInput[] mInputs;
    private AlertDialog mDialog;

    private final LayoutInflater mInflater;
    private final Context mContext;
    private PromptCallback mCallback;
    private String mGuid;

    private static boolean mInitialized = false;
    private static int mGroupPaddingSize;
    private static int mLeftRightTextWithIconPadding;
    private static int mTopBottomTextWithIconPadding;
    private static int mIconTextPadding;
    private static int mIconSize;
    private static int mInputPaddingSize;
    private static int mMinRowSize;
    private PromptListAdapter mAdapter;

    public Prompt(Context context, PromptCallback callback) {
        this(context);
        mCallback = callback;
    }

    private Prompt(Context context) {
        mContext = context;
        mInflater = LayoutInflater.from(mContext);

        if (!mInitialized) {
            Resources res = mContext.getResources();
            mGroupPaddingSize = (int) (res.getDimension(R.dimen.prompt_service_group_padding_size));
            mLeftRightTextWithIconPadding = (int) (res.getDimension(R.dimen.prompt_service_left_right_text_with_icon_padding));
            mTopBottomTextWithIconPadding = (int) (res.getDimension(R.dimen.prompt_service_top_bottom_text_with_icon_padding));
            mIconTextPadding = (int) (res.getDimension(R.dimen.prompt_service_icon_text_padding));
            mIconSize = (int) (res.getDimension(R.dimen.prompt_service_icon_size));
            mInputPaddingSize = (int) (res.getDimension(R.dimen.prompt_service_inputs_padding));
            mMinRowSize = (int) (res.getDimension(R.dimen.prompt_service_min_list_item_height));
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
        processMessage(message);
    }


    public void show(String title, String text, PromptListItem[] listItems, boolean multipleSelection) {
        show(title, text, listItems, multipleSelection, null);
    }

    public void show(String title, String text, PromptListItem[] listItems, boolean multipleSelection, boolean[] selected) {
        ThreadUtils.assertOnUiThread();

        GeckoAppShell.getLayerView().abortPanning();

        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        if (!TextUtils.isEmpty(title)) {
            builder.setTitle(title);
        }

        if (!TextUtils.isEmpty(text)) {
            builder.setMessage(text);
        }

        
        
        if (listItems != null && listItems.length > 0) {
            addListItems(builder, listItems, multipleSelection, selected);
        } else if (!addInputs(builder)) {
            
            return;
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
        mDialog.show();
    }

    public void setButtons(String[] buttons) {
        mButtons = buttons;
    }

    public void setInputs(PromptInput[] inputs) {
        mInputs = inputs;
    }

    




    private void addListResult(final JSONObject result, int which) {
        try {
            boolean[] selectedItems = mAdapter.getSelected();
            if (selectedItems != null) {
                JSONArray selected = new JSONArray();
                for (int i = 0; i < selectedItems.length; i++) {
                    if (selectedItems[i]) {
                        selected.put(i);
                    }
                }
                result.put("list", selected);
            } else {
                
                JSONArray selected = new JSONArray();
                selected.put(which);
                result.put("list", selected);
                
                result.put("button", which);
            }
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
        JSONObject ret = new JSONObject();
        try {
            ListView list = mDialog.getListView();
            addButtonResult(ret, which);
            addInputValues(ret);

            if (list != null || mAdapter.getSelected() != null) {
                addListResult(ret, which);
            }
        } catch(Exception ex) {
            Log.i(LOGTAG, "Error building return: " + ex);
        }

        if (dialog != null) {
            dialog.dismiss();
        }

        finishDialog(ret);
    }

    













    private void addListItems(AlertDialog.Builder builder, PromptListItem[] listItems, boolean multipleSelection, boolean[] selected) {
        if (selected != null && selected.length > 0) {
            if (multipleSelection) {
                addMultiSelectList(builder, listItems, selected);
            } else {
                addSingleSelectList(builder, listItems, selected);
            }
        } else {
            addMenuList(builder, listItems);
        }
    }

    








    private void addMultiSelectList(AlertDialog.Builder builder, PromptListItem[] listItems, boolean[] selected) {
        mAdapter = new PromptListAdapter(mContext, R.layout.select_dialog_multichoice, listItems);
        mAdapter.setSelected(selected);

        ListView listView = (ListView) mInflater.inflate(R.layout.select_dialog_list, null);
        listView.setOnItemClickListener(this);
        listView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
        listView.setAdapter(mAdapter);

        builder.setInverseBackgroundForced(true);
        builder.setView(listView);
    }

    






    private void addSingleSelectList(AlertDialog.Builder builder, PromptListItem[] listItems, boolean[] selected) {
        mAdapter = new PromptListAdapter(mContext, R.layout.select_dialog_singlechoice, listItems);
        
        int selectedIndex = -1;
        for (int i = 0; i < selected.length; i++) {
            if (selected[i]) {
                selectedIndex = i;
                break;
            }
        }

        builder.setSingleChoiceItems(mAdapter, selectedIndex, this);
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
        finishDialog(ret);
    }

    


    public void finishDialog(JSONObject aReturn) {
        mInputs = null;
        mButtons = null;
        mDialog = null;
        try {
            aReturn.put("guid", mGuid);
        } catch(JSONException ex) { }

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createNoOpEvent());

        if (mCallback != null) {
            mCallback.onPromptFinished(aReturn.toString());
        }
        mGuid = null;
    }

    

    private void processMessage(JSONObject geckoObject) {
        String title = geckoObject.optString("title");
        String text = geckoObject.optString("text");
        mGuid = geckoObject.optString("guid");

        mButtons = getStringArray(geckoObject, "buttons");

        JSONArray inputs = getSafeArray(geckoObject, "inputs");
        mInputs = new PromptInput[inputs.length()];
        for (int i = 0; i < mInputs.length; i++) {
            try {
                mInputs[i] = PromptInput.getInput(inputs.getJSONObject(i));
            } catch(Exception ex) { }
        }

        PromptListItem[] menuitems = PromptListItem.getArray(geckoObject.optJSONArray("listitems"));
        boolean multiple = geckoObject.optBoolean("multiple");
        show(title, text, menuitems, multiple, getBooleanArray(geckoObject, "selected"));
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
