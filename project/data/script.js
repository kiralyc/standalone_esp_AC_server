// Complete project details: https://github.com/kiralyc/standalone_esp_AC_server

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getValues(){
    websocket.send("getValues");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function updateOnchange(element) {
    var sliderValue = document.getElementById('slider1').value;
    document.getElementById('sliderValue1').innerHTML = sliderValue;
	if(element.id.startsWith('slider')) {
		console.log(sliderValue);
		websocket.send("1s"+sliderValue.toString());
	}
	if(element.id.startsWith('fan')) {
		var fanRadios = document.getElementsByName("fan");
		for (var j = 0; j < fanRadios.length; j++) {
			if (fanRadios[j].checked == true) {
				var fanValue =  fanRadios[j].value;
				break;
			}
		}
		console.log(fanValue);
		websocket.send("2s"+fanValue.toString());
	}
	if(element.id.startsWith('ac')) {
		var acRadios = document.getElementsByName("ac");
		for (var j = 0; j < acRadios.length; j++) {
			if (acRadios[j].checked == true) {
				var acValue =  acRadios[j].value;
				break;
			}
		}
		console.log(acValue);
		websocket.send("3s"+acValue.toString());
	}
	if(element.id.startsWith('power')) {
		var powerRadios = document.getElementsByName("power");
		for (var j = 0; j < powerRadios.length; j++) {
			if (powerRadios[j].checked == true) {
				var powerValue =  powerRadios[j].value;
				break;
			}
		}
		console.log(powerValue);
		websocket.send("4s"+powerValue.toString());
	}
}

function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
		if((key.valueOf() == 'sliderValue1') || (key.valueOf() == 'temperature') || (key.valueOf() == 'humidity') || (key.valueOf() == 'batterylevel') || (key.valueOf() == 'batteryvoltage')) {
			document.getElementById(key).innerHTML = myObj[key];
		}
		if(key.startsWith('slider')){
			document.getElementById(key.replace('Value','')).value = myObj[key];	
		}
		if((key.valueOf() == 'fan') || (key.valueOf() == 'ac') || (key.valueOf() == 'power')) {
			setCheckedValueOfRadioButtonGroup(key, myObj[key]);
		}
    }
}

function setCheckedValueOfRadioButtonGroup(vRadioObj, vValue) {
    var radios = document.getElementsByName(vRadioObj);
    for (var j = 0; j < radios.length; j++) {
        if (radios[j].value == vValue) {
            radios[j].checked = true;
            break;
        }
    }
}
