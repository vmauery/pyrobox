/* File: pyrobox.js
 * Description: Common functions used on pyrobox pages
 * Author: Vernon Mauery <vernon@mauery.com>
 */

var url = document.location + ""; url = url.replace(/http(s)?:\/\/[^\/]+/, '');

function is_array(obj) {
	if (obj == null) return false;
	return obj.constructor == Array;
}
function is_object(obj) {
	//returns true is it is an object
	if (obj == null) return false;
	return obj.constructor == Object;
}

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
	$.dump = function(v, level) {
		return _dump(v, level);
	}

	function _dump(v, level) {
		var dumped_text = "";
		if (!level) level = 0;

		//The padding given at the beginning of the line.
		var level_padding = "";
		for (var j=0;j<level+1;j++) level_padding += "    ";
		if (level > 3) {
			return level_padding+"...\n";
		}

		if (is_object(v)) {
			dumped_text += level_padding+"{\n";
		} else if (is_array(v)) {
			dumped_text += level_padding+"[\n";
		}
		if (typeof v == "object") { //Array/Hashes/Objects 
			try {
			for(var item in v) {
				var value;
					value = v[item];

				if (typeof value == "object") { // not scalar
					dumped_text += level_padding + "'" + item + "':\n";
					dumped_text += _dump(value,level+1);
				} else {
					dumped_text += level_padding + "'" + item + "' => \"" + value + "\"\n";
				}
			}
			} catch (err) {
				dumped_text += "<ERROR dumping object...>\n";
				return dumped_text;
			}
		} else { //Strings/Chars/Numbers etc.
			return v+"\n";
		}
		if (is_object(v)) {
			dumped_text += "\n"+level_padding+"}\n";
		} else if (is_array(v)) {
			dumped_text += "\n"+level_padding+"]\n";
		}
		return dumped_text;
	};

	$.dump_scr = function(obj) {
		var v = "<pre>"+_dump(obj)+"</pre>";
		$('body').append('<div class="popup">'+v+'</div>');
		$('.popup').unbind('click').click(function(e) {
			$(this).remove();
		});
	};
	/* end DEBUG dump */

	
	$.postJSON = function( url, data, callback ) {
		return jQuery.post(url, data, callback, "json");
	};

