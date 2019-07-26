



package org.mozilla.gecko;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.Context;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.util.AttributeSet;
import android.util.Xml;
import android.view.InflateException;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import java.io.IOException;

public class GeckoMenuInflater extends MenuInflater { 
    private static final String LOGTAG = "GeckoMenuInflater";

    private static final String TAG_ITEM = "item";
    private static final int NO_ID = 0;

    private Context mContext;

    
    private class ParsedItem {
        public int id;
        public int order;
        public CharSequence title;
        public int iconRes;
        public boolean checkable;
        public boolean checked;
        public boolean visible;
        public boolean enabled;
        public boolean showAsAction;
    }

    public GeckoMenuInflater(Context context) {
        super(context);
        mContext = context;
    }

    public void inflate(int menuRes, Menu menu) {

        
        
        

        XmlResourceParser parser = null;
        try {
            parser = mContext.getResources().getXml(menuRes);
            AttributeSet attrs = Xml.asAttributeSet(parser);

            ParsedItem item = null;
   
            String tag;
            int eventType = parser.getEventType();

            do {
                tag = parser.getName();
    
                switch (eventType) {
                    case XmlPullParser.START_TAG:
                        if (tag.equals(TAG_ITEM)) {
                            
                            item = new ParsedItem();
                            parseItem(item, attrs);
                         }
                        break;
                        
                    case XmlPullParser.END_TAG:
                        if (parser.getName().equals(TAG_ITEM)) {
                            
                            MenuItem menuItem = menu.add(NO_ID, item.id, item.order, item.title);
                            setValues(item, menuItem);
                        }
                        break;
                }

                eventType = parser.next();

            } while (eventType != XmlPullParser.END_DOCUMENT);

        } catch (XmlPullParserException e) {
            throw new InflateException("Error inflating menu XML", e);
        } catch (IOException e) {
            throw new InflateException("Error inflating menu XML", e);
        } finally {
            if (parser != null)
                parser.close();
        }
    }

    public void parseItem(ParsedItem item, AttributeSet attrs) {
        TypedArray a = mContext.obtainStyledAttributes(attrs, R.styleable.MenuItem);

        item.id = a.getResourceId(R.styleable.MenuItem_id, NO_ID);
        item.order = a.getInt(R.styleable.MenuItem_orderInCategory, 0);
        item.title = a.getText(R.styleable.MenuItem_title);
        item.iconRes = a.getResourceId(R.styleable.MenuItem_icon, 0);
        item.checkable = a.getBoolean(R.styleable.MenuItem_checkable, false);
        item.checked = a.getBoolean(R.styleable.MenuItem_checked, false);
        item.visible = a.getBoolean(R.styleable.MenuItem_visible, true);
        item.enabled = a.getBoolean(R.styleable.MenuItem_enabled, true);
        item.showAsAction = a.getBoolean(R.styleable.MenuItem_showAsAction, false);

        a.recycle();
    }
        
    public void setValues(ParsedItem item, MenuItem menuItem) {
        menuItem.setChecked(item.checked)
                .setVisible(item.visible)
                .setEnabled(item.enabled)
                .setCheckable(item.checkable)
                .setCheckable(item.checked)
                .setIcon(item.iconRes)
                .setShowAsAction(item.showAsAction ? 1 : 0);
    }
}
