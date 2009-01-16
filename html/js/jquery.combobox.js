// dynamic ajax form elements
jQuery(function($) {

$.fn.combobox = function(options) {
	if (!$.fn.combobox.init_done) {
		//alert('do init');
		$('head').append('<style type="text/css" media="all">\ndiv.combobox-wrapper { display: inline; border: 0px; padding: .3em; position: relative; }\nselect.combobox { position: absolute; left: 0; top: 0; }\ninput.combobox { z-index: 5; position: absolute; left: 0; top: 0; border-right: none; }\n</style>');
		$.fn.combobox.init_done = true;
	}
	return $(this).each(function() {
		var opts = $.extend($.fn.combobox.defaults, options);
		// we run this code only for the textbox widget
		// basically all we want to do is find our select widget
		// alert('textbox widget id is: '+$(this).attr('id'));
		//alert('check for id'+$(this).attr('id'));
		if (!$(this).attr('id')) return;
		//alert('check for select type');
		if (!this.type == 'select') return;
		//alert('check for parent wrapper class');
		if ($(this).parent().attr('class') == 'combobox-wrapper') return;
		combobox_setup(this);
	});
};
$.fn.combobox.defaults = {
};
$.fn.combobox.init_done = false;

function random_string(count) {
	var s = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
	var l = s.length;
	var r = '';
	for (var i=0;i<count;i++) {
		r += s[Math.floor(Math.random()*(l+1))];
	}
	return r;
}

function select_has_option_text(select, val) {
	for (var i=0; i<select.options.length; i++) {
		if (select.options[i].text == val) {
			return i;
		}
	}
	return null;
}

function select_has_option_value(select, val) {
	for (var i=0; i<select.options.length; i++) {
		if (select.options[i].value == val) {
			return i;
		}
	}
	return null;
}

// meat and potatoes here.  We have both text and select
// jquery objects and can set them up as we please
function combobox_setup(select) {
	var $select = $(select);
	// create a new textbox that the user can type in
	// and a hidden value that will store the final result
	// Also rename the select box so it can be replaced
	// by the hidden value on post
	// while we let the given textbox be a user scratch area
	var hiddenname = $select.attr('name');
	var selectid = $select.attr('id');
	var textname = hiddenname + '_t_' + random_string(8);
	var selectname = hiddenname + '_s_' + random_string(8);
	var textvalue = $select.val();
	$select.attr('id', selectid+'_select');
	$select.attr('name', selectname);
	if (!$select.hasClass('combobox')) {
		$select.addClass('combobox');
	}

	// TODO: add user specified classes to combobox fields for theme overriding
	// wrap a div around the select and new widgets for positioning purposes
	var $wrapper = $select.wrap('<div class="combobox-wrapper" id="'+selectid+'_combobox"></div>');
	var $text = $select.before('<input type="text" class="combobox" name="'+textname+'" value="'+textvalue+'" />').prev();
	$text.attr('id', selectid);
	var $hidden = $text.before('<input type="hidden" name="'+hiddenname+'" value="'+textvalue+'" />').prev();

	var text_css_defaults = {'width': $select.width() - 17 };
	$text.css(text_css_defaults);

	// when the select box changes values, update the text field
	// to be the text string in the select box and the hidden field
	// to be the value of the select box
	$select.change(function() {
		// alert($select.attr('id')+":"+$select.val());
		// alert($text.attr('id')+":"+$text.val());
		selectext = this.options[this.selectedIndex].text;
		$hidden.val($(this).val());
		$text.val(selectext);
		$text.focus();
	});

	// when the user presses the up down keys, change the selection
	// and update the textbox (which updates the hidden as well)
	$text.keydown(function(e) {
		if (e.which == 38) {
			// up key was pressed
			if (select.selectedIndex > 0) {
				select.selectedIndex = select.selectedIndex - 1;
				$select.change();
			}
		} else if (e.which == 40) {
			// down key was pressed
			if (select.selectedIndex < (select.options.length - 1)) {
				select.selectedIndex = select.selectedIndex + 1;
				$select.change();
			}
		}
	});
	// when the user types something in the textbox, search the
	// select box for a matching string and update the select box
	// to match (which in turn updates the others)
	$text.keypress(function(e) {
		if (e.which) {
			newval = $(this).val() + String.fromCharCode(e.which);
			if ((selind = select_has_option_text(select, newval)) != null) {
				select.selectedIndex = selind;
				$select.change();
				// we return false because we are directly setting the textbox's value
				// and don't want the newly pressed key to be appended to the new value
				return false;
			} else if ((selind = select_has_option_value(select, newval)) != null) {
				select.selectedIndex = selind;
			}
		}
	});

	$text.change(function() {
		$hidden.val($(this).val());
	});
}

$(function() {
	var now = new Date();
	Math.random(now.getMilliseconds());
});

});