/* common dynamic forms code */
	function row_hover() {
		$("tr").filter(".edit").each(function() {
			$(this).children().not(":last").unbind('click').click(function() {
				edit_row(this);
			});
		});
	}
	/* remove a row from the table of rows */
	$(".combobox").combobox();
	js_links();
	row_hover();

	var msg_id = 0;
	$.msgbox = function(msg, level) {
		if (level == null)
			level = 'info';
		msg_id++;
		var id = '#msg-'+msg_id;
		$('#content').prepend('<div class="message '+level+'"id="msg-'+msg_id+'">'+msg+'</div>');
		$('#msg-'+msg_id).show('fast', function() {
			setTimeout(function() {$(id).hide('slow');}, 5000);
		}).click(function() {
			$(this).hide('fast');
		});
	};

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
		//$.log($.dump(values));
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

	$.render_textbox = function(el, input_only) {
		var attributes = ["maxlength", ];
		element_common(el, attributes);
		if (input_only != null && input_only) {
			return '<input name="'+el.name+'" id="'+el.id+'" type="text" value="'+el.value+'" '+render_attributes(el)+'/>';
		}
		return '<div class="field-item">\n'+
			'\t<div class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></div>\n'+

			'\t<span class="edit"><input name="'+el.name+'" id="'+el.id+'" type="text" value="'+el.value+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>';
	};

	$.render_textarea = function(el) {
		var attributes = ["rows", "cols", ];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<div class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></div>\n'+

			'\t<span class="edit"><textarea name="'+el.name+'" id="'+el.id+'" '+render_attributes(el)+'>'+el.value+'</textarea></span><span class="error"></span>\n</div>';
	};

	$.render_password = function(el) {
		var attributes = ["maxlength", ];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<div class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></div>\n'+

			'\t<span class="edit"><input name="'+el.name+'" id="'+el.id+'" type="'+el.type+'" value="'+el.value+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>';
	};

	$.render_checkbox = function(el) {
		var attributes = [];
		element_common(el, attributes);
		if (!el.check_value) {
			el.check_value = '1';
		}
		checked = (el.value == el.check_value) ? 'checked="checked" ' : '';
		return '<div class="field-item">\n'+
			'\t<div class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></div>\n'+
			'\t<span class="edit"><input type="checkbox" name="'+el.name+'" id="'+el.id+'" value="'+el.check_value+'" '+checked+render_attributes(el)+'/></span><span class="error"></span>\n</div>\n';
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
			'<div class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></div>\n'+
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

	$.render_select = function(el, input_only) {
		var attributes = [];
		element_common(el, attributes);
		if (input_only) {
			return '<select name="'+el.name+'" id="'+el.id+'">\n'+
			render_select_options(el) +
			'</select><span class="error"></span>\n</div>';
		}
		return '<div class="field-item">\n'+
			'<div class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></div>\n'+
			'<select name="'+el.name+'" id="'+el.id+'">\n'+
			render_select_options(el) +
			'</select><span class="error"></span>\n</div>';
	};

	$.render_combobox = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'<div class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></div>\n'+
			'<span class="edit"><ul>\n'+
			render_select_options(el) +
			'</ul></span><span class="error"></span>\n</div>';
	};

	$.render_button = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<span class="edit"><input type="'+el.type+'" name="'+el.name+'" id="'+el.id+'" value="'+el.value+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>\n';
	};

	$.render_submit = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<span class="edit"><input type="'+el.type+'" name="'+el.name+'" id="'+el.id+'" value="'+el.value+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>\n';
	};

	$.render_file = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<div class="field-item">\n'+
			'\t<div class="label"><label for="'+el.id+'">'+el.label+'</label><div class="description">'+el.description+'</div></div>\n'+

			'\t<span class="edit"><input name="'+el.name+'" id="'+el.id+'" type="'+el.type+'" '+render_attributes(el)+'/></span><span class="error"></span>\n</div>';
	};

	$.render_hidden = function(el) {
		var attributes = [];
		element_common(el, attributes);
		return '<input type="hidden" name="'+el.name+'" id="'+el.id+'" value="'+el.value+'"/>\n';
	};








	/* generic table handling functions */
	var form_or_td_extraction = function(node)  {
		if (node.childNodes.length > 0) {
			if (node.childNodes[0].nodeName == "#text") {
				return node.childNodes[0].nodeValue;
			}
			if (node.childNodes[0].nodeName == "INPUT") {
				return $(node.childNodes[0]).val();
			}
		}
		return node.innerHTML;
	};

	function edit_row(obj) {
		var $row = $(obj).parent().parent();
		var id = $row.attr("id").replace(/row_([0-9]*)_.*/, '$1');
		var table = $row.attr("id").replace(/row_([0-9]*)_(.*)/, '$2');
		var row = table_data[table][id];
		// render the row as an inline form
		var html = draw_form_row(table_form[table], row);
		$row.replaceWith(html);
		$(".tablesorter").trigger("update");
		js_links();
		return;
	}

	function cancel_edit_row(obj) {
		var $row = $(obj).parent().parent();
		var id = $row.attr("id").replace(/row_([0-9]*)_.*/, '$1');
		if (id == "0") {
			$row.remove();
		} else {
			var table = $row.attr("id").replace(/row_([0-9]*)_(.*)/, '$2');
			var row = table_data[table][id];
			// render the row as a table row
			var html = draw_row(table_form[table], row);
			$row.replaceWith(html);
		}
		$(".tablesorter").trigger("update");
		js_links();
		return;
	}

	function save_row(obj) {
		var $row = $(obj).parent().parent();
		var id = $row.attr("id").replace(/row_([0-9]*)_.*/, '$1');
		var table = $row.attr("id").replace(/row_([0-9]*)_(.*)/, '$2');
		var row = table_data[table][id];
		for (var col in row) {
			var val = $('#id_row_'+id+'-'+table+'-'+col).val();
			row[col] = val;
		}
		// post the row
		var post = {table: table};
		for (i in row) {
			post[i] = row[i];
		}
		var url = "/config/?json&row_submit&debug";
		$.postJSON(url, post, function(data) {
			var new_row = data.submitted;
			if (new_row == null) {
				$.msgbox("The row was not saved");
				return;
			}
			table_data[table][new_row.id] = new_row;
			$.msgbox("The row was saved.");
			row = new_row;

			// render the row as an inline form
			var html = draw_row(table_form[table], row);
			$row.replaceWith(html);
			$(".tablesorter").trigger("update");
			js_links();
		});
		return;
	}

	function delete_row(obj) {
		var $row = $(obj).parent().parent();
		var id = $row.attr("id").replace(/row_([0-9]*)_.*/, '$1');
		var table = $row.attr("id").replace(/row_([0-9]*)_(.*)/, '$2');
		var q = "Are you sure you want to delete:\n";
		var row = table_data[table][id];
		var form = table_form[table].elements;
		for (i in form) {
			if (form[i].type == 'hidden') continue;
			q += row[form[i].name]+"\n";
		}
		if (confirm(q)) {
			var url = "/config/?json&row_delete&debug";
			var post = {table: table, id: id};
			$.postJSON(url, post, function(data) {
				$.msgbox("The row was deleted");
				$row.empty();
				$(".tablesorter").trigger("update");
			});
		}
	}

	function add_row(obj) {
		// here, obj is not a link in a td in a tr in a table
		// it is a link outside the table with a parallel id with the table
		var id = $(obj).parent().attr('id');
		id = id.substr(0, id.length - 6);
		var $htable = $('#'+id);
		var form = table_form[table_name_hash[id]];
		var item = {};
		for (var i in form.elements) {
			item[form.elements[i].name] = '';
		}
		item.id = 0;
		table_data[table_name_hash[id]][0] = item;
		
		$htable.children('tbody').eq(0).prepend(draw_form_row(form, item));
		$(".tablesorter").trigger("update");
		js_links();
	}

	function js_links() {
		/* set the on click for delete links */
		$(".jslink").filter(".delete").unbind('click').click(function() {
			delete_row(this);
		});
		/* set the on click for edit links */
		$(".jslink").filter(".edit").unbind('click').click(function() {
			edit_row(this);
		});
		/* set the on click for save links */
		$(".jslink").filter(".save").unbind('click').click(function() {
			save_row(this);
		});
		/* set the on click for cancel_edit links */
		$(".jslink").filter(".cancel").unbind('click').click(function() {
			cancel_edit_row(this);
		});
		/* set the on click for cancel_edit links */
		$(".jslink").filter(".add").unbind('click').click(function() {
			add_row(this);
		});
	}
	$.activate_add_popups = function() {
		$(".add-popup").each(function() {
			var $this = $(this);
			$this.children('h2').eq(0).css('clear', 'left');
			$this.prepend("<a class='jslink add'>Add new...</a>");
		});
	}

	var table_data = {};
	var table_form = {};
	var table_name_hash = {};
	function draw_row(form, obj) {
		var ret = '<tr id="row_'+obj.id+'_'+form.name+'" class="edit">\n';
		for (i in form.elements) {
			var el = form.elements[i];
			if (el.type == 'hidden') continue;
			ret += '<td id="row_'+obj.id+'-'+form.name+'-'+el.name+'">'+obj[el.name]+'</td>\n';
		}
		if (form.actions != null) {
			ret += '<td>';
			var actions = form.actions.split(' ');
			for (i in actions) {
				var a = actions[i]; 
				ret += '<a id="'+a+'_'+obj.id+'_'+form.name+'" class="jslink '+a+'">'+a+'</a>';
			}
			ret += '</td>\n';
		}
		ret += '</tr>\n';
		return ret;
	}
	function draw_form_row(form, obj) {
		var ret = '<tr id="row_'+obj.id+'_'+form.name+'">\n';
		for (i in form.elements) {
			var el = form.elements[i];
			if (el.type == 'hidden') continue;
			el.id = 'id_row_'+obj.id+'-'+form.name+'-'+el.name;
			var field = {type: el.type, name: el.name, id: el.id, value: obj[el.name]};
			if (el.options != null) {
				field.options = el.options;
			}
			ret += '<td id="row_'+obj.id+'-'+form.name+'-'+el.name+'">';
			if (typeof el.type == 'string' &&
					eval('typeof $.render_' + el.type) == 'function') {
				ret += eval('$.render_' + el.type + '(field, 1)');
			}
			ret += '</td>\n';
		}
		ret += '<td>';
		var actions = ['save', 'cancel'];
		for (i in form.elements) {
			var el = form.elements[i];
			if (el.type != 'hidden') continue;
			el.id = 'id_row_'+obj.id+'-'+form.name+'-'+el.name;
			var field = {type: el.type, name: el.name, id: el.id, value: obj[el.name]};
			ret += $.render_hidden(field, 1)+'\n';
		}
		for (i in actions) {
			var a = actions[i]; 
			ret += '<a id="'+a+'_'+obj.id+'_'+form.name+'" class="jslink '+a+'">'+a+'</a>';
		}
		ret += '</td>\n</tr>\n';
		return ret;
	}
	function draw_table(name) {
		var form = table_form[name];
		var data = table_data[name];
		var table = '<thead><tr>';
		for (i in form.elements) {
			var el = form.elements[i];
			if (el.type == 'hidden') continue;
			table += '<th>'+el.label+'</th>';
		}
		table += '<th>Actions</th></tr></thead>';

		$.each(data, function(i, item) {
			table += draw_row(form, item);
		});
		if (form.id == null) {
			form.id = form.name;
		}
		$("#"+form.id).html(table);
		js_links();
	}

	$.json_forms_and_tables = function(forms, tables) {
		if (forms == null) {
			forms = [];
		}
		if (tables == null) {
			tables = [];
		}
		var url = "/config/?json&debug";
		var query = {};
		for (i in tables)
			query[tables[i]] = "records";
		for (i in forms)
			query[forms[i]] = "form";
		
		$.postJSON(url, query, function(data) {
				for (i in forms) {
					var rform = $.render_form(data[forms[i]].form, data[forms[i]].values);
					$('#'+data[forms[i]].form.id+'-wrapper').html(rform);
					var form_options = {
						url: "/config/?json&form_submit="+forms[i]+"&debug",
						type: "POST",
						dataType: "json",
						success: function() {
							$.msgbox("The settings were saved.");
						},
					};
					$('#'+data[forms[i]].form.id+'-wrapper').submit(function() {
						$(this).ajaxSubmit(form_options);
						return false;
					});
				}
				for (i in tables) {
					table_data[tables[i]] = data[tables[i]].records;
					table_form[tables[i]] = data[tables[i]].form;
					table_name_hash[data[tables[i]].form.id] = tables[i];
					draw_table(tables[i]);
				}
				$(".tablesorter").tablesorter({textExtraction: form_or_td_extraction});
		});
	}


}); })(jQuery);

