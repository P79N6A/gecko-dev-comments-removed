




package org.mozilla.gecko.prompts;

import org.mozilla.gecko.util.GeckoEventResponder;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.DateTimePicker;
import org.mozilla.gecko.R;
import org.mozilla.gecko.GeckoAppShell;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.Html;
import android.text.InputType;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.inputmethod.InputMethodManager;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CheckedTextView;
import android.widget.DatePicker;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.TimePicker;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.TimeUnit;

public class Prompt implements OnClickListener, OnCancelListener, OnItemClickListener {
    private static final String LOGTAG = "GeckoPromptService";

    private String[] mButtons;
    private PromptInput[] mInputs;
    private boolean[] mSelected;
    private AlertDialog mDialog;

    private final LayoutInflater mInflater;
    private ConcurrentLinkedQueue<String> mPromptQueue;
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

    public Prompt(Context context, ConcurrentLinkedQueue<String> queue) {
        this(context);
        mCallback = null;
        mPromptQueue = queue;
    }

    public Prompt(Context context, PromptCallback callback) {
        this(context);
        mCallback = callback;
        mPromptQueue = null;
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

    private View applyInputStyle(View view) {
        view.setPadding(mInputPaddingSize, 0, mInputPaddingSize, 0);
        return view;
    }

    public void show(JSONObject message) {
        processMessage(message);
    }

    public void show(String title, String text, PromptListItem[] listItems, boolean multipleSelection) {
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
            addlistItems(builder, listItems, multipleSelection);
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
            if (mSelected != null) {
                JSONArray selected = new JSONArray();
                for (int i = 0; i < mSelected.length; i++) {
                    selected.put(mSelected[i]);
                }
                result.put("button", selected);
            } else {
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
            if (list != null || mSelected != null) {
                addListResult(ret, which);
            } else {
                addButtonResult(ret, which);
            }
            addInputValues(ret);
        } catch(Exception ex) {
            Log.i(LOGTAG, "Error building return: " + ex);
        }

        if (dialog != null) {
            dialog.dismiss();
        }

        finishDialog(ret);
    }

    











    private void addlistItems(AlertDialog.Builder builder, PromptListItem[] listItems, boolean multipleSelection) {
        if (mSelected != null && mSelected.length > 0) {
            if (multipleSelection) {
                addMultiSelectList(builder, listItems);
            } else {
                addSingleSelectList(builder, listItems);
            }
        } else {
            addMenuList(builder, listItems);
        }
    }

    








    private void addMultiSelectList(AlertDialog.Builder builder, PromptListItem[] listItems) {
        PromptListAdapter adapter = new PromptListAdapter(mContext, R.layout.select_dialog_multichoice, listItems);
        adapter.listView = (ListView) mInflater.inflate(R.layout.select_dialog_list, null);
        adapter.listView.setOnItemClickListener(this);
        builder.setInverseBackgroundForced(true);
        adapter.listView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
        adapter.listView.setAdapter(adapter);
        builder.setView(adapter.listView);
    }

    






    private void addSingleSelectList(AlertDialog.Builder builder, PromptListItem[] listItems) {
        PromptListAdapter adapter = new PromptListAdapter(mContext, R.layout.select_dialog_singlechoice, listItems);
        
        int selectedIndex = -1;
        for (int i = 0; i < mSelected.length; i++) {
            if (mSelected[i]) {
                selectedIndex = i;
                break;
            }
        }
        mSelected = null;

        builder.setSingleChoiceItems(adapter, selectedIndex, this);
    }

    






    private void addMenuList(AlertDialog.Builder builder, PromptListItem[] listItems) {
        PromptListAdapter adapter = new PromptListAdapter(mContext, android.R.layout.simple_list_item_1, listItems);
        builder.setAdapter(adapter, this);
        mSelected = null;
    }

    







    private boolean addInputs(AlertDialog.Builder builder) {
        int length = mInputs == null ? 0 : mInputs.length;
        if (length == 0) {
            return true;
        }

        try {
            if (length == 1) {
                ScrollView view = new ScrollView(mContext);
                view.addView(mInputs[0].getView(mContext));
                builder.setView(applyInputStyle(view));
            } else if (length > 1) {
                LinearLayout linearLayout = new LinearLayout(mContext);
                linearLayout.setOrientation(LinearLayout.VERTICAL);
                for (int i = 0; i < length; i++) {
                    View content = mInputs[i].getView(mContext);
                    linearLayout.addView(content);
                }
                ScrollView view = new ScrollView(mContext);
                view.addView(linearLayout);
                builder.setView(applyInputStyle(view));
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
        mSelected[position] = !mSelected[position];
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
        mSelected = null;
        try {
            aReturn.put("guid", mGuid);
        } catch(JSONException ex) { }

        if (mPromptQueue != null) {
            mPromptQueue.offer(aReturn.toString());
        }

        
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

        PromptListItem[] menuitems = getListItemArray(geckoObject, "listitems");
        mSelected = getBooleanArray(geckoObject, "selected");
        boolean multiple = geckoObject.optBoolean("multiple");
        show(title, text, menuitems, multiple);
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

    private PromptListItem[] getListItemArray(JSONObject aObject, String aName) {
        JSONArray items = getSafeArray(aObject, aName);
        int length = items.length();
        PromptListItem[] list = new PromptListItem[length];
        for (int i = 0; i < length; i++) {
            try {
                list[i] = new PromptListItem(items.getJSONObject(i));
            } catch(Exception ex) { }
        }
        return list;
    }

    public static class PromptListItem {
        public final String label;
        public final boolean isGroup;
        public final boolean inGroup;
        public final boolean disabled;
        public final int id;
        public final boolean isParent;

        
        public Drawable icon;

        PromptListItem(JSONObject aObject) {
            label = aObject.optString("label");
            isGroup = aObject.optBoolean("isGroup");
            inGroup = aObject.optBoolean("inGroup");
            disabled = aObject.optBoolean("disabled");
            id = aObject.optInt("id");
            isParent = aObject.optBoolean("isParent");
        }

        public PromptListItem(String aLabel) {
            label = aLabel;
            isGroup = false;
            inGroup = false;
            disabled = false;
            id = 0;
            isParent = false;
        }
    }

    public interface PromptCallback {
        public void onPromptFinished(String jsonResult);
    }

    public class PromptListAdapter extends ArrayAdapter<PromptListItem> {
        private static final int VIEW_TYPE_ITEM = 0;
        private static final int VIEW_TYPE_GROUP = 1;
        private static final int VIEW_TYPE_COUNT = 2;

        public ListView listView;
        private int mResourceId = -1;
        private Drawable mBlankDrawable = null;
        private Drawable mMoreDrawable = null;

        PromptListAdapter(Context context, int textViewResourceId, PromptListItem[] objects) {
            super(context, textViewResourceId, objects);
            mResourceId = textViewResourceId;
        }

        @Override
        public int getItemViewType(int position) {
            PromptListItem item = getItem(position);
            return (item.isGroup ? VIEW_TYPE_GROUP : VIEW_TYPE_ITEM);
        }

        @Override
        public int getViewTypeCount() {
            return VIEW_TYPE_COUNT;
        }

        private Drawable getMoreDrawable(Resources res) {
            if (mMoreDrawable == null) {
                mMoreDrawable = res.getDrawable(android.R.drawable.ic_menu_more);
            }
            return mMoreDrawable;
        }

        private Drawable getBlankDrawable(Resources res) {
            if (mBlankDrawable == null) {
                mBlankDrawable = res.getDrawable(R.drawable.blank);
            }
            return mBlankDrawable;
        }

        private void maybeUpdateIcon(PromptListItem item, TextView t) {
            if (item.icon == null && !item.inGroup && !item.isParent) {
                t.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
                return;
            }

            Drawable d = null;
            Resources res = mContext.getResources();
            
            t.setCompoundDrawablePadding(mIconTextPadding);
            if (item.icon != null) {
                
                
                Bitmap bitmap = ((BitmapDrawable) item.icon).getBitmap();
                d = new BitmapDrawable(Bitmap.createScaledBitmap(bitmap, mIconSize, mIconSize, true));
            } else if (item.inGroup) {
                
                d = getBlankDrawable(res);
            }

            Drawable moreDrawable = null;
            if (item.isParent) {
                moreDrawable = getMoreDrawable(res);
            }

            if (d != null || moreDrawable != null) {
                t.setCompoundDrawablesWithIntrinsicBounds(d, null, moreDrawable, null);
            }
        }

        private void maybeUpdateCheckedState(int position, PromptListItem item, ViewHolder viewHolder) {
            viewHolder.textView.setEnabled(!item.disabled && !item.isGroup);
            viewHolder.textView.setClickable(item.isGroup || item.disabled);

            if (mSelected == null) {
                return;
            }

            CheckedTextView ct;
            try {
                ct = (CheckedTextView) viewHolder.textView;
                
                
                
                if (listView != null) {
                    listView.setItemChecked(position, mSelected[position]);
                }
            } catch (Exception e) {
                return;
            }

        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            PromptListItem item = getItem(position);
            ViewHolder viewHolder = null;

            if (convertView == null) {
                int resourceId = mResourceId;
                if (item.isGroup) {
                    resourceId = R.layout.list_item_header;
                }

                convertView = mInflater.inflate(resourceId, null);
                convertView.setMinimumHeight(mMinRowSize);

                TextView tv = (TextView) convertView.findViewById(android.R.id.text1);
                viewHolder = new ViewHolder(tv, tv.getPaddingLeft(), tv.getPaddingRight(),
                                            tv.getPaddingTop(), tv.getPaddingBottom());

                convertView.setTag(viewHolder);
            } else {
                viewHolder = (ViewHolder) convertView.getTag();
            }

            viewHolder.textView.setText(item.label);
            maybeUpdateCheckedState(position, item, viewHolder);
            maybeUpdateIcon(item, viewHolder.textView);

            return convertView;
        }

        private class ViewHolder {
            public final TextView textView;
            public final int paddingLeft;
            public final int paddingRight;
            public final int paddingTop;
            public final int paddingBottom;

            ViewHolder(TextView aTextView, int aLeft, int aRight, int aTop, int aBottom) {
                textView = aTextView;
                paddingLeft = aLeft;
                paddingRight = aRight;
                paddingTop = aTop;
                paddingBottom = aBottom;
            }
        }
    }
}
