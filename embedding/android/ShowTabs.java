




































package org.mozilla.gecko;

import java.io.*;
import java.util.*;

import org.mozilla.gecko.*;

import android.os.*;
import android.content.*;
import android.app.*;
import android.text.*;
import android.util.*;
import android.widget.*;
import android.database.sqlite.*;
import android.database.*;
import android.view.*;
import android.view.View.*;
import android.net.Uri;
import android.graphics.*;

public class ShowTabs extends ListActivity {
    private static final String LOG_FILE_NAME = "ShowTabs";
    public static final String ID = "id";
    public static final String TYPE = "type";
    private ArrayList <HashMap<String, String>> tabsList = null;
    public static enum Type { ADD, SWITCH };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.show_tabs);
        
        ListView list = (ListView) findViewById(android.R.id.list);
        Button addTab = new Button(this);
        addTab.setText("+ add tab");
        addTab.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Intent resultIntent = new Intent();
                resultIntent.putExtra(TYPE, Type.ADD.name());
                setResult(Activity.RESULT_OK, resultIntent);
                finish();
            }
        });

        list.addHeaderView(addTab);

        HashMap<Integer, Tab> tabs = Tabs.getInstance().getTabs();
        tabsList = new ArrayList<HashMap<String, String>> ();

        if (tabs != null) {
            Iterator keys = tabs.keySet().iterator();
            HashMap<String, String> map;
            Tab tab;
            while (keys.hasNext()) {
                tab = tabs.get(keys.next());
                map = new HashMap<String, String>();
                map.put("id", "" + tab.getId());
                map.put("title", tab.getTitle());
                map.put("url", tab.getURL());
                tabsList.add(map);
            }
        }
        
        list.setAdapter(new SimpleAdapter(
            ShowTabs.this,
            tabsList,
            R.layout.awesomebar_row,
            new String[] { "title", "url" },
            new int[] { R.id.title, R.id.url }
        ));
    }

    @Override
    public void onListItemClick(ListView l, View v, int position, long id) {
        HashMap<String, String> map = tabsList.get((int) id); 
        Intent resultIntent = new Intent();
        resultIntent.putExtra(TYPE, Type.SWITCH.name());
        resultIntent.putExtra(ID, map.get("id"));
        setResult(Activity.RESULT_OK, resultIntent);
        finish();
    }
}
