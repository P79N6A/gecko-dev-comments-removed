



































 




var EXPORTED_SYMBOLS = ["FormData"];

const CC = Components.classes;
const CI = Components.interfaces;
const CU = Components.utils;

CU.import("resource://tps/logger.jsm");

let formService = CC["@mozilla.org/satchel/form-history;1"]
                  .getService(CI.nsIFormHistory2);







let FormDB = {
  







  makeGUID: function makeGUID() {
    
    const code =
      "!()*-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~";

    let guid = "";
    let num = 0;
    let val;

    
    for (let i = 0; i < 10; i++) {
      
      if (i == 0 || i == 5)
        num = Math.random();

      
      num *= 70;
      val = Math.floor(num);
      guid += code[val];
      num -= val;
    }

    return guid;
  },

  











  insertValue: function (fieldname, value, us) {
    let query = this.createStatement(
      "INSERT INTO moz_formhistory " +
      "(fieldname, value, timesUsed, firstUsed, lastUsed, guid) VALUES " +
      "(:fieldname, :value, :timesUsed, :firstUsed, :lastUsed, :guid)");
    query.params.fieldname = fieldname;
    query.params.value = value;
    query.params.timesUsed = 1;
    query.params.firstUsed = us;
    query.params.lastUsed = us;
    query.params.guid = this.makeGUID();
    query.execute();
    query.reset();
  },

  








  updateValue: function (id, newvalue) {
    let query = this.createStatement(
      "UPDATE moz_formhistory SET value = :value WHERE id = :id");
    query.params.id = id;
    query.params.value = newvalue;
    query.execute();
    query.reset();
  },

  











  getDataForValue: function (fieldname, value) {
    let query = this.createStatement(
      "SELECT id, lastUsed, firstUsed FROM moz_formhistory WHERE " +
      "fieldname = :fieldname AND value = :value");
    query.params.fieldname = fieldname;
    query.params.value = value;
    if (!query.executeStep())
      return null;

    return {
      id: query.row.id,
      lastUsed: query.row.lastUsed,
      firstUsed: query.row.firstUsed
    };
  },

  








  createStatement: function createStatement(query) {
    try {
      
      return formService.DBConnection.createStatement(query);
    }
    catch(ex) {
      
      formService.DBConnection.executeSimpleSQL(
        "ALTER TABLE moz_formhistory ADD COLUMN guid TEXT");
      formService.DBConnection.executeSimpleSQL(
        "CREATE INDEX IF NOT EXISTS moz_formhistory_guid_index " +
        "ON moz_formhistory (guid)");
    }

    
    return formService.DBConnection.createStatement(query);
  }
};






function FormData(props, usSinceEpoch) {
  this.fieldname = null;
  this.value = null;
  this.date = 0;
  this.newvalue = null;
  this.usSinceEpoch = usSinceEpoch;

  for (var prop in props) {
    if (prop in this)
      this[prop] = props[prop];
  }
}




FormData.prototype = {
  








  hours_to_us: function(hours) {
    return this.usSinceEpoch + (hours * 60 * 60 * 1000 * 1000);
  },

  







  Create: function() {
    Logger.AssertTrue(this.fieldname != null && this.value != null,
      "Must specify both fieldname and value");

    let formdata = FormDB.getDataForValue(this.fieldname, this.value);
    if (!formdata) {
      
      FormDB.insertValue(this.fieldname, this.value,
                         this.hours_to_us(this.date));
    }
    else {
      



    }
  },

  







  Find: function() {
    let formdata = FormDB.getDataForValue(this.fieldname, this.value);
    let status = formdata != null;
    if (status) {
      








        this.id = formdata.id;
    }
    return status;
  },

  







  Remove: function() {
    

    formService.removeEntry(this.fieldname, this.value);
    return true;
  },
};
