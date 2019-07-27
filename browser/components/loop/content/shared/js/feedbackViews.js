







var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = loop.shared.views || {};
loop.shared.views.FeedbackView = (function(l10n) {
  "use strict";

  var sharedActions = loop.shared.actions;
  var sharedMixins = loop.shared.mixins;

  var WINDOW_AUTOCLOSE_TIMEOUT_IN_SECONDS = 5;
  var FEEDBACK_STATES = loop.store.FEEDBACK_STATES;

  





  var FeedbackLayout = React.createClass({displayName: 'FeedbackLayout',
    propTypes: {
      children: React.PropTypes.component.isRequired,
      title: React.PropTypes.string.isRequired,
      reset: React.PropTypes.func 
    },

    render: function() {
      var backButton = React.DOM.div(null);
      if (this.props.reset) {
        backButton = (
          React.DOM.button({className: "fx-embedded-btn-back", type: "button", 
                  onClick: this.props.reset}, 
            "« ", l10n.get("feedback_back_button")
          )
        );
      }
      return (
        React.DOM.div({className: "feedback"}, 
          backButton, 
          React.DOM.h3(null, this.props.title), 
          this.props.children
        )
      );
    }
  });

  


  var FeedbackForm = React.createClass({displayName: 'FeedbackForm',
    propTypes: {
      feedbackStore: React.PropTypes.instanceOf(loop.store.FeedbackStore),
      pending:       React.PropTypes.bool,
      reset:         React.PropTypes.func
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
        disconnected : l10n.get("feedback_category_was_disconnected"),
        confusing:     l10n.get("feedback_category_confusing"),
        other:         l10n.get("feedback_category_other")
      };
    },

    _getCategoryFields: function() {
      var categories = this._getCategories();
      return Object.keys(categories).map(function(category, key) {
        return (
          React.DOM.label({key: key, className: "feedback-category-label"}, 
            React.DOM.input({type: "radio", ref: "category", name: "category", 
                   className: "feedback-category-radio", 
                   value: category, 
                   onChange: this.handleCategoryChange, 
                   checked: this.state.category === category}), 
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
        category: category,
        description: category == "other" ? "" : this._getCategories()[category]
      });
      if (category == "other") {
        this.refs.description.getDOMNode().focus();
      }
    },

    handleDescriptionFieldChange: function(event) {
      this.setState({description: event.target.value});
    },

    handleDescriptionFieldFocus: function(event) {
      this.setState({category: "other", description: ""});
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
      var descriptionDisplayValue = this.state.category === "other" ?
                                    this.state.description : "";
      return (
        FeedbackLayout({title: l10n.get("feedback_what_makes_you_sad"), 
                        reset: this.props.reset}, 
          React.DOM.form({onSubmit: this.handleFormSubmit}, 
            this._getCategoryFields(), 
            React.DOM.p(null, 
              React.DOM.input({type: "text", ref: "description", name: "description", 
                className: "feedback-description", 
                onChange: this.handleDescriptionFieldChange, 
                onFocus: this.handleDescriptionFieldFocus, 
                value: descriptionDisplayValue, 
                placeholder: 
                  l10n.get("feedback_custom_category_text_placeholder")})
            ), 
            React.DOM.button({type: "submit", className: "btn btn-success", 
                    disabled: !this._isFormReady()}, 
              l10n.get("feedback_submit_button")
            )
          )
        )
      );
    }
  });

  






  var FeedbackReceived = React.createClass({displayName: 'FeedbackReceived',
    propTypes: {
      onAfterFeedbackReceived: React.PropTypes.func
    },

    getInitialState: function() {
      return {countdown: WINDOW_AUTOCLOSE_TIMEOUT_IN_SECONDS};
    },

    componentDidMount: function() {
      this._timer = setInterval(function() {
        this.setState({countdown: this.state.countdown - 1});
      }.bind(this), 1000);
    },

    componentWillUnmount: function() {
      if (this._timer) {
        clearInterval(this._timer);
      }
    },

    render: function() {
      if (this.state.countdown < 1) {
        clearInterval(this._timer);
        if (this.props.onAfterFeedbackReceived) {
          this.props.onAfterFeedbackReceived();
        }
      }
      return (
        FeedbackLayout({title: l10n.get("feedback_thank_you_heading")}, 
          React.DOM.p({className: "info thank-you"}, 
            l10n.get("feedback_window_will_close_in2", {
              countdown: this.state.countdown,
              num: this.state.countdown
            }))
        )
      );
    }
  });

  


  var FeedbackView = React.createClass({displayName: 'FeedbackView',
    mixins: [Backbone.Events],

    propTypes: {
      feedbackStore: React.PropTypes.instanceOf(loop.store.FeedbackStore),
      onAfterFeedbackReceived: React.PropTypes.func,
      
      feedbackState: React.PropTypes.string
    },

    getInitialState: function() {
      var storeState = this.props.feedbackStore.getStoreState();
      return _.extend({}, storeState, {
        feedbackState: this.props.feedbackState || storeState.feedbackState
      });
    },

    componentWillMount: function() {
      this.listenTo(this.props.feedbackStore, "change", this._onStoreStateChanged);
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.feedbackStore);
    },

    _onStoreStateChanged: function() {
      this.setState(this.props.feedbackStore.getStoreState());
    },

    reset: function() {
      this.setState(this.props.feedbackStore.getInitialStoreState());
    },

    handleHappyClick: function() {
      
      
      this.props.feedbackStore.dispatchAction(new sharedActions.SendFeedback({
        happy: true,
        category: "",
        description: ""
      }));
    },

    handleSadClick: function() {
      this.props.feedbackStore.dispatchAction(
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
            FeedbackLayout({title: 
              l10n.get("feedback_call_experience_heading2")}, 
              React.DOM.div({className: "faces"}, 
                React.DOM.button({className: "face face-happy", 
                        onClick: this.handleHappyClick}), 
                React.DOM.button({className: "face face-sad", 
                        onClick: this.handleSadClick})
              )
            )
          );
        }
        case FEEDBACK_STATES.DETAILS: {
          return (
            FeedbackForm({
              feedbackStore: this.props.feedbackStore, 
              reset: this.reset, 
              pending: this.state.feedbackState === FEEDBACK_STATES.PENDING})
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
            FeedbackReceived({
              onAfterFeedbackReceived: this.props.onAfterFeedbackReceived})
          );
        }
      }
    }
  });

  return FeedbackView;
})(navigator.mozL10n || document.mozL10n);
