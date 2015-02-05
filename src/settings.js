var battery;

Pebble.addEventListener("ready",
  function(e) {
    console.log("PebbleKit JS ready!");
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
    //Load the remote config page and settings from JS
    battery = window.localStorage.getItem('battery');
    Pebble.openURL("http://sephallen.github.io/DriverTimer/index.html?battery=" + battery);
  }
);

	
Pebble.addEventListener("webviewclosed",
  function(e) {
    
    //Get JSON dictionary
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log("Configuration window returned: " + JSON.stringify(configuration));
    // Store settings in JS
    window.localStorage.setItem('battery', configuration.seconds);
 
    //Send to Pebble, persist there
    Pebble.sendAppMessage(
      {"KEY_BATTERY": configuration.seconds},
      function(e) {
        console.log("Sending settings data...");
      },
      function(e) {
        console.log("Settings feedback failed!");
      }
    );
  }
);