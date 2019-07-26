




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.HomeItems;
import org.mozilla.gecko.home.HomeConfig.ItemType;

import com.squareup.picasso.Picasso;

import android.content.Context;
import android.database.Cursor;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.lang.ref.WeakReference;

class PanelItemView extends LinearLayout {
    private final TextView mTitle;
    private final TextView mDescription;
    private final ImageView mImage;
    private final LinearLayout mTitleDescContainer;

    private PanelItemView(Context context, int layoutId) {
        super(context);

        LayoutInflater.from(context).inflate(layoutId, this);
        mTitle = (TextView) findViewById(R.id.title);
        mDescription = (TextView) findViewById(R.id.description);
        mImage = (ImageView) findViewById(R.id.image);
        mTitleDescContainer = (LinearLayout) findViewById(R.id.title_desc_container);
    }

    public void updateFromCursor(Cursor cursor) {
        int titleIndex = cursor.getColumnIndexOrThrow(HomeItems.TITLE);
        final String title = cursor.getString(titleIndex);

        
        final boolean hasTitle = !TextUtils.isEmpty(title);
        mTitleDescContainer.setVisibility(hasTitle ? View.VISIBLE : View.GONE);
        if (hasTitle) {
            mTitle.setText(title);

            int descriptionIndex = cursor.getColumnIndexOrThrow(HomeItems.DESCRIPTION);
            final String description = cursor.getString(descriptionIndex);

            final boolean hasDescription = !TextUtils.isEmpty(description);
            mDescription.setVisibility(hasDescription ? View.VISIBLE : View.GONE);
            if (hasDescription) {
                mDescription.setText(description);
            }
        }

        int imageIndex = cursor.getColumnIndexOrThrow(HomeItems.IMAGE_URL);
        final String imageUrl = cursor.getString(imageIndex);

        
        final boolean hasImageUrl = !TextUtils.isEmpty(imageUrl);
        mImage.setVisibility(hasImageUrl ? View.VISIBLE : View.GONE);

        if (hasImageUrl) {
            Picasso.with(getContext())
                   .load(imageUrl)
                   .error(R.drawable.favicon)
                   .into(mImage);
        }
    }

    private static class ArticleItemView extends PanelItemView {
        private ArticleItemView(Context context) {
            super(context, R.layout.panel_article_item);
            setOrientation(LinearLayout.HORIZONTAL);
        }
    }

    private static class ImageItemView extends PanelItemView {
        private ImageItemView(Context context) {
            super(context, R.layout.panel_image_item);
            setOrientation(LinearLayout.VERTICAL);
        }
    }

    public static PanelItemView create(Context context, ItemType itemType) {
        switch(itemType) {
            case ARTICLE:
                return new ArticleItemView(context);

            case IMAGE:
                return new ImageItemView(context);

            default:
                throw new IllegalArgumentException("Could not create panel item view from " + itemType);
        }
    }
}
