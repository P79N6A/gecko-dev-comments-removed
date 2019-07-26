






if (typeof Mozilla == 'undefined') {
	var Mozilla = {};
}

(function($) {
  'use strict';

	
	if (typeof Mozilla.UITour == 'undefined') {
		Mozilla.UITour = {};
	}

	var themeIntervalId = null;
	function _stopCyclingThemes() {
		if (themeIntervalId) {
			clearInterval(themeIntervalId);
			themeIntervalId = null;
		}
	}


	function _sendEvent(action, data) {
		var event = new CustomEvent('mozUITour', {
			bubbles: true,
			detail: {
				action: action,
				data: data || {}
			}
		});
		console.log("Sending mozUITour event: ", event);
		document.dispatchEvent(event);
	}

	Mozilla.UITour.DEFAULT_THEME_CYCLE_DELAY = 10 * 1000;

	Mozilla.UITour.showHighlight = function(target) {
		_sendEvent('showHighlight', {
			target: target
		});
	};

	Mozilla.UITour.hideHighlight = function() {
		_sendEvent('hideHighlight');
	};

	Mozilla.UITour.showInfo = function(target, title, text) {
		_sendEvent('showInfo', {
			target: target,
			title: title,
			text: text
		});
	};

	Mozilla.UITour.hideInfo = function() {
		_sendEvent('hideInfo');
	};

	Mozilla.UITour.previewTheme = function(theme) {
		_stopCyclingThemes();

		_sendEvent('previewTheme', {
			theme: JSON.stringify(theme)
		});
	};

	Mozilla.UITour.resetTheme = function() {
		_stopCyclingThemes();

		_sendEvent('resetTheme');
	};

	Mozilla.UITour.cycleThemes = function(themes, delay, callback) {
		_stopCyclingThemes();

		if (!delay) {
			delay = Mozilla.UITour.DEFAULT_THEME_CYCLE_DELAY;
		}

		function nextTheme() {
			var theme = themes.shift();
			themes.push(theme);

			_sendEvent('previewTheme', {
				theme: JSON.stringify(theme),
				state: true
			});

			callback(theme);
		}

		themeIntervalId = setInterval(nextTheme, delay);
		nextTheme();
	};

	Mozilla.UITour.addPinnedTab = function() {
		_sendEvent('addPinnedTab');
	};

	Mozilla.UITour.removePinnedTab = function() {
		_sendEvent('removePinnedTab');
	};

	Mozilla.UITour.showMenu = function(name) {
		_sendEvent('showMenu', {
			name: name
		});
	};
})();