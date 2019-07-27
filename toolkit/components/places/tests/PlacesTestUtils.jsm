"use strict";

this.EXPORTED_SYMBOLS = [
  "PlacesTestUtils",
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");


this.PlacesTestUtils = Object.freeze({
  

















  addVisits(placeInfo) {
    return new Promise((resolve, reject) => {
      let places = [];
      if (placeInfo instanceof Ci.nsIURI) {
        places.push({ uri: placeInfo });
      }
      else if (Array.isArray(placeInfo)) {
        places = places.concat(placeInfo);
      } else {
        places.push(placeInfo)
      }

      
      let now = Date.now();
      for (let place of places) {
        if (typeof place.title != "string") {
          place.title = "test visit for " + place.uri.spec;
        }
        place.visits = [{
          transitionType: place.transition === undefined ? Ci.nsINavHistoryService.TRANSITION_LINK
                                                             : place.transition,
          visitDate: place.visitDate || (now++) * 1000,
          referrerURI: place.referrer
        }];
      }

      PlacesUtils.asyncHistory.updatePlaces(
        places,
        {
          handleError: function AAV_handleError(resultCode, placeInfo) {
            let ex = new Components.Exception("Unexpected error in adding visits.",
                                              resultCode);
            reject(ex);
          },
          handleResult: function () {},
          handleCompletion: function UP_handleCompletion() {
            resolve();
          }
        }
      );
    });
  },
});
