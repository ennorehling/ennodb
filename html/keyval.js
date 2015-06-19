function update(result) {
    $("#result").get(0).innerHTML = result;
    $("#val").val(result);
}

function reload(word) {
$.ajax({
    url: '/ennodb/k/' + word,
    success: function(result) {
    	update(result);
    },
    error: function(xhr) {
        update("");
    }
});
}

var el = $("#key").get(0);
el.addEventListener("input", function() {
    var prefix = $('#key').val();
    if (prefix.length>0) {
        reload(prefix);
    }
    else {
    	var el = $("#result").get(0);
        el.innerHTML = "";
    }
});

var el = $("#submit").get(0);
el.addEventListener("click", function() {
    var word = $('#key').val();
    $.ajax({
    	type: "POST",
        url: '/ennodb/k/' + word,
        data: $('#val').val(),
        complete: function() { reload(word); }
    });
});
