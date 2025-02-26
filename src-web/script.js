var gateway = `ws://${window.location.hostname}:81/control`;
var websocket;

const urlParams = new URLSearchParams(window.location.search.toLowerCase());

window.addEventListener('load', onload);

function onload(event) {
  initWebSocket();
}

function shiftUp() {
  websocket.send("shiftUp");
}

function shiftDown() {
  websocket.send("shiftDown");
}

function checkWebSocket() {
  var connectionStatus = document.getElementById('connection_status');
  if (connectionStatus != null) {
    switch (websocket.readyState) {
      case 1:
        connectionStatus.innerHTML = 'Connected';
        connectionStatus.className = 'statusgreen';
        break;
      default:
        connectionStatus.innerHTML = 'Disconnected';
        connectionStatus.className = 'statusred';
        var shiftUpButton = document.getElementById('shift_up_btn');
        if (shiftUpButton != null) {
          shiftUpButton.disabled = true;
        }
        var shiftDownButton = document.getElementById('shift_down_btn');
        if (shiftDownButton != null) {
          shiftDownButton.disabled = true;
        }
        var currentGear = document.getElementById('current_gear');
        if (currentGear != null) {
          currentGear.innerHTML = '-';
        }
        break;
    }
  }
}

function initWebSocket() {
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
  websocket.onerror = onerror;
}

function onError(event) {
  websocket.close();
}

function onOpen(event) {
  checkWebSocket();
}

function onClose(event) {
  checkWebSocket();
  setTimeout(initWebSocket, 1000);
}

function onMessage(event) {
  var json = JSON.parse(event.data);
  var keys = Object.keys(json);
  var zwiftlessVirtualShifting = false;
  var simMode = false;
  for (var i = 0; i < keys.length; i++) {
    var key = keys[i];
    if (key == 'zwiftless_virtual_shifting') {
      zwiftlessVirtualShifting = json[key];
      continue
    }
    if (key == 'trainer_mode') {
      if (json[key].includes('SIM')) {
        simMode = true;
      }
      zwiftlessVirtualShifting = json[key];
      continue
    }
    if (document.getElementById(key) == null) {
      continue
    }
    document.getElementById(key).innerHTML = json[key];
  }

  if (urlParams.has('override')) {
    zwiftlessVirtualShifting = true;
  }

  var controlContainer = document.getElementById('control_container');
  if (controlContainer != null) {
    controlContainer.style.display = simMode ? 'block' : 'none';
  }

  var shiftUpButton = document.getElementById('shift_up_btn');
  if (shiftUpButton != null) {
    shiftUpButton.disabled = !zwiftlessVirtualShifting
  }
  var shiftDownButton = document.getElementById('shift_down_btn');
  if (shiftDownButton != null) {
    shiftDownButton.disabled = !zwiftlessVirtualShifting;
  }
}