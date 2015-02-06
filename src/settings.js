var battery;
var rules;

Pebble.addEventListener("ready",
  function(e) {
    console.log("PebbleKit JS ready!");
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
    //Load the remote config page and settings from JS
    battery = window.localStorage.getItem('battery');
    rules = window.localStorage.getItem('rules');
    Pebble.openURL("http://sephallen.github.io/DriverTimer/index.html?battery=" + battery + "&rules=" + rules);
  }
);

	
Pebble.addEventListener("webviewclosed",
  function(e) {
    
    //Get JSON dictionary
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log("Configuration window returned: " + JSON.stringify(configuration));
    // Store settings in JS
    window.localStorage.setItem('battery', configuration.seconds);
    window.localStorage.setItem('rules', configuration.rules);
 
    //Send to Pebble, persist there
    Pebble.sendAppMessage(
      {'0': configuration.seconds, '1': configuration.rules},
      function(e) {
        console.log("Sending settings data...");
      },
      function(e) {
        console.log("Sending settings failed!");
      }
    );
  }
);