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
			html = request.responseText.replace(re, '\1');
			alert("ajax error!\nhttp-status: "+request.status);
			if (html.length > 0)
				$("html").html(html);
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
		var v = "<pre>"+_dump(obj)+"</pre>";
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

}); })(jQuery);

