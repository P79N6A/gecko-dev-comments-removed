setup({explicit_done: true});

var debug = document.getElementById("debug");

function debug_print (x) {


}

var starting_elements = {};

function check_list_subset_of_targetlist(list, list_name, targetlist, targetlist_name) {
	var exist_in_targetlist;
	for(i=0; i<list.length; i++) {
		exist_in_targetlist=false;
		for(j=0; j<targetlist.length; j++)
			if(list.item(i).identifier==targetlist.item(j).identifier)
				exist_in_targetlist=true;

		assert_true(exist_in_targetlist, list_name + ".item("+i+") exists in " + targetlist_name);
	}
}

function check_list_subset_of_two_targetlists(list, list_name, targetlist1, targetlist1_name, targetlist2, targetlist2_name) {
	var exist_in_targetlists;
	for(i=0; i<list.length; i++) {
		exist_in_targetlists=false;
		for(j=0; j<targetlist1.length; j++)
			if(list.item(i).identifier==targetlist1.item(j).identifier)
				exist_in_targetlists=true;

		if(!exist_in_targetlists)
			for(j=0; j<targetlist2.length; j++)
				if(list.item(i).identifier==targetlist2.item(j).identifier)
					exist_in_targetlists=true;

		assert_true(exist_in_targetlists, list_name + ".item("+i+") exists in " + targetlist1_name + " or " + targetlist2_name);
	}

}

function is_at_least_one_item_in_targetlist(list, targetlist) {
	for(i=0; i<list.length; i++)
		for(j=0; j<targetlist.length; j++)
			if(list.item(i).identifier==targetlist.item(j).identifier)
				return true;

	return false;
}

function check_no_item_in_targetlist(list, list_name, targetlist, targetlist_name) {
	for(i=0; i<list.length; i++)
		for(j=0; j<targetlist.length; j++) {
			assert_false(list.item(i).identifier==targetlist.item(j).identifier, list_name + ".item("+i+") exists in " + targetlist_name);
			return;
		}
}

function check_targets(list, target) {
	for(i=0; i<list.length; i++)
		assert_true(list.item(i).target==target, "item(" + i + ").target is element receiving event");
}

function check_starting_elements(list) {
	for (i=0; i<list.length; i++) {
		assert_equals(list.item(i).target, starting_elements[list.item(i).identifier], "item(" + i + ").target matches starting element");
	}
}

