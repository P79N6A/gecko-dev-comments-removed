








var loop = loop || {};
loop.contacts = (function(_, mozL10n) {
  "use strict";

  var sharedMixins = loop.shared.mixins;

  const Button = loop.shared.views.Button;
  const ButtonGroup = loop.shared.views.ButtonGroup;
  const CALL_TYPES = loop.shared.utils.CALL_TYPES;

  
  const CONTACTS_CHUNK_SIZE = 100;

  
  const MIN_CONTACTS_FOR_FILTERING = 7;

  let getContactNames = function(contact) {
    
    
    
    
    let names = contact.name[0].split(" ");
    return {
      firstName: names.shift(),
      lastName: names.join(" ")
    };
  };

  







  let getPreferred = function(contact, field) {
    if (!contact[field] || !contact[field].length) {
      return { value: "" };
    }
    return contact[field].find(e => e.pref) || contact[field][0];
  };

  








  let setPreferred = function(contact, field, value) {
    
    if (!value && (!contact[field] || !contact[field].length)) {
      return;
    }

    if (!contact[field]) {
      contact[field] = [];
    }

    if (!contact[field].length) {
      contact[field][0] = {"value": value};
      return;
    }
    
    for (let i in contact[field]) {
      if (contact[field][i].pref) {
        contact[field][i].value = value;
        return;
      }
    }
    contact[field][0].value = value;
  };

  const GravatarPromo = React.createClass({displayName: "GravatarPromo",
    mixins: [sharedMixins.WindowCloseMixin],

    propTypes: {
      handleUse: React.PropTypes.func.isRequired
    },

    getInitialState: function() {
      return {
        showMe: navigator.mozLoop.getLoopPref("contacts.gravatars.promo") &&
          !navigator.mozLoop.getLoopPref("contacts.gravatars.show")
      };
    },

    handleCloseButtonClick: function() {
      navigator.mozLoop.setLoopPref("contacts.gravatars.promo", false);
      this.setState({ showMe: false });
    },

    handleLinkClick: function(event) {
      if (!event.target || !event.target.href) {
        return;
      }

      event.preventDefault();
      navigator.mozLoop.openURL(event.target.href);
      this.closeWindow();
    },

    handleUseButtonClick: function() {
      navigator.mozLoop.setLoopPref("contacts.gravatars.promo", false);
      navigator.mozLoop.setLoopPref("contacts.gravatars.show", true);
      this.setState({ showMe: false });
      this.props.handleUse();
    },

    render: function() {
      if (!this.state.showMe) {
        return null;
      }

      let privacyUrl = navigator.mozLoop.getLoopPref("legal.privacy_url");
      let message = mozL10n.get("gravatars_promo_message", {
        "learn_more": React.renderToStaticMarkup(
          React.createElement("a", {href: privacyUrl, target: "_blank"}, 
            mozL10n.get("gravatars_promo_message_learnmore")
          )
        )
      });
      return (
        React.createElement("div", {className: "contacts-gravatar-promo"}, 
          React.createElement(Button, {additionalClass: "button-close", onClick: this.handleCloseButtonClick}), 
          React.createElement("p", {dangerouslySetInnerHTML: {__html: message}, 
             onClick: this.handleLinkClick}), 
          React.createElement(ButtonGroup, null, 
            React.createElement(Button, {caption: mozL10n.get("gravatars_promo_button_nothanks"), 
                    onClick: this.handleCloseButtonClick}), 
            React.createElement(Button, {caption: mozL10n.get("gravatars_promo_button_use"), 
                    additionalClass: "button-accept", 
                    onClick: this.handleUseButtonClick})
          )
        )
      );
    }
  });

  const ContactDropdown = React.createClass({displayName: "ContactDropdown",
    propTypes: {
      handleAction: React.PropTypes.func.isRequired,
      canEdit: React.PropTypes.bool
    },

    getInitialState: function () {
      return {
        openDirUp: false,
      };
    },

    componentDidMount: function () {
      
      
      

      let menuNode = this.getDOMNode();
      let menuNodeRect = menuNode.getBoundingClientRect();

      let listNode = document.getElementsByClassName("contact-list")[0];
      let listNodeRect = listNode.getBoundingClientRect();

      if (menuNodeRect.top + menuNodeRect.height >=
          listNodeRect.top + listNodeRect.height) {
        this.setState({
          openDirUp: true,
        });
      }
    },

    onItemClick: function(event) {
      this.props.handleAction(event.currentTarget.dataset.action);
    },

    render: function() {
      var cx = React.addons.classSet;

      let blockAction = this.props.blocked ? "unblock" : "block";
      let blockLabel = this.props.blocked ? "unblock_contact_menu_button"
                                          : "block_contact_menu_button";

      return (
        React.createElement("ul", {className: cx({ "dropdown-menu": true,
                            "dropdown-menu-up": this.state.openDirUp })}, 
          React.createElement("li", {className: cx({ "dropdown-menu-item": true,
                              "disabled": this.props.blocked }), 
              onClick: this.onItemClick, 
              "data-action": "video-call"}, 
            React.createElement("i", {className: "icon icon-video-call"}), 
            mozL10n.get("video_call_menu_button")
          ), 
          React.createElement("li", {className: cx({ "dropdown-menu-item": true,
                              "disabled": this.props.blocked }), 
              onClick: this.onItemClick, "data-action": "audio-call"}, 
            React.createElement("i", {className: "icon icon-audio-call"}), 
            mozL10n.get("audio_call_menu_button")
          ), 
          React.createElement("li", {className: cx({ "dropdown-menu-item": true,
                              "disabled": !this.props.canEdit }), 
              onClick: this.onItemClick, "data-action": "edit"}, 
            React.createElement("i", {className: "icon icon-edit"}), 
            mozL10n.get("edit_contact_menu_button")
          ), 
          React.createElement("li", {className: "dropdown-menu-item", 
              onClick: this.onItemClick, "data-action": blockAction}, 
            React.createElement("i", {className: "icon icon-" + blockAction}), 
            mozL10n.get(blockLabel)
          ), 
          React.createElement("li", {className: cx({ "dropdown-menu-item": true,
                              "disabled": !this.props.canEdit }), 
              onClick: this.onItemClick, "data-action": "remove"}, 
            React.createElement("i", {className: "icon icon-remove"}), 
            mozL10n.get("remove_contact_menu_button")
          )
        )
      );
    }
  });

  const ContactDetail = React.createClass({displayName: "ContactDetail",
    getInitialState: function() {
      return {
        showMenu: false,
      };
    },

    propTypes: {
      handleContactAction: React.PropTypes.func,
      contact: React.PropTypes.object.isRequired
    },

    _onBodyClick: function() {
      
      setTimeout(this.hideDropdownMenu, 10);
    },

    showDropdownMenu: function() {
      document.body.addEventListener("click", this._onBodyClick);
      this.setState({showMenu: true});
    },

    hideDropdownMenu: function() {
      document.body.removeEventListener("click", this._onBodyClick);
      
      
      if (this.isMounted()) {
        this.setState({showMenu: false});
      }
    },

    componentWillUnmount: function() {
      document.body.removeEventListener("click", this._onBodyClick);
    },

    shouldComponentUpdate: function(nextProps, nextState) {
      let currContact = this.props.contact;
      let nextContact = nextProps.contact;
      let currContactEmail = getPreferred(currContact, "email").value;
      let nextContactEmail = getPreferred(nextContact, "email").value;
      return (
        currContact.name[0] !== nextContact.name[0] ||
        currContact.blocked !== nextContact.blocked ||
        currContactEmail !== nextContactEmail ||
        nextState.showMenu !== this.state.showMenu
      );
    },

    handleAction: function(actionName) {
      if (this.props.handleContactAction) {
        this.props.handleContactAction(this.props.contact, actionName);
      }
    },

    canEdit: function() {
      
      
      return this.props.contact.category[0] != "google";
    },

    render: function() {
      let names = getContactNames(this.props.contact);
      let email = getPreferred(this.props.contact, "email");
      let cx = React.addons.classSet;
      let contactCSSClass = cx({
        contact: true,
        blocked: this.props.contact.blocked
      });

      return (
        React.createElement("li", {className: contactCSSClass, onMouseLeave: this.hideDropdownMenu}, 
          React.createElement("div", {className: "avatar"}, 
            React.createElement("img", {src: navigator.mozLoop.getUserAvatar(email.value)})
          ), 
          React.createElement("div", {className: "details"}, 
            React.createElement("div", {className: "username"}, React.createElement("strong", null, names.firstName), " ", names.lastName, 
              React.createElement("i", {className: cx({"icon icon-google": this.props.contact.category[0] == "google"})}), 
              React.createElement("i", {className: cx({"icon icon-blocked": this.props.contact.blocked})})
            ), 
            React.createElement("div", {className: "email"}, email.value)
          ), 
          React.createElement("div", {className: "icons"}, 
            React.createElement("i", {className: "icon icon-video", 
               onClick: this.handleAction.bind(null, "video-call")}), 
            React.createElement("i", {className: "icon icon-caret-down", 
               onClick: this.showDropdownMenu})
          ), 
          this.state.showMenu
            ? React.createElement(ContactDropdown, {handleAction: this.handleAction, 
                               canEdit: this.canEdit(), 
                               blocked: this.props.contact.blocked})
            : null
          
        )
      );
    }
  });

  const ContactsList = React.createClass({displayName: "ContactsList",
    mixins: [
      React.addons.LinkedStateMixin,
      loop.shared.mixins.WindowCloseMixin
    ],

    propTypes: {
      notifications: React.PropTypes.instanceOf(
        loop.shared.models.NotificationCollection).isRequired
    },

    


    contacts: null,

    


    _userProfile: null,

    getInitialState: function() {
      return {
        importBusy: false,
        filter: "",
      };
    },

    refresh: function(callback = function() {}) {
      let contactsAPI = navigator.mozLoop.contacts;

      this.handleContactRemoveAll();

      contactsAPI.getAll((err, contacts) => {
        if (err) {
          callback(err);
          return;
        }

        
        
        let addContactsInChunks = () => {
          contacts.splice(0, CONTACTS_CHUNK_SIZE).forEach(contact => {
            this.handleContactAddOrUpdate(contact, false);
          });
          if (contacts.length) {
            setTimeout(addContactsInChunks, 0);
          } else {
            callback();
          }
          this.forceUpdate();
        };

        addContactsInChunks(contacts);
      });
    },

    componentWillMount: function() {
      
      
      this.contacts = {};
      this._userProfile = navigator.mozLoop.userProfile;
    },

    componentDidMount: function() {
      window.addEventListener("LoopStatusChanged", this._onStatusChanged);

      this.refresh(err => {
        if (err) {
          throw err;
        }

        let contactsAPI = navigator.mozLoop.contacts;

        
        contactsAPI.on("add", (eventName, contact) => {
          this.handleContactAddOrUpdate(contact);
        });
        contactsAPI.on("remove", (eventName, contact) => {
          this.handleContactRemove(contact);
        });
        contactsAPI.on("removeAll", () => {
          this.handleContactRemoveAll();
        });
        contactsAPI.on("update", (eventName, contact) => {
          this.handleContactAddOrUpdate(contact);
        });
      });
    },

    componentWillUnmount: function() {
      window.removeEventListener("LoopStatusChanged", this._onStatusChanged);
    },

    _onStatusChanged: function() {
      let profile = navigator.mozLoop.userProfile;
      let currUid = this._userProfile ? this._userProfile.uid : null;
      let newUid = profile ? profile.uid : null;
      if (currUid != newUid) {
        
        this._userProfile = profile;
        
        this.refresh();
      }
    },

    handleContactAddOrUpdate: function(contact, render = true) {
      let contacts = this.contacts;
      let guid = String(contact._guid);
      contacts[guid] = contact;
      if (render) {
        this.forceUpdate();
      }
    },

    handleContactRemove: function(contact) {
      let contacts = this.contacts;
      let guid = String(contact._guid);
      if (!contacts[guid]) {
        return;
      }
      delete contacts[guid];
      this.forceUpdate();
    },

    handleContactRemoveAll: function() {
      
      this.contacts = {};
      this.forceUpdate();
    },

    handleImportButtonClick: function() {
      this.setState({ importBusy: true });
      navigator.mozLoop.startImport({
        service: "google"
      }, (err, stats) => {
        this.setState({ importBusy: false });
        if (err) {
          console.error("Contact import error", err);
          this.props.notifications.errorL10n("import_contacts_failure_message");
          return;
        }
        this.props.notifications.successL10n("import_contacts_success_message", {
          num: stats.success,
          total: stats.success
        });
      });
    },

    handleAddContactButtonClick: function() {
      this.props.startForm("contacts_add");
    },

    handleContactAction: function(contact, actionName) {
      switch (actionName) {
        case "edit":
          this.props.startForm("contacts_edit", contact);
          break;
        case "remove":
          navigator.mozLoop.confirm({
            message: mozL10n.get("confirm_delete_contact_alert"),
            okButton: mozL10n.get("confirm_delete_contact_remove_button"),
            cancelButton: mozL10n.get("confirm_delete_contact_cancel_button")
          }, (err, result) => {
            if (err) {
              throw err;
            }

            if (!result) {
              return;
            }

            navigator.mozLoop.contacts.remove(contact._guid, err => {
              if (err) {
                throw err;
              }
            });
          });
          break;
        case "block":
        case "unblock":
          
          navigator.mozLoop.contacts[actionName](contact._guid, err => {
            if (err) {
              throw err;
            }
          });
          break;
        case "video-call":
          if (!contact.blocked) {
            navigator.mozLoop.calls.startDirectCall(contact, CALL_TYPES.AUDIO_VIDEO);
            this.closeWindow();
          }
          break;
        case "audio-call":
          if (!contact.blocked) {
            navigator.mozLoop.calls.startDirectCall(contact, CALL_TYPES.AUDIO_ONLY);
            this.closeWindow();
          }
          break;
        default:
          console.error("Unrecognized action: " + actionName);
          break;
      }
    },

    handleUseGravatar: function() {
      
      
      this.refresh();
    },

    sortContacts: function(contact1, contact2) {
      let comp = contact1.name[0].localeCompare(contact2.name[0]);
      if (comp !== 0) {
        return comp;
      }
      
      
      return contact1._guid - contact2._guid;
    },

    render: function() {
      let cx = React.addons.classSet;

      let viewForItem = item => {
        return (
          React.createElement(ContactDetail, {key: item._guid, contact: item, 
                         handleContactAction: this.handleContactAction})
        );
      };

      let shownContacts = _.groupBy(this.contacts, function(contact) {
        return contact.blocked ? "blocked" : "available";
      });

      let showFilter = Object.getOwnPropertyNames(this.contacts).length >=
                       MIN_CONTACTS_FOR_FILTERING;
      if (showFilter) {
        let filter = this.state.filter.trim().toLocaleLowerCase();
        if (filter) {
          let filterFn = contact => {
            return contact.name[0].toLocaleLowerCase().contains(filter) ||
                   getPreferred(contact, "email").value.toLocaleLowerCase().contains(filter);
          };
          if (shownContacts.available) {
            shownContacts.available = shownContacts.available.filter(filterFn);
          }
          if (shownContacts.blocked) {
            shownContacts.blocked = shownContacts.blocked.filter(filterFn);
          }
        }
      }

      return (
        React.createElement("div", null, 
          React.createElement("div", {className: "content-area"}, 
            React.createElement(ButtonGroup, null, 
              React.createElement(Button, {caption: this.state.importBusy
                               ? mozL10n.get("importing_contacts_progress_button")
                               : mozL10n.get("import_contacts_button"), 
                      disabled: this.state.importBusy, 
                      onClick: this.handleImportButtonClick}, 
                React.createElement("div", {className: cx({"contact-import-spinner": true,
                                    spinner: true,
                                    busy: this.state.importBusy})})
              ), 
              React.createElement(Button, {caption: mozL10n.get("new_contact_button"), 
                      onClick: this.handleAddContactButtonClick})
            ), 
            showFilter ?
            React.createElement("input", {className: "contact-filter", 
                   placeholder: mozL10n.get("contacts_search_placesholder"), 
                   valueLink: this.linkState("filter")})
            : null, 
            React.createElement(GravatarPromo, {handleUse: this.handleUseGravatar})
          ), 
          React.createElement("ul", {className: "contact-list"}, 
            shownContacts.available ?
              shownContacts.available.sort(this.sortContacts).map(viewForItem) :
              null, 
            shownContacts.blocked && shownContacts.blocked.length > 0 ?
              React.createElement("div", {className: "contact-separator"}, mozL10n.get("contacts_blocked_contacts")) :
              null, 
            shownContacts.blocked ?
              shownContacts.blocked.sort(this.sortContacts).map(viewForItem) :
              null
          )
        )
      );
    }
  });

  const ContactDetailsForm = React.createClass({displayName: "ContactDetailsForm",
    mixins: [React.addons.LinkedStateMixin],

    propTypes: {
      mode: React.PropTypes.string
    },

    getInitialState: function() {
      return {
        contact: null,
        pristine: true,
        name: "",
        email: "",
        tel: "",
      };
    },

    initForm: function(contact) {
      let state = this.getInitialState();
      if (contact) {
        state.contact = contact;
        state.name = contact.name[0];
        state.email = getPreferred(contact, "email").value;
        state.tel = getPreferred(contact, "tel").value;
      }
      this.setState(state);
    },

    handleAcceptButtonClick: function() {
      
      this.setState({
        pristine: false,
      });

      let emailInput = this.refs.email.getDOMNode();
      let telInput = this.refs.tel.getDOMNode();
      if (!this.refs.name.getDOMNode().checkValidity() ||
          ((emailInput.required || emailInput.value) && !emailInput.checkValidity()) ||
          ((telInput.required || telInput.value) && !telInput.checkValidity())) {
        return;
      }

      this.props.selectTab("contacts");

      let contactsAPI = navigator.mozLoop.contacts;

      switch (this.props.mode) {
        case "edit":
          this.state.contact.name[0] = this.state.name.trim();
          setPreferred(this.state.contact, "email", this.state.email.trim());
          setPreferred(this.state.contact, "tel", this.state.tel.trim());
          contactsAPI.update(this.state.contact, err => {
            if (err) {
              throw err;
            }
          });
          this.setState({
            contact: null,
          });
          break;
        case "add":
          var contact = {
            id: navigator.mozLoop.generateUUID(),
            name: [this.state.name.trim()],
            email: [{
              pref: true,
              type: ["home"],
              value: this.state.email.trim()
            }],
            category: ["local"]
          };
          var tel = this.state.tel.trim();
          if (!!tel) {
            contact["tel"] = [{
              pref: true,
              type: ["fxos"],
              value: tel
            }];
          }
          contactsAPI.add(contact, err => {
            if (err) {
              throw err;
            }
          });
          break;
      }
    },

    handleCancelButtonClick: function() {
      this.props.selectTab("contacts");
    },

    render: function() {
      let cx = React.addons.classSet;
      let phoneOrEmailRequired = !this.state.email && !this.state.tel;

      return (
        React.createElement("div", {className: "content-area contact-form"}, 
          React.createElement("header", null, this.props.mode == "add"
                   ? mozL10n.get("add_contact_button")
                   : mozL10n.get("edit_contact_title")), 
          React.createElement("label", null, mozL10n.get("edit_contact_name_label")), 
          React.createElement("input", {ref: "name", required: true, pattern: "\\s*\\S.*", type: "text", 
                 className: cx({pristine: this.state.pristine}), 
                 valueLink: this.linkState("name")}), 
          React.createElement("label", null, mozL10n.get("edit_contact_email_label")), 
          React.createElement("input", {ref: "email", type: "email", required: phoneOrEmailRequired, 
                 className: cx({pristine: this.state.pristine}), 
                 valueLink: this.linkState("email")}), 
          React.createElement("label", null, mozL10n.get("new_contact_fxos_phone_placeholder")), 
          React.createElement("input", {ref: "tel", type: "tel", required: phoneOrEmailRequired, 
                 className: cx({pristine: this.state.pristine}), 
                 valueLink: this.linkState("tel")}), 
          React.createElement(ButtonGroup, null, 
            React.createElement(Button, {additionalClass: "button-cancel", 
                    caption: mozL10n.get("cancel_button"), 
                    onClick: this.handleCancelButtonClick}), 
            React.createElement(Button, {additionalClass: "button-accept", 
                    caption: this.props.mode == "add"
                             ? mozL10n.get("add_contact_button")
                             : mozL10n.get("edit_contact_done_button"), 
                    onClick: this.handleAcceptButtonClick})
          )
        )
      );
    }
  });

  return {
    ContactsList: ContactsList,
    ContactDetailsForm: ContactDetailsForm,
    _getPreferred: getPreferred,
    _setPreferred: setPreferred,
  };
})(_, document.mozL10n);
