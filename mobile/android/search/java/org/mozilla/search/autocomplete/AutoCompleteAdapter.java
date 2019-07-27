



package org.mozilla.search.autocomplete;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import org.mozilla.search.R;

import java.util.List;




class AutoCompleteAdapter extends ArrayAdapter<String> {

    private final AcceptsJumpTaps acceptsJumpTaps;

    private final LayoutInflater inflater;

    public AutoCompleteAdapter(Context context, AcceptsJumpTaps acceptsJumpTaps) {
        
        
        super(context, 0);
        this.acceptsJumpTaps = acceptsJumpTaps;

        
        setNotifyOnChange(false);

        inflater = LayoutInflater.from(context);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if (convertView == null) {
            convertView = inflater.inflate(R.layout.search_auto_complete_row, null);
        }

        final String text = getItem(position);

        final TextView textView = (TextView) convertView.findViewById(R.id.auto_complete_row_text);
        textView.setText(text);

        final View jumpButton = convertView.findViewById(R.id.auto_complete_row_jump_button);
        jumpButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                acceptsJumpTaps.onJumpTap(text);
            }
        });

        return convertView;
    }

    




    public void update(List<String> suggestions) {
        clear();
        if (suggestions != null) {
            for (String s : suggestions) {
                add(s);
            }
        }
        notifyDataSetChanged();
    }
}
