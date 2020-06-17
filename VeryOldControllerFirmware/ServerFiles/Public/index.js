var windowState = [];

/*
    API ERROR CODES (eCode)
    17  -> Window moving
    7   -> Window already at that state
    77  -> Invalid Parameters
    700 -> Controller Busy
    0   -> OK
*/

function fetchTemp() {
    $('#fetchTempBtn').attr("disabled", "disabled");
    $.ajax({
        url: '/temp',
        type: 'get',
        datatype: 'json'
    }).done(function (data) {
        $('#fetchTempBtn').removeAttr("disabled");
        if (data.eCode == 0) {
            updateTemps(data);
        } else if (data.eCode == 700) {
            alertify.alert("Controller Response", "Controller busy... Try again in 10 sec.");
        } else {
            alertify.alert("Controller Response", "Unknown Error has occured.");
        }

    }).fail(function (a) {
        console.log("Error getting temps!");
        console.log(a);
        alertify.alert("API Error", "Error getting temps!\n" + a);
    });
}

function fetchLoadTemp() {
    $('#fetchTempBtn').attr("disabled", "disabled");
    $.ajax({
        url: '/loadTemp',
        type: 'get',
        datatype: 'json'
    }).done(function (data) {
        $('#fetchTempBtn').removeAttr("disabled");
        if (data.eCode == 0) {
            updateTemps(data);
        } else if (data.eCode == 700) {
            alertify.alert("Controller Response", "Controller busy... Try again in 10 sec.");
        } else {
            alertify.alert("Controller Response", "Unknown Error has occured.");
        }

    }).fail(function (a) {
        console.log("Error getting temps!");
        console.log(a);
        alertify.alert("API Error", "Error getting temps!\n" + a);
    });
}

function updateTemps(data) {
    $('#temp').html(data.tempF + " &deg;F");
}

function updateState(windowID, state) {
    windowState[windowID].state = state;

    if (state === 'closed') {
        $("#closeBtn" + windowID).attr("disabled", "disabled");
        $("#openBtn" + windowID).removeAttr("disabled");
    } else {
        $("#openBtn" + windowID).attr("disabled", "disabled");
        $("#closeBtn" + windowID).removeAttr("disabled");
    }

    $("#windowState" + windowID).html(getWindowStringState(state));
}

function requestClose(windowIndex) {
    if (windowIndex < windowState.length && windowIndex >= 0) {

        if (windowState[windowIndex].state == 'closed') {
            alertify.alert("Error Closing", "Window Already Closed!");
        } else {
            $.ajax({
                url: '/close?id=' + windowIndex,
                type: 'POST',
                datatype: 'json',
                processData: false,
                contentType: "application/json"
            }).done(function (data) {
                if (data.status) {
                    updateState(windowIndex, 'closed');
                }
                else if (data.eCode == 17) {
                    updateState(windowIndex, data.dir);
                    
                    if(data.dir === 'open') {
                        alertify.alert("Error Closing", "Window is currently opening.");
                    } else {
                        alertify.alert("Error Closing", "Window currently closing.");
                    }
                }
                else if (data.eCode == 7) {
                    updateState(windowIndex, 'closed');
                }
            }).fail(function (a, b) {
                console.log("Error requesting window close: " + a.statusText);
                alertify.alert("API Error", "Error requesting window close: " + a.statusText);
            });
        }
    }
}

function requestOpen(windowIndex) {

    if (windowIndex < windowState.length && windowIndex >= 0) {
        if (windowState[windowIndex].state === 'open') {
            alertify.alert("Error Opening", "Window Already Open!");
        } else {
            $.ajax({
                url: '/open?id=' + windowIndex,
                type: 'POST',
                datatype: 'json',
                processData: false,
                contentType: "application/json"
            }).done(function (data) {
                if (data.status) {
                    updateState(windowIndex, 'open');
                }
                else if (data.eCode == 17) {
                    updateState(windowIndex, data.dir);

                    if(data.dir === 'open') {
                        alertify.alert("Error Opening", "Window is currently opening.");
                    } else {
                        alertify.alert("Error Opening", "Window currently closing.");
                    }
                }
                else if (data.eCode == 7) {
                    updateState(windowIndex, 'open');
                    alertify.alert("Error Opening", "Window Already Open!");
                }
            }).fail(function (a, b) {
                console.log("Error requesting window open: " + a.statusText);
                alertify.alert("API Error", "Error requesting window open: " + a.statusText);
            });
        }
    }
}

function getWindowStringState(state) {
    return state == 'open' ? 'Open' : 'Closed';
}

function setCurrentWindowState() {
    $.ajax({
        async: true,
        url: '/state',
        type: 'GET',
        datatype: 'json',
        processData: false,
        contentType: "application/json"
    }).done(function (data) {
        windowState = data;

        $("#windowControls").empty();
        for (var i = 0; i < windowState.length; i++) {
            $("#windowControls").append(`
                <div class="controlSet">
                    <h3>Window ${i + 1} : <strong id="windowState${i}">${getWindowStringState(windowState[i].state)}</strong></h3>
                    <button id="openBtn${windowState[i].windowID}" onClick="requestOpen(${windowState[i].windowID})" type="button" class="btn btn-lg btn-default">Open</button>
                    <button id="closeBtn${windowState[i].windowID}" onClick="requestClose(${windowState[i].windowID})" type="button" class="btn btn-lg btn-default">Close</button>
                </div>
                <hr/>
            `);
            updateState(i, data[i].state);
        }



    }).fail(function (a, b) {
        console.error("Error requesting window state: " + a.statusText);
        alertify.alert("API Error", "Error requesting window state: " + a.statusText);
        return false;
    });
    return true;
}

$(document).ready(function () {
    if (setCurrentWindowState()) {
        fetchLoadTemp();
        //setInterval(fetchTemp, 10000);
        // Error handling here.
    }
});