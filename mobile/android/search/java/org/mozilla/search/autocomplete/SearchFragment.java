



package org.mozilla.search.autocomplete;


import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.Fragment;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.TextView;

import org.mozilla.search.R;








public class SearchFragment extends Fragment implements AdapterView.OnItemClickListener,
        TextView.OnEditorActionListener, AcceptsJumpTaps {

    private View mainView;
    private FrameLayout backdropFrame;
    private EditText searchBar;
    private ListView suggestionDropdown;
    private InputMethodManager inputMethodManager;
    private AutoCompleteAdapter autoCompleteAdapter;
    private AutoCompleteAgentManager autoCompleteAgentManager;
    private State state;

    private enum State {
        WAITING,  
        RUNNING   
    }

    public SearchFragment() {
        
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {


        mainView = inflater.inflate(R.layout.search_auto_complete, container, false);
        backdropFrame = (FrameLayout) mainView.findViewById(R.id.auto_complete_backdrop);
        searchBar = (EditText) mainView.findViewById(R.id.auto_complete_search_bar);
        suggestionDropdown = (ListView) mainView.findViewById(R.id.auto_complete_dropdown);

        inputMethodManager =
                (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);

        
        searchBar.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {

            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {

            }

            @Override
            public void afterTextChanged(Editable s) {
                autoCompleteAgentManager.search(s.toString());
            }
        });
        searchBar.setOnEditorActionListener(this);
        searchBar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (v.hasFocus()) {
                    return;
                }
                transitionToRunning();
            }
        });

        backdropFrame.setOnClickListener(new BackdropClickListener());

        autoCompleteAdapter = new AutoCompleteAdapter(getActivity(), this);

        
        
        autoCompleteAdapter.setNotifyOnChange(false);

        suggestionDropdown.setAdapter(autoCompleteAdapter);

        initRows();

        autoCompleteAgentManager =
                new AutoCompleteAgentManager(getActivity(), new MainUiHandler(autoCompleteAdapter));

        
        

        
        
        transitionToRunning();

        
        suggestionDropdown.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String query = ((AutoCompleteModel) suggestionDropdown.getItemAtPosition(position))
                        .getMainText();
                startSearch(query);
            }
        });

        return mainView;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        inputMethodManager = null;
        mainView = null;
        searchBar = null;
        if (null != suggestionDropdown) {
            suggestionDropdown.setOnItemClickListener(null);
            suggestionDropdown.setAdapter(null);
            suggestionDropdown = null;
        }
        autoCompleteAdapter = null;
    }

    


    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        
        
    }

    


    @Override
    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (actionId == EditorInfo.IME_ACTION_SEARCH) {
            startSearch(v.getText().toString());
            return true;
        }
        return false;
    }


    private void initRows() {
        
        autoCompleteAdapter.add(new AutoCompleteModel("banana"));
        autoCompleteAdapter.add(new AutoCompleteModel("cat pics"));
        autoCompleteAdapter.add(new AutoCompleteModel("mexican food"));
        autoCompleteAdapter.add(new AutoCompleteModel("cuba libre"));

        autoCompleteAdapter.notifyDataSetChanged();
    }


    


    private void startSearch(String queryString) {
        if (getActivity() instanceof AcceptsSearchQuery) {
            searchBar.setText(queryString);
            searchBar.setSelection(queryString.length());
            transitionToWaiting();
            ((AcceptsSearchQuery) getActivity()).onSearch(queryString);
        } else {
            throw new RuntimeException("Parent activity does not implement AcceptsSearchQuery.");
        }
    }

    private void transitionToWaiting() {
        if (state == State.WAITING) {
            return;
        }
        searchBar.setFocusable(false);
        searchBar.setFocusableInTouchMode(false);
        searchBar.clearFocus();
        inputMethodManager.hideSoftInputFromWindow(searchBar.getWindowToken(), 0);
        suggestionDropdown.setVisibility(View.GONE);
        backdropFrame.setVisibility(View.GONE);
        state = State.WAITING;
    }

    private void transitionToRunning() {
        if (state == State.RUNNING) {
            return;
        }
        searchBar.setFocusable(true);
        searchBar.setFocusableInTouchMode(true);
        searchBar.requestFocus();
        inputMethodManager.showSoftInput(searchBar, InputMethodManager.SHOW_IMPLICIT);
        suggestionDropdown.setVisibility(View.VISIBLE);
        backdropFrame.setVisibility(View.VISIBLE);
        state = State.RUNNING;
    }

    @Override
    public void onJumpTap(String suggestion) {
        searchBar.setText(suggestion);
        
        searchBar.setSelection(suggestion.length());
        autoCompleteAgentManager.search(suggestion);
    }


    


    private static class MainUiHandler extends Handler {

        final AutoCompleteAdapter autoCompleteAdapter1;

        public MainUiHandler(AutoCompleteAdapter autoCompleteAdapter) {
            autoCompleteAdapter1 = autoCompleteAdapter;
        }

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (null == msg.obj) {
                return;
            }

            if (!(msg.obj instanceof Iterable)) {
                return;
            }

            autoCompleteAdapter1.clear();

            for (Object obj : (Iterable) msg.obj) {
                if (obj instanceof AutoCompleteModel) {
                    autoCompleteAdapter1.add((AutoCompleteModel) obj);
                }
            }
            autoCompleteAdapter1.notifyDataSetChanged();

        }
    }

    






    private class BackdropClickListener implements View.OnClickListener {
        @Override
        public void onClick(View v) {
            transitionToWaiting();
        }
    }
}
