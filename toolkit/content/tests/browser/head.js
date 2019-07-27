"use strict";





function closeFindbarAndWait(findbar) {
  return new Promise((resolve) => {
    if (findbar.hidden)
      return resolve();
    findbar.addEventListener("transitionend", function cont(aEvent) {
      if (aEvent.propertyName != "visibility") {
        return;
      }
      findbar.removeEventListener("transitionend", cont);
      resolve();
    });
    findbar.close();
  });
}
