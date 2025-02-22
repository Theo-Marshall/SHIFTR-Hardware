var gateway = `ws://${window.location.hostname}:81/`;
var websocket;

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

function getReadings() {
  websocket.send("getReadings");
}

function checkWebSocket() {
  if (websocket.readyState == 1) {
    websocket.send('');  
  } 
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
        break;
    }
  }
  setTimeout(checkWebSocket, 1000);
}

function initWebSocket() {
  console.log('Trying to open the WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');
  getReadings();
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  checkWebSocket();
  var json = JSON.parse(event.data);
  var keys = Object.keys(json);
  for (var i = 0; i < keys.length; i++) {
    var key = keys[i];
    if (document.getElementById(key) == null) {
      continue
    }
    document.getElementById(key).innerHTML = json[key];
  }
}