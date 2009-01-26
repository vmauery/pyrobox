/* File: pyrobox.js
 * Description: Common functions used on pyrobox pages
 * Author: Vernon Mauery <vernon@mauery.com>
 */

var url = document.location + ""; url = url.replace(/http(s)?:\/\/[^\/]+/, '');
(function($) {$(function() {
	/* DEBUG jQuery.post override.  Comment out for production use */
	$.post = function ( url, data, callback, type ) {
		if ( jQuery.isFunction( data ) ) {
			callback = data;
			data = {};
		}

		var callback_error = function(request, textStatus, errorThrown) {
			/* replace the current page with the html error page */
			var re = /.*<html[^>]*>(.*)<\/html>.*/
			var html = request.responseText.replace(re, '\1');
			alert("ajax error!\nhttp-status: "+request.status+"\ntextStatus: "+textStatus+"\nerrorThrown: "+errorThrown);
			if (html.length > 0) {
				$('body').append('<div class="popup">'+html+'</div>');
				$('.popup').unbind('click').click(function(e) {
					$(this).remove();
				});
			}
		};
		return jQuery.ajax({
			type: "POST",
			url: url,
			data: data,
			success: callback,
			error: callback_error,
			dataType: type
		});
	}
	/* end DEBUG jQuery.post */
	/* DEBUG dump */
	function _dump(obj, name, depth) {
		var MAX_DUMP_DEPTH = 3;
		var indent = "  ";
		if (!depth) depth = 0;
		if (depth > MAX_DUMP_DEPTH) {
			return indent + name + ": <Maximum Depth Reached>\n";
		}
		if (typeof obj == "object") {
			var child = null;
			var output = indent + name + "\n";
			indent += "\t";
			for (var item in obj)
			{
				try {
					child = obj[item];
				} catch (e) {
					child = "<Unable to Evaluate>";
				}
				if (typeof child == "object") {
					output += _dump(child, item, depth + 1);
				} else {
					output += indent + item + ": " + child + "\n";
				}
			}
			return output;
		} else {
			return obj;
		}
	}
	$.dump = function(obj) {
		var v = "<pre>"+_dump(obj, '')+"</pre>";
		$('body').append('<div class="popup">'+v+'</div>');
		$('.popup').unbind('click').click(function(e) {
			$(this).remove();
		});
	}
	/* end DEBUG dump */

	
	$.postJSON = function( url, data, callback ) {
		return jQuery.post(url, data, callback, "json");
	}

/* common dynamic forms code */
	function js_links() {
		/* set the on click for delete links */
		$(".jslink").filter(".delete").unbind('click').click(function() {
			delete_row(this);
		});
		/* set the on click for edit links */
		$(".jslink").filter(".edit").unbind('click').click(function() {
			edit_row(this);
		});
	}
	function row_hover() {
		$("tr").filter(".edit").each(function() {
			$(this).children().not(":last").unbind('click').click(function() {
				edit_row(this);
			});
		});
	}
	/* remove a row from the table of rows */
	function delete_row(obj) {
		var $row = $(obj).parent().parent();
		var row = '';
		id = $(obj).attr("id").replace(/delete_([0-9]+)_.*/, '$1');
		var table = $(obj).attr("id").replace(/delete_([0-9]+)_(.*)/, '$2');
		$.post(url, {'delete':id, 'table':table}, function(data, textStatus) {
			if (textStatus == "success") {
				$row.remove();
			}
		});
	}
	/* add an edit form for editing rows in place */
	function edit_row(obj) {
		var $row = $(obj).parents("tr").eq(0);
		var form_el_id = $(obj).attr('id').replace(/row_\d+-(.*)/, '#id_$1');
		var row = '';
		id = $(obj).attr("id").replace(/row_([0-9]+)-.*/, '$1');
		var table = $(obj).attr("id").replace(/row_([0-9]+)-(.*)/, '$2');
		$.post(url, {'edit':id, 'table':table}, function(data, textStatus) {
			if (textStatus == "success") {
				$row.after(data);
				$row.hide();
				$(".combobox").combobox();
				var $form = $("#form_"+id);
				var $selected = $form.find(form_el_id);
				$selected.trigger('focus');
				$("#cancel_"+id).click(function() {
					$form.remove();
					$row.show();
				});
				function post_em(element) {
					/* gather the form elements and post them */
					var values = {'id':id, 'table':table}
					var v = "";
					$(element).parents("#form_"+id).children().children().each(function() {
						if (this.type == 'button') return;
						if ($(this).attr('class') == 'combobox-wrapper') {
							$(this).children().each(function() {
								values[this.name] = $(this).val();
								v = v + "values["+this.name+"] = "+$(this).val() + "\n";
							});
						} else {
							values[this.name] = $(this).val();
							v = v + "values["+this.name+"] = "+$(this).val() + "\n";
						}
					});
					
					$.post(url, values, function(data, textStatus) {
						$form.remove();
						$row.after(data);
						$row.remove();
						$(".combobox").combobox();
						/* set the on click for edit and delete links */
						js_links();
						row_hover();
					});
				}
				$("#save_"+id).click(function() { post_em(this); });
				$form.children().keypress(function(e) {
					if (e.keyCode == 27) {
						$form.remove();
						$row.show();
					} else if (e.keyCode == 13) {
						$(this).trigger('change');
						post_em(this);
					}
					return true;
				});
			}
		});
	}
	$(".combobox").combobox();
	js_links();
	row_hover();

	function arr_to_obj(a) {
		var o = {};
		for(var i=0;i<a.length;i++) {
			o[a[i]]=null;
		}
		return o;
	};

	function filter_attributes(attrs, ok_attrs) {
		// check for baddies in the attributes
		var ok_attrs = arr_to_obj(ok_attrs.concat(["alt", "class", "style", "readonly", "tabindex", "onselect", "disabled", "accesskey", "onfocus", "onblur", "onchange", "onclick", "ondblclick", "onmousedown", "onmouseup", "onmouseover", "onmousemove", "onmouseout", "onkeypress", "onkeydown", "onkeyup"]));
		var attributes = {};
		for (a in attrs) {
			if (typeof(attrs[a]) == 'string' && a in ok_attrs) {
				attributes[a] = attrs[a];
			}
		}
		return attributes;
	}

	function render_attributes(element) {
		// FIXME: html escape attr value
		var ret = "";
		for (attr in element.attributes) {
			ret += attr + '="' + element.attributes[attr] + '" ';
		}
		return ret;
	}

	function render_children(elements, values) {
		if (values == null) values = {}
		var i, ret = "", value;
		for (i=0; i<elements.length; i++) {
			el = elements[i];
			if (el.type == 'fieldset')
				elements[i].value = values;
			else if (el.name in values)
				elements[i].value = eval('values.'+el.name);
			else if (!("value" in elements[i]))
				elements[i].value = null;

			if (typeof el.type == 'string' &&
					eval('typeof $.render_' + el.type) == 'function') {
				ret += eval('$.render_' + elements[i].type + '(elements[i])');
			}
		}
		return ret;
	}

	function element_common(element, attrs) {
		element.attributes = filter_attributes(element, attrs);
		var required_attrs = arr_to_obj(["label", "description", "name", "value"]);
		for (attr in required_attrs) {
			if (!(attr in element)) {
				eval('element.'+attr+' = ""');
			}
		}
		if (element.id == null) {
			element.id = 'id_'+element.name;
		}
		if (element.value == null && element.default_value == null) {
			element.value = "";
		} else if (element.value == null && element.default_value != null) {
			element.value = element.default_value;
		}
	}

	$.render_form = function(form, values) {
		if (typeof form == 'array' && values == null) {
			values = form[1];
			form = form[0];
		}
		var attributes = ["action","method","enctype","onsubmit"];
		element_common(form, attributes);
		return '<form '+render_attributes(form)+'>'+render_children(form.elements, values)+'</form>\n';
	};

	$.render_fieldset = function(el) {
		var attributes = [];
		element_common(form, attributes);
		return '<fieldset '+render_attributes(el)+'><legend>'+el.label+'</legend>\n'+render_children(el.elements, el.value)+'</fieldset>\n';
	};

	$.render_textbox = function(el) {
		var attributes = ["maxlength", ];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<span class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></span>\n'+

			'\t<span class="edit"><input name="'+el.name+'" id="'+el.id+'" type="text" value="'+el.value+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>';
	};

	$.render_textarea = function(el) {
		var attributes = ["rows", "cols", ];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<span class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></span>\n'+

			'\t<span class="edit"><textarea name="'+el.name+'" id="'+el.id+'" '+render_attributes(el)+'>'+el.value+'</textarea></span><span class="error"></span>\n</div>';
	};

	$.render_password = function(el) {
		var attributes = ["maxlength", ];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<span class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></span>\n'+

			'\t<span class="edit"><input name="'+el.name+'" id="'+el.id+'" type="'+el.type+'" value="'+el.value+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>';
	};

	$.render_checkbox = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<span class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></span>\n'+
			'\t<span class="edit"><input type="checkbox" name="'+el.name+'" id="'+el.id+'" value="'+el.value+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>\n';
	};

	function render_radio_options(el) {
		var i=0, ret="";
		var opt;
		var attributes = [];
		for (i=0; i<el.options.length; i++) {
			opt = el.options[i];
			element_common(opt, attributes);
			checked = (el.value == opt.value) ? 'checked="checked"' : '';
			ret += '<li><label><input type="radio" '+checked+' id="'+el.id+'_'+i+'" value="'+opt.value+'" name="'+el.name+'" '+render_attributes(el)+'/>'+opt.label+'</label></li>';
		}
		return ret;
	}

	$.render_radios = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'<span class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></span>\n'+
			'<span class="edit"><ul>\n'+
			render_radio_options(el) +
			'</ul></span><span class="error"></span>\n</div>';
	};

	function render_select_options(el) {
		var i, ret="", opt;
		for (i=0; i<el.options.length; i++) {
			opt = el.options[i];
			selected = (el.value == opt.value) ? 'selected="selected"' : '';
			ret += '<option '+selected+' value="'+opt.value+'">'+opt.label+'</option>';
		}
		return ret;
	}

	$.render_select = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'<span class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></span>\n'+
			'<select name="'+el.name+'">\n'+
			render_select_options(el) +
			'</select><span class="error"></span>\n</div>';
	};

	$.render_combobox = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'<span class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></span>\n'+
			'<span class="edit"><ul>\n'+
			render_select_options(el) +
			'</ul></span><span class="error"></span>\n</div>';
	};

	$.render_button = function(el) {
		var attributes = [];
		var button_type = ("button_type" in el) ? el.button_type : "button";
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<span class="edit"><input type="'+button_type+'" name="'+el.name+'" id="'+el.id+'" value="'+el.value+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>\n';
	};

	$.render_file = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<span class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></span>\n'+

			'\t<span class="edit"><input name="'+el.name+'" id="'+el.id+'" type="'+el.type+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>';
	};

	$.render_hidden = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<input type="hidden" name="'+el.name+'" id="'+el.id+'" value="'+el.value+'"/>\n';
	};

}); })(jQuery);

