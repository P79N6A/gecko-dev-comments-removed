




package org.mozilla.gecko.widget;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.SessionParser;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.util.GamepadUtils;

import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Bitmap;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;

public class LastTabsSection extends AboutHomeSection {
    private Context mContext;

    public LastTabsSection(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    public void readLastTabs(GeckoProfile profile) {
        String jsonString = profile.readSessionFile(true);
        if (jsonString == null) {
            
            return;
        }

        final ArrayList<String> lastTabUrlsList = new ArrayList<String>();
        new SessionParser() {
            @Override
            public void onTabRead(final SessionTab tab) {
                final String url = tab.getSelectedUrl();
                
                if (url.equals("about:home")) {
                    return;
                }

                ContentResolver resolver = mContext.getContentResolver();
                final Bitmap favicon = BrowserDB.getFaviconForUrl(resolver, url);
                lastTabUrlsList.add(url);

                LastTabsSection.this.post(new Runnable() {
                    @Override
                    public void run() {
                        View container = LayoutInflater.from(mContext).inflate(R.layout.abouthome_last_tabs_row, getItemsContainer(), false);
                        ((TextView) container.findViewById(R.id.last_tab_title)).setText(tab.getSelectedTitle());
                        ((TextView) container.findViewById(R.id.last_tab_url)).setText(tab.getSelectedUrl());
                        if (favicon != null) {
                            ((ImageView) container.findViewById(R.id.last_tab_favicon)).setImageBitmap(favicon);
                        }

                        container.setOnClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                int flags = Tabs.LOADURL_NEW_TAB;
                                if (Tabs.getInstance().getSelectedTab().isPrivate())
                                    flags |= Tabs.LOADURL_PRIVATE;
                                Tabs.getInstance().loadUrl(url, flags);
                            }
                        });
                        container.setOnKeyListener(GamepadUtils.getClickDispatcher());

                        addItem(container);
                    }
                });
            }
        }.parse(jsonString);

        final int nu = lastTabUrlsList.size();
        if (nu >= 1) {
            post(new Runnable() {
                @Override
                public void run() {
                    if (nu > 1) {
                        showMoreText();
                        setOnMoreTextClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                int flags = Tabs.LOADURL_NEW_TAB;
                                if (Tabs.getInstance().getSelectedTab().isPrivate())
                                    flags |= Tabs.LOADURL_PRIVATE;
                                for (String url : lastTabUrlsList) {
                                    Tabs.getInstance().loadUrl(url, flags);
                                }
                            }
                        });
                    } else if (nu == 1) {
                        hideMoreText();
                    }
                    show();
                }
            });
        }
    }
}
