




'use strict';
const {
  utils: Cu
} = Components;

this.EXPORTED_SYMBOLS = [
  'ManifestObtainer',
  'ManifestProcessor'
];


for (let symbl of EXPORTED_SYMBOLS) {
  Cu.import(`resource://gre/modules/${symbl}.js`);
}
