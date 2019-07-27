



package org.mozilla.search;

import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.view.View;
import android.widget.ArrayAdapter;

import org.mozilla.search.stream.PreloadAgent;







public class PreSearchFragment extends ListFragment {

    private ArrayAdapter<PreloadAgent.TmpItem> adapter;

    



    public PreSearchFragment() {
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        getListView().setDivider(null);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        View headerView = getLayoutInflater(savedInstanceState)
                .inflate(R.layout.search_stream_header, getListView(), false);
        getListView().addHeaderView(headerView, null, false);
        if (null == adapter) {
            adapter = new ArrayAdapter<PreloadAgent.TmpItem>(getActivity(), R.layout.search_card,
                    R.id.card_title, PreloadAgent.ITEMS) {
                



                @Override
                public boolean isEnabled(int position) {
                    return false;
                }
            };
        }

        setListAdapter(adapter);
    }


}
