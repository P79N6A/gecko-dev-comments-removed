(function(){

	
	var doDelay = typeof window != "undefined" && window.location && window.location.search == "?delay";

	
	var pre = typeof document != "undefined" && document.getElementsByTagName &&
		document.getElementsByTagName("pre")[0];
	
	
	
	var numTests = 100, timeout = !doDelay ? 20000 : 4500;

	var title, testName, summary = 0, queue = [];

	
	
	this.startTest = function(name){
		testName = name;

		if ( typeof onlyName == "undefined" )
			log([ "__start_report", testName ]);
	};

	
	this.endTest = function(){
		
		queue.push(function(){
			if ( typeof onlyName == "undefined" ) {
				log([ "__summarystats", summary ]);
				log([ "__end_report" ]);

			
			} else if ( typeof tpRecordTime != "undefined" )
				tpRecordTime( summary );

			
			else
				log([ "__start_report" + summary + "__end_report" ]);

			
			if ( typeof goQuitApplication != "undefined" )
				goQuitApplication();
		});

		
		dequeue();
	};

	
	
	
	
	this.test = function(name, num, fn){
		
		if ( typeof onlyName == "undefined" || (name == onlyName && num == onlyNum) ) {
			
			queue.push(function(){
				doTest( name, num, fn );
			});
		}
	};

	
	function doTest(name, num, fn){
		title = name;
		var times = [], start, diff, sum = 0, min = -1, max = -1,
			median, std, mean;
		
		if ( !fn ) {
			fn = num;
			num = '';
		}

		
		try {
			
			var testStart = (new Date()).getTime();
			
			for ( var i = 0; i < numTests; i++ ) {
				start = (new Date()).getTime();
				fn();
				var cur = (new Date()).getTime();
				diff = cur - start;
				
				
				sum += diff;
				
				
				if ( min == -1 || diff < min )
				  min = diff;
				  
				
				if ( max == -1 || diff > max )
				  max = diff;
				  
				
				times.push( diff );
					
				
				if ( timeout > 0 && cur - testStart > timeout )
					break;
			}
		} catch( e ) {
			if ( typeof onlyName == "undefined" )
				return log( [ title, num, NaN, NaN, NaN, NaN, NaN ] );
			else
				return log( [ "__FAIL" + e + "__FAIL" ] );
		}

		
		mean = sum / times.length;
		
		
		summary += mean;
		
		
		times = times.sort(function(a,b){
			return a - b;
		});
		
		if ( times.length % 2 == 0 )
			median = (times[(times.length/2)-1] + times[times.length/2]) / 2;
		else
			median = times[(times.length/2)];
		
		
		var variance = 0;
		for ( var i = 0; i < times.length; i++ )
			variance += Math.pow(times[i] - mean, 2);
		variance /= times.length - 1;
		
		
		std = Math.sqrt( variance );

		if ( typeof onlyName == "undefined" )
			log( [ title, num, median, mean, min, max, std ] );

		
		dequeue();
	};

	
	function dequeue(){
		
		
		if ( doDelay && typeof setTimeout != "undefined" )
			setTimeout(function(){
				queue.shift()();
			}, 13);

		
		else
			queue.shift()();
	}

	
	function log( arr ) {
		
		if ( pre )
			pre.innerHTML += arr.join(":") + "\n";

		
		else if ( typeof document != "undefined" && document.write && !doDelay )
			document.write( arr.join(":") + "\n" );

		
		else if ( typeof window == "undefined" && typeof print != "undefined" )
			print( arr.join(":") );
	}

})();
