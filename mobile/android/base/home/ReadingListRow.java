




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.home.TwoLinePageRow;
import org.mozilla.gecko.util.StringUtils;

import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;

public class ReadingListRow extends LinearLayout {

    private final Resources resources;

    private final TextView title;
    private final TextView excerpt;
    private final TextView readTime;

    
    private static final int AVERAGE_READING_SPEED = 250;

    
    private static final float AVERAGE_WORD_LENGTH = 5.1f;


    public ReadingListRow(Context context) {
        this(context, null);
    }

    public ReadingListRow(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(R.layout.reading_list_row_view, this);

        resources = context.getResources();

        title = (TextView) findViewById(R.id.title);
        excerpt = (TextView) findViewById(R.id.excerpt);
        readTime = (TextView) findViewById(R.id.read_time);
    }

    public void updateFromCursor(Cursor cursor) {
        if (cursor == null) {
            return;
        }

        final String url = cursor.getString(cursor.getColumnIndexOrThrow(ReadingListItems.URL));

        final String titleText = cursor.getString(cursor.getColumnIndexOrThrow(ReadingListItems.TITLE));
        title.setText(TextUtils.isEmpty(titleText) ? StringUtils.stripCommonSubdomains(StringUtils.stripScheme(url)) : titleText);

        final String excerptText = cursor.getString(cursor.getColumnIndexOrThrow(ReadingListItems.EXCERPT));
        excerpt.setText(TextUtils.isEmpty(excerptText) ? url : excerptText);

        








    }

    





    private static int getEstimatedReadTime(int length) {
        final int minutes = (int) Math.ceil((length / AVERAGE_WORD_LENGTH) / AVERAGE_READING_SPEED);

        
        return Math.max(minutes, 1);
    }
}
