



var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = loop.shared.views || {};
loop.shared.views.FeedbackView = (function(l10n) {
  "use strict";

  var sharedActions = loop.shared.actions;
  var sharedMixins = loop.shared.mixins;

  var WINDOW_AUTOCLOSE_TIMEOUT_IN_SECONDS =
      loop.shared.views.WINDOW_AUTOCLOSE_TIMEOUT_IN_SECONDS = 5;
  var FEEDBACK_STATES = loop.store.FEEDBACK_STATES;

  





  var FeedbackLayout = React.createClass({displayName: "FeedbackLayout",
    propTypes: {
      children: React.PropTypes.component.isRequired,
      reset: React.PropTypes.func, 
      title: React.PropTypes.string.isRequired
    },

    render: function() {
      var backButton = React.createElement("div", null);
      if (this.props.reset) {
        backButton = (
          React.createElement("button", {className: "fx-embedded-btn-back", 
            onClick: this.props.reset, 
            type: "button"}, 
            "« ", l10n.get("feedback_back_button")
          )
        );
      }
      return (
        React.createElement("div", {className: "feedback"}, 
          backButton, 
          React.createElement("h3", null, this.props.title), 
          this.props.children
        )
      );
    }
  });

  


  var FeedbackForm = React.createClass({displayName: "FeedbackForm",
    propTypes: {
      feedbackStore: React.PropTypes.instanceOf(loop.store.FeedbackStore),
      pending: React.PropTypes.bool,
      reset: React.PropTypes.func
    },

    getInitialState: function() {
      return {category: "", description: ""};
    },

    getDefaultProps: function() {
      return {pending: false};
    },

    _getCategories: function() {
      return {
        audio_quality: l10n.get("feedback_category_audio_quality"),
        video_quality: l10n.get("feedback_category_video_quality"),
        disconnected: l10n.get("feedback_category_was_disconnected"),
        confusing: l10n.get("feedback_category_confusing2"),
        other: l10n.get("feedback_category_other2")
      };
    },

    _getCategoryFields: function() {
      var categories = this._getCategories();
      return Object.keys(categories).map(function(category, key) {
        return (
          React.createElement("label", {className: "feedback-category-label", key: key}, 
            React.createElement("input", {
              checked: this.state.category === category, 
              className: "feedback-category-radio", 
              name: "category", 
              onChange: this.handleCategoryChange, 
              ref: "category", 
              type: "radio", 
              value: category}), 
            categories[category]
          )
        );
      }, this);
    },

    









    _isFormReady: function() {
      if (this.props.pending || !this.state.category) {
        return false;
      }
      if (this.state.category === "other" && !this.state.description) {
        return false;
      }
      return true;
    },

    handleCategoryChange: function(event) {
      var category = event.target.value;
      this.setState({
        category: category
      });
      if (category == "other") {
        this.refs.description.getDOMNode().focus();
      }
    },

    handleDescriptionFieldChange: function(event) {
      this.setState({description: event.target.value});
    },

    handleFormSubmit: function(event) {
      event.preventDefault();
      
      this.props.feedbackStore.dispatchAction(new sharedActions.SendFeedback({
        happy: false,
        category: this.state.category,
        description: this.state.description
      }));
    },

    render: function() {
      return (
        React.createElement(FeedbackLayout, {
          reset: this.props.reset, 
          title: l10n.get("feedback_category_list_heading")}, 
          React.createElement("form", {onSubmit: this.handleFormSubmit}, 
            this._getCategoryFields(), 
            React.createElement("p", null, 
              React.createElement("input", {className: "feedback-description", 
                name: "description", 
                onChange: this.handleDescriptionFieldChange, 
                placeholder: 
                  l10n.get("feedback_custom_category_text_placeholder"), 
                ref: "description", 
                type: "text", 
                value: this.state.description})
            ), 
            React.createElement("button", {className: "btn btn-success", 
              disabled: !this._isFormReady(), 
              type: "submit"}, 
              l10n.get("feedback_submit_button")
            )
          )
        )
      );
    }
  });

  






  var FeedbackReceived = React.createClass({displayName: "FeedbackReceived",
    propTypes: {
      noCloseText: React.PropTypes.bool,
      onAfterFeedbackReceived: React.PropTypes.func
    },

    getInitialState: function() {
      return {countdown: WINDOW_AUTOCLOSE_TIMEOUT_IN_SECONDS};
    },

    componentDidMount: function() {
      this._timer = setInterval(function() {
      if (this.state.countdown == 1) {
        clearInterval(this._timer);
        if (this.props.onAfterFeedbackReceived) {
          this.props.onAfterFeedbackReceived();
        }
        return;
      }
        this.setState({countdown: this.state.countdown - 1});
      }.bind(this), 1000);
    },

    componentWillUnmount: function() {
      if (this._timer) {
        clearInterval(this._timer);
      }
    },

    _renderCloseText: function() {
      if (this.props.noCloseText) {
        return null;
      }

      return (
        React.createElement("p", {className: "info thank-you"}, 
          l10n.get("feedback_window_will_close_in2", {
            countdown: this.state.countdown,
            num: this.state.countdown
          }))
      );
    },

    render: function() {
      return (
        React.createElement(FeedbackLayout, {title: l10n.get("feedback_thank_you_heading")}, 
          this._renderCloseText()
        )
      );
    }
  });

  


  var FeedbackView = React.createClass({displayName: "FeedbackView",
    mixins: [
      Backbone.Events,
      loop.store.StoreMixin("feedbackStore")
    ],

    propTypes: {
      
      feedbackState: React.PropTypes.string,
      noCloseText: React.PropTypes.bool,
      onAfterFeedbackReceived: React.PropTypes.func
    },

    getInitialState: function() {
      var storeState = this.getStoreState();
      return _.extend({}, storeState, {
        feedbackState: this.props.feedbackState || storeState.feedbackState
      });
    },

    reset: function() {
      this.setState(this.getStore().getInitialStoreState());
    },

    handleHappyClick: function() {
      
      
      this.getStore().dispatchAction(new sharedActions.SendFeedback({
        happy: true,
        category: "",
        description: ""
      }));
    },

    handleSadClick: function() {
      this.getStore().dispatchAction(
        new sharedActions.RequireFeedbackDetails());
    },

    _onFeedbackSent: function(err) {
      if (err) {
        
        console.error("Unable to send user feedback", err);
      }
      this.setState({pending: false, step: "finished"});
    },

    render: function() {
      switch(this.state.feedbackState) {
        default:
        case FEEDBACK_STATES.INIT: {
          return (
            React.createElement(FeedbackLayout, {title: 
              l10n.get("feedback_call_experience_heading2")}, 
              React.createElement("div", {className: "faces"}, 
                React.createElement("button", {className: "face face-happy", 
                        onClick: this.handleHappyClick}), 
                React.createElement("button", {className: "face face-sad", 
                        onClick: this.handleSadClick})
              )
            )
          );
        }
        case FEEDBACK_STATES.DETAILS: {
          return (
            React.createElement(FeedbackForm, {
              feedbackStore: this.getStore(), 
              pending: this.state.feedbackState === FEEDBACK_STATES.PENDING, 
              reset: this.reset})
            );
        }
        case FEEDBACK_STATES.PENDING:
        case FEEDBACK_STATES.SENT:
        case FEEDBACK_STATES.FAILED: {
          if (this.state.error) {
            
            console.error("Error encountered while submitting feedback",
                          this.state.error);
          }
          return (
            React.createElement(FeedbackReceived, {
              noCloseText: this.props.noCloseText, 
              onAfterFeedbackReceived: this.props.onAfterFeedbackReceived})
          );
        }
      }
    }
  });

  return FeedbackView;
})(navigator.mozL10n || document.mozL10n);
