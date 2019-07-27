




package org.mozilla.gecko.prompts;

import java.util.LinkedHashMap;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.R;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TabHost;
import android.widget.TextView;

public class TabInput extends PromptInput implements AdapterView.OnItemClickListener {
    public static final String INPUT_TYPE = "tabs";
    public static final String LOGTAG = "GeckoTabInput";

    
    final private LinkedHashMap<String, PromptListItem[]> mTabs;

    private TabHost mHost;
    private int mPosition;

    public TabInput(JSONObject obj) {
        super(obj);
        mTabs = new LinkedHashMap<String, PromptListItem[]>();
        try {
            JSONArray tabs = obj.getJSONArray("items");
            for (int i = 0; i < tabs.length(); i++) {
                JSONObject tab = tabs.getJSONObject(i);
                String title = tab.getString("label");
                JSONArray items = tab.getJSONArray("items");
                mTabs.put(title, PromptListItem.getArray(items));
            }
        } catch(JSONException ex) {
            Log.e(LOGTAG, "Exception", ex);
        }
    }

    @Override
    public View getView(final Context context) throws UnsupportedOperationException {
        final LayoutInflater inflater = LayoutInflater.from(context);
        mHost = (TabHost) inflater.inflate(R.layout.tab_prompt_input, null);
        mHost.setup();

        for (String title : mTabs.keySet()) {
            final TabHost.TabSpec spec = mHost.newTabSpec(title);
            spec.setContent(new TabHost.TabContentFactory() {
                @Override
                public View createTabContent(final String tag) {
                    PromptListAdapter adapter = new PromptListAdapter(context, android.R.layout.simple_list_item_1, mTabs.get(tag));
                    ListView listView = new ListView(context);
                    listView.setCacheColorHint(0);
                    listView.setOnItemClickListener(TabInput.this);
                    listView.setAdapter(adapter);
                    return listView;
                }
            });

            
            if (Versions.preHC) {
                TextView textview = (TextView) inflater.inflate(R.layout.tab_prompt_tab, null);
                textview.setText(title);
                spec.setIndicator(textview);
            } else {
                spec.setIndicator(title);
            }
            mHost.addTab(spec);
        }
        mView = mHost;
        return mHost;
    }

    @Override
    public Object getValue() {
        JSONObject obj = new JSONObject();
        try {
            obj.put("tab", mHost.getCurrentTab());
            obj.put("item", mPosition);
        } catch(JSONException ex) { }

        return obj;
    }

    @Override
    public boolean getScrollable() {
        return true;
    }

    @Override
    public boolean canApplyInputStyle() {
        return false;
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        ThreadUtils.assertOnUiThread();
        mPosition = position;
        notifyListeners(Integer.toString(position));
    }

}
