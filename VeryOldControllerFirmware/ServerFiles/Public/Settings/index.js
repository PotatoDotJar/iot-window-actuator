$(document).ready(function () {
    fetchWifiStrength();
});

function fetchWifiStrength() {
    $('#fetchWifiBtn').attr("disabled", "disabled");
    $.ajax({
        url: '/wifiSignal',
        type: 'get',
        datatype: 'json'
    }).done(function (data) {
        $('#fetchWifiBtn').removeAttr("disabled");
        if (data.eCode == 0) {
            $('#wifi-signal').html(data.signal + " dBm");
        } else if (data.eCode == 700) {
            alertify.alert("Controller Response", "Controller busy... Try again in 10 sec.");
        } else {
            alertify.alert("Controller Response", "Unknown Error has occured.");
        }

    }).fail(function (a) {
        console.log("Error getting temps!");
        console.log(a);
        alertify.alert("API Error", "Error getting wifi signal strength!\n" + a);
    });
}

$("#upload_form").submit((e) => {
    e.preventDefault();
    var form = $("#upload_form")[0];
    var data = new FormData(form);

    var fileExt = $("#firmwareFile").val();
    var ext = fileExt.substring(fileExt.lastIndexOf('.') + 1);

    if (ext === "bin") {
        $.ajax({
            url: '/update',
            type: 'POST',
            data: data,
            contentType: false,
            processData: false,
            xhr: () => {
                var xhr = new window.XMLHttpRequest();
                xhr.upload.addEventListener('progress', (evt) => {
                    if (evt.lengthComputable) {
                        var per = evt.loaded / evt.total;
                        $("#peg").html('Progress: ' + Math.round(per * 100) + "%");
                    }
                }, false);
                return xhr;
            },
            success: (d, s) => {
                console.log("Loaded! Please wait for the controller to restart...");
                alertify.alert("Firmware Updated!", "Please wait while controller restarts.");
                window.location.href = "/";
            },
            error: (a, b, c) => {
                console.error("Error!\n" + a, b, c);
                alertify.alert("Firmware Update error!", "Error updating firmware!\n" + a);
            }
        });
    } else {
        console.error("Invalid file type!");
        alertify.alert("Invalid Filetype", "Error! File type must be a compiled .bin!");
        $("#firmwareFile").val('');
    }
});