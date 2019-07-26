



package org.mozilla.gecko;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.Context;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.os.Build;
import android.util.AttributeSet;
import android.util.Xml;
import android.view.InflateException;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.SubMenu;

import java.io.IOException;

public class GeckoMenuInflater extends MenuInflater { 
    private static final String LOGTAG = "GeckoMenuInflater";

    private static final String TAG_MENU = "menu";
    private static final String TAG_ITEM = "item";
    private static final int NO_ID = 0;

    private Context mContext;

    private boolean isSubMenu;

    
    private class ParsedItem {
        public int id;
        public int order;
        public CharSequence title;
        public int iconRes;
        public boolean checkable;
        public boolean checked;
        public boolean visible;
        public boolean enabled;
        public int showAsAction;
    }

    public GeckoMenuInflater(Context context) {
        super(context);
        mContext = context;

        isSubMenu = false;
    }

    @Override
    public void inflate(int menuRes, Menu menu) {

        

        XmlResourceParser parser = null;
        try {
            parser = mContext.getResources().getXml(menuRes);
            AttributeSet attrs = Xml.asAttributeSet(parser);

            ParsedItem item = null;
            SubMenu subMenu = null;
            MenuItem menuItem = null;
   
            String tag;
            int eventType = parser.getEventType();

            do {
                tag = parser.getName();
    
                switch (eventType) {
                    case XmlPullParser.START_TAG:
                        if (tag.equals(TAG_ITEM)) {
                            
                            item = new ParsedItem();
                            parseItem(item, attrs);
                         } else if (tag.equals(TAG_MENU)) {
                            if (item != null) {
                                
                                isSubMenu = true;
                                subMenu = menu.addSubMenu(NO_ID, item.id, item.order, item.title);
                                menuItem = subMenu.getItem();

                                
                                setValues(item, menuItem);
                            }
                        }
                        break;
                        
                    case XmlPullParser.END_TAG:
                        if (parser.getName().equals(TAG_ITEM)) {
                            if (isSubMenu && subMenu == null) {
                                isSubMenu = false;
                            } else {
                                
                                if (subMenu == null)
                                    menuItem = menu.add(NO_ID, item.id, item.order, item.title);
                                else
                                    menuItem = subMenu.add(NO_ID, item.id, item.order, item.title);

                                setValues(item, menuItem);
                            }
                        } else if (tag.equals(TAG_MENU)) {
                            
                            subMenu = null;
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

        item.id = a.getResourceId(R.styleable.MenuItem_android_id, NO_ID);
        item.order = a.getInt(R.styleable.MenuItem_android_orderInCategory, 0);
        item.title = a.getText(R.styleable.MenuItem_android_title);
        item.iconRes = a.getResourceId(R.styleable.MenuItem_android_icon, 0);
        item.checkable = a.getBoolean(R.styleable.MenuItem_android_checkable, false);
        item.checked = a.getBoolean(R.styleable.MenuItem_android_checked, false);
        item.visible = a.getBoolean(R.styleable.MenuItem_android_visible, true);
        item.enabled = a.getBoolean(R.styleable.MenuItem_android_enabled, true);

        if (Build.VERSION.SDK_INT >= 11)
            item.showAsAction = a.getInt(R.styleable.MenuItem_android_showAsAction, 0);

        a.recycle();
    }
        
    public void setValues(ParsedItem item, MenuItem menuItem) {
        menuItem.setChecked(item.checked)
                .setVisible(item.visible)
                .setEnabled(item.enabled)
                .setCheckable(item.checkable)
                .setIcon(item.iconRes);

        if (Build.VERSION.SDK_INT >= 11)
            menuItem.setShowAsAction(item.showAsAction);
    }
}