function run() {
	var target0 = document.getElementById("target0");
	var target1 = document.getElementById("target1");

	var test_touchstart = async_test("touchstart event received");
	var test_touchmove = async_test("touchmove event received");
	var test_touchend = async_test("touchend event received");
	var test_mousedown = async_test("Interaction with mouse events");

	var touchstart_received = 0;
	var touchmove_received = 0;
	var touchend_received = 0;
	var touchstart_identifier;

	
	var last_touches;
	var last_targetTouches={};
	var last_changedTouches={};

	on_event(window, "touchstart", function onTouchStart(ev) {
		
		if(ev.target != target0 && ev.target != target1 )
			return;

		ev.preventDefault();

		if(!touchstart_received) {
			
			test_touchstart.step(function() {
				assert_true(touchmove_received==0, "touchstart precedes touchmove");
				assert_true(touchend_received==0, "touchstart precedes touchend");
			});
			test_touchstart.done();
			test_mousedown.done(); 
		}
		touchstart_received++;

		
		test(function() {
			assert_true(ev.changedTouches.length >= 1, "changedTouches.length is at least 1");
			assert_true(ev.changedTouches.length <= ev.touches.length, "changedTouches.length is smaller than touches.length");
			check_list_subset_of_targetlist(ev.changedTouches, "changedTouches", ev.touches, "touches");
		}, "touchstart #" + touchstart_received + ": changedTouches is a subset of touches");

		
		test(function() {
			assert_true(ev.targetTouches.length >= 1, "targetTouches.length is at least 1");
			assert_true(ev.targetTouches.length <= ev.touches.length, "targetTouches.length is smaller than touches.length");
			check_list_subset_of_targetlist(ev.targetTouches, "targetTouches", ev.touches, "touches");
		}, "touchstart #" + touchstart_received + ": targetTouches is a subset of touches");

		
		test(function() {
			check_targets(ev.targetTouches, ev.target);
		}, "touchstart #" + touchstart_received + ": targets of targetTouches are correct");

		
		test(function() {
			assert_true(ev.touches.length >= 1, "touches.length is at least 1");
		}, "touchstart #" + touchstart_received + ": touches.length is valid");

		if(touchstart_received == 1) {
			
			test(function() {
				assert_true(ev.targetTouches.length <= ev.changedTouches.length, "targetTouches.length is smaller than changedTouches.length");
				check_list_subset_of_targetlist(ev.targetTouches, "targetTouches", ev.changedTouches, "changedTouches");
			}, "touchstart #" + touchstart_received + ": targetTouches is a subset of changedTouches");

			
			test(function() {
				assert_true(ev.touches.length==ev.changedTouches.length, "touches and changedTouches have the same length");
			}, "touchstart #" + touchstart_received + ": touches and changedTouches have the same length");
		} else {
			
			test(function() {
				var diff_in_targetTouches = ev.targetTouches.length - (last_targetTouches[ev.target.id] ? last_targetTouches[ev.target.id].length : 0);
				assert_true(diff_in_targetTouches > 0, "targetTouches.length is larger than last received targetTouches.length");
				assert_true(diff_in_targetTouches <= ev.changedTouches.length, "change in targetTouches.length is smaller than changedTouches.length");
			}, "touchstart #" + touchstart_received + ": change in targetTouches.length is valid");

			
			test(function() {
				assert_true(is_at_least_one_item_in_targetlist(ev.targetTouches, ev.changedTouches), "at least one item of targetTouches is in changedTouches");
			}, "touchstart #" + touchstart_received + ": at least one targetTouches item in changedTouches");

			
			test(function() {
				var diff_in_touches = ev.touches.length - last_touches.length;
				assert_true(diff_in_touches > 0, "touches.length is larger than last received touches.length");
				assert_true(diff_in_touches == ev.changedTouches.length, "change in touches.length equals changedTouches.length");
			}, "touchstart #" + touchstart_received + ": change in touches.length is valid");

			
			test(function() {
				check_list_subset_of_two_targetlists(ev.touches, "touches", ev.changedTouches, "changedTouches", last_touches, "last touches");
			}, "touchstart #" + touchstart_received + ": touches is subset of {changedTouches, last received touches}");
		}

		
		for (i=0; i<ev.changedTouches.length; i++) {
			starting_elements[ev.changedTouches.item(i).identifier] = ev.changedTouches.item(i).target;
		}

		last_touches = ev.touches;
		last_targetTouches[ev.target.id] = ev.targetTouches;
		last_changedTouches = {};	
	});

	on_event(window, "touchmove", function onTouchMove(ev) {
		
		if(ev.target != target0 && ev.target != target1 )
			return;

		ev.preventDefault();

		
		test_touchmove.step(function() {
			assert_true(touchstart_received>0, "touchmove follows touchstart");
			
		});
		test_touchmove.done();

		touchmove_received++;

		
		if(touchmove_received<6) {
			
			test(function() {
				assert_true(ev.changedTouches.length >= 1, "changedTouches.length is at least 1");
				assert_true(ev.changedTouches.length <= ev.touches.length, "changedTouches.length is smaller than touches.length");
				check_list_subset_of_targetlist(ev.changedTouches, "changedTouches", ev.touches, "touches");
			}, "touchmove #" + touchmove_received + ": changedTouches is a subset of touches");

			
			test(function() {
				assert_true(ev.targetTouches.length >= 1, "targetTouches.length is at least 1");
				assert_true(ev.targetTouches.length <= ev.touches.length, "targetTouches.length is smaller than touches.length");
				check_list_subset_of_targetlist(ev.targetTouches, "targetTouches", ev.touches, "touches");
			}, "touchmove #" + touchmove_received + ": targetTouches is a subset of touches");

			
			test(function() {
				assert_true(is_at_least_one_item_in_targetlist(ev.targetTouches, ev.changedTouches), "at least one item of targetTouches is in changedTouches");
			}, "touchmove #" + touchmove_received + ": at least one targetTouches item in changedTouches");

			
			test(function() {
				check_targets(ev.targetTouches, ev.target);
			}, "touchmove #" + touchmove_received + ": targets of targetTouches are correct");

			
			test(function() {
				assert_true(ev.touches.length==last_touches.length, "length of touches is same as length of last received touches");
				check_list_subset_of_targetlist(ev.touches, "touches", last_touches, "last received touches");
			}, "touchmove #" + touchmove_received + ": touches must be same as last received touches");

			
			check_starting_elements(ev.changedTouches);
		}

		last_touches = ev.touches;
		last_targetTouches[ev.target.id] = ev.targetTouches;
		last_changedTouches = {};	
	});

	on_event(window, "touchend", function onTouchEnd(ev) {
		
		if(ev.target != target0 && ev.target != target1 )
			return;

		test_touchend.step(function() {
			assert_true(touchstart_received>0, "touchend follows touchstart");
		});
		test_touchend.done();

		touchend_received++;

		debug_print("touchend #" + touchend_received + ":<br>");
		debug_print("changedTouches.length=" + ev.changedTouches.length + "<br>");
		debug_print("targetTouches.length=" + ev.targetTouches.length + "<br>");
		debug_print("touches.length=" + ev.touches.length + "<br>");
		for(i=0; i<ev.changedTouches.length; i++)
			debug_print("changedTouches.item(" + i + ").target=" + ev.changedTouches.item(i).target.id + "<br>");

		
		test(function() {
			assert_true(ev.changedTouches.length >= 1, "changedTouches.length is at least 1");
		}, "touchend #" + touchend_received + ": length of changedTouches is valid");

		
		test(function() {
			check_list_subset_of_targetlist(ev.changedTouches, "changedTouches", last_touches, "last received touches");
		}, "touchend #" + touchend_received + ": changedTouches is a subset of last received touches");

		
		test(function() {
			check_no_item_in_targetlist(ev.changedTouches, "changedTouches", ev.touches, "touches");
			check_no_item_in_targetlist(ev.changedTouches, "changedTouches", ev.targetTouches, "targetTouches");
		}, "touchend #" + touchend_received + ": no item in changedTouches are in touches or targetTouches");

		
		test(function() {
			var found=false;
			for (i=0; i<ev.changedTouches.length; i++)
				if (ev.changedTouches.item(i).target == ev.target)
					found=true;
			assert_true(found, "at least one item in changedTouches has matching target");
		}, "touchend #" + touchend_received + ": at least one item in changedTouches targeted at this element");

		
		test(function() {
			assert_true(ev.targetTouches.length >= 0, "targetTouches.length is non-negative");
			assert_true(ev.targetTouches.length <= ev.touches.length, "targetTouches.length is smaller than touches.length");
			check_list_subset_of_targetlist(ev.targetTouches, "targetTouches", ev.touches, "touches");
		}, "touchend #" + touchend_received + ": targetTouches is a subset of touches");

		
		test(function() {
			check_targets(ev.targetTouches, ev.target);
		}, "touchend #" + touchend_received + ": targets of targetTouches are correct");

		
		
		
		
		var same_event_as_last = false;
		if (last_changedTouches && last_changedTouches.length==ev.changedTouches.length) {
			same_event_as_last = true; 
			for (i=0; i<last_changedTouches.length; i++) {
				var match = false;
				for (j=0; j<ev.changedTouches.length; j++)
					if (last_changedTouches.item(i) == ev.changedTouches.item(j)) {
						match = true;
						break;
					}
				if (!match)
					same_event_as_last = false;
			}
		}

		if (!same_event_as_last) {
			
			
			
			test(function() {
				assert_true(last_targetTouches[ev.target.id].length > 0, "last received targetTouches.length is not zero");
				var diff_in_targetTouches = last_targetTouches[ev.target.id].length - ev.targetTouches.length;
				debug_print("diff_in_targetTouches=" + diff_in_targetTouches + "<br>");
				assert_true(diff_in_targetTouches > 0, "targetTouches.length is smaller than last received targetTouches.length");
				assert_true(diff_in_targetTouches <= ev.changedTouches.length, "change in targetTouches.length is smaller than changedTouches.length");
			}, "touchend #" + touchend_received + ": change in targetTouches.length is valid");

			
			
			
			
			
			
			test(function() {
				assert_true(last_touches.length > 0, "last received touches.length is not zero");
				var diff_in_touches = last_touches.length - ev.touches.length;
				debug_print("diff_in_touches=" + diff_in_touches + "<br>");
				assert_true(diff_in_touches > 0, "touches.length is smaller than last received touches.length");
				assert_equals(diff_in_touches, ev.changedTouches.length, "change in touches.length equals changedTouches.length");
			}, "touchend #" + touchend_received + ": change in touches.length is valid");
		}

		
		debug_print("touchend #" + touchend_received + ": TA 1.6.4<br>");
		test(function() {
			check_starting_elements(ev.changedTouches);
		}, "touchend #" + touchend_received + ": event dispatched to correct element<br>");

		debug_print("touchend #" + touchend_received + ": saving touch lists<br>");

		last_touches = ev.touches;
		last_targetTouches[ev.target.id] = ev.targetTouches;
		last_changedTouches = ev.changedTouches;

		debug_print("touchend #" + touchend_received + ": done<br>");
		if(ev.touches.length==0)
			done();
	});

	on_event(target0, "mousedown", function onMouseDown(ev) {
		test_mousedown.step(function() {
			assert_true(touchstart_received,
				"The touchstart event must be dispatched before any mouse " +
				"events. (If this fails, it might mean that the user agent does " +
				"not implement W3C touch events at all.)"
			);
		});
		test_mousedown.done();

		if (!touchstart_received) {
			
			
			
			done();
		}
	});
}
