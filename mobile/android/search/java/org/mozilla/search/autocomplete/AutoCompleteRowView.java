



package org.mozilla.search.autocomplete;


import android.content.Context;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.mozilla.search.R;




class AutoCompleteRowView extends LinearLayout {

    private TextView textView;
    private AcceptsJumpTaps onJumpListener;

    public AutoCompleteRowView(Context context) {
        super(context);
        init();
    }

    private void init() {
        setOrientation(LinearLayout.HORIZONTAL);
        setGravity(Gravity.CENTER_VERTICAL);

        LayoutInflater inflater =
                (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        inflater.inflate(R.layout.search_auto_complete_row, this, true);

        textView = (TextView) findViewById(R.id.auto_complete_row_text);

        findViewById(R.id.auto_complete_row_jump_button).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (null == onJumpListener) {
                    Log.e("SuggestionRow.onJump", "jump listener is null");
                    return;
                }

                onJumpListener.onJumpTap(textView.getText().toString());
            }
        });
    }

    public void setMainText(String s) {
        textView.setText(s);
    }

    public void setOnJumpListener(AcceptsJumpTaps onJumpListener) {
        this.onJumpListener = onJumpListener;
    }
}
