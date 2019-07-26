



package org.mozilla.gecko;

import org.mozilla.gecko.animation.AnimationUtils;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.widget.GeckoPopupMenu;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.ScaleAnimation;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;

class ActionModeCompatView extends LinearLayout implements GeckoMenu.ActionItemBarPresenter {
    private final String LOGTAG = "GeckoActionModeCompatPresenter";

    private static final int SPEC = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);

    private Button mTitleView;
    private ImageButton mMenuButton;
    private ViewGroup mActionButtonBar;
    private GeckoPopupMenu mPopupMenu;

    
    private static final int MAX_ACTION_ITEMS = 4;

    private int mActionButtonsWidth = 0;

    public ActionModeCompatView(Context context) {
        super(context);
        init(context);
    }

    public ActionModeCompatView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public ActionModeCompatView(Context context, AttributeSet attrs, int style) {
        super(context, attrs, style);
        init(context);
    }

    public void init(Context context) {
        LayoutInflater.from(context).inflate(R.layout.actionbar, this);

        mTitleView = (Button) findViewById(R.id.actionmode_title);
        mMenuButton = (ImageButton) findViewById(R.id.actionbar_menu);
        mActionButtonBar = (ViewGroup) findViewById(R.id.actionbar_buttons);

        mPopupMenu = new GeckoPopupMenu(getContext(), mMenuButton);
        ((GeckoMenu) mPopupMenu.getMenu()).setActionItemBarPresenter(this);

        mMenuButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                openMenu();
            }
        });
    }

    public void initForMode(final ActionModeCompat mode) {
        mTitleView.setOnClickListener(mode);
        mPopupMenu.setOnMenuItemClickListener(mode);
        mPopupMenu.setOnMenuItemLongClickListener(mode);
    }

    public CharSequence getTitle() {
        return mTitleView.getText();
    }

    public void setTitle(CharSequence title) {
        mTitleView.setText(title);
    }

    public void setTitle(int resId) {
        mTitleView.setText(resId);
    }

    public Menu getMenu() {
        return mPopupMenu.getMenu();
    }

    public void invalidate() {
        
        if (mPopupMenu != null && mMenuButton != null) {
            mMenuButton.setVisibility(mPopupMenu.getMenu().hasVisibleItems() ? View.VISIBLE : View.GONE);
        }
        super.invalidate();
    }

    
    @Override
    public boolean addActionItem(View actionItem) {
        final int count = mActionButtonBar.getChildCount();
        if (count >= MAX_ACTION_ITEMS) {
            return false;
        }

        int maxWidth = mActionButtonBar.getMeasuredWidth();
        if (maxWidth == 0) {
            mActionButtonBar.measure(SPEC, SPEC);
            maxWidth = mActionButtonBar.getMeasuredWidth();
        }

        
        if (mMenuButton.getVisibility() == View.GONE) {
            
            mMenuButton.measure(SPEC, SPEC);
            maxWidth -= mMenuButton.getMeasuredWidth();
        }

        if (mActionButtonsWidth <= 0) {
            mActionButtonsWidth = 0;

            
            for (int i = 0; i < count; i++) {
                View v = mActionButtonBar.getChildAt(i);
                v.measure(SPEC, SPEC);
                mActionButtonsWidth += v.getMeasuredWidth();
            }
        }

        actionItem.measure(SPEC, SPEC);
        int w = actionItem.getMeasuredWidth();
        if (mActionButtonsWidth + w < maxWidth) {
            
            mActionButtonsWidth += w;
            mActionButtonBar.addView(actionItem);
            return true;
        }

        return false;
    }

    
    @Override
    public void removeActionItem(View actionItem) {
        actionItem.measure(SPEC, SPEC);
        mActionButtonsWidth -= actionItem.getMeasuredWidth();
        mActionButtonBar.removeView(actionItem);
    }

    public void openMenu() {
        mPopupMenu.openMenu();
    }

    public void closeMenu() {
        mPopupMenu.dismiss();
    }

    public void animateIn() {
        long duration = AnimationUtils.getShortDuration(getContext());
        TranslateAnimation t = new TranslateAnimation(Animation.RELATIVE_TO_SELF, -0.5f, Animation.RELATIVE_TO_SELF, 0f,
                                                      Animation.RELATIVE_TO_SELF,  0f,   Animation.RELATIVE_TO_SELF, 0f);
        t.setDuration(duration);

        ScaleAnimation s = new ScaleAnimation(1f, 1f, 0f, 1f,
                                              Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
        s.setDuration((long) (duration * 1.5f));

        mTitleView.startAnimation(t);
        mActionButtonBar.startAnimation(s);
        mMenuButton.startAnimation(s);
    }
}
