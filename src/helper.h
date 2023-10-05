// SRPT-02 Repeater © 2023 by Red's Engineering is licensed under 
//CC BY-NC 4.0. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/4.0/


// helper-function for easy to use non-blocking timing
boolean TimePeriodIsOver (unsigned long &StartTime, unsigned long TimePeriod) {
  unsigned long currentMillis  = millis();  
  if ( currentMillis - StartTime >= TimePeriod ){
    StartTime = currentMillis; // store timestamp when the new interval has started
    return true;               // more time than TimePeriod) has elapsed since last time if-condition was true
  } 
  else return false; // return value false because LESS time than TimePeriod has passed by
}

// helper-function for easy to use non-blocking timing
boolean TimePeriodIsOver1 (unsigned long &StartTime, unsigned long TimePeriod) {
  unsigned long currentMillis  = millis();  
  if ( currentMillis - StartTime >= TimePeriod ){
    StartTime = currentMillis; // store timestamp when the new interval has started
    return true;               // more time than TimePeriod) has elapsed since last time if-condition was true
  } 
  else return false; // return value false because LESS time than TimePeriod has passed by
}

static const char REPEATER_ELEMENTS[] PROGMEM = R"(
{
  "uri": "/repeater",
  "title": "Configure",
  "menu": true,
  "element": [
        {
      "name": "style",
      "type": "ACStyle",
      "value": "
      
               .noorder {
                margin: auto
              }

              .noorder label {
               display: inline-block;
               width: 12em;
               margin: 5px 0 0 0;
               cursor: pointer;
               padding: 5px           
              }

               .noorder input[type='number'] {
               width: 60px;
               height: 20px;
               margin: 2px 0.3em 0 0;
               cursor: pointer;
               padding: 2px;
               text-align: left;
               background-color: #bbc7fc
              }
              .noorder input[type='text'] {
                width: 60px;
               height: 20px;
               margin: 5px 0.3em 0 0;
               cursor: pointer;
               padding: 2px;
               text-align: left;
               background-color: #bbc7fc
              }
             .noorder select {
              width: 60px;
               height: 25px;
              margin: 5px 0.3em 0 0;
               cursor: pointer;
               padding: 5px;
               text-align: left;
               background-color: #bbc7fc
              }

              .noorder input[type='button'] {          
               background-color: #4c6aed 
              }"
    },

    {
      "name": "product",
      "type": "ACText",
      "value": "SRPT-02 Config",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970",
      "posterior": "div"
    },
    {
      "name": "enableStationID",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Enable Station ID",
      "labelposition": "infront",
      "checked": false
    },

    {
      "name": "stationID",
      "type": "ACInput",
      "label": "Station ID",
      "placeholder": "Call Sign"
     },
     {
      "name": "wpm",
      "type": "ACSelect",
      "option": [
        "20",
        "25",
        "30",
        "35",
        "40"
      ],
      "label": "Morse Code WPM",
      "selected": 2
    },
    {
      "name": "stationIDDelay",
      "type": "ACInput",
      "label": "Station ID Delay",
      "value": "1000",
      "apply": "number",
      "pattern": "\\d*"
    },
    {
      "name": "carrierDetectDelay",
      "type": "ACInput",
      "label": "Carrier Detect Delay",
      "value": "100",
      "min": "100",
      "max": "1500", 
      "apply": "number",
      "pattern": "\\d*"
    },
    {
      "name": "minimumMessageLength",
      "type": "ACInput",
      "label": "Minimum Message Length",
      "value": "1000",
      "apply": "number",
      "pattern": "\\d*"
    },
     {
      "name": "powerSavingMode",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Power Saving Mode",
      "labelposition": "infront",
      "checked": false
    },
    {
      "name": "enableDebug",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Enable Debug Mode",
      "labelposition": "infront",
      "checked": false
    },
    
    {
      "name": "save",
      "type": "ACSubmit",
      "value": "Save Settings",
      "uri": "/save"
    }
    
  ]
}
)";

static const char PAGE_SAVE[] PROGMEM = R"(
{
  "uri": "/save",
  "title": "Elements",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "format": "Configuration has been saved. Please press Setup on repeater",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "echo",
      "type": "ACText",
      "style": "font-family:monospace;font-size:small;white-space:pre;",
      "posterior": "div"
    },
    {
      "name": "ok",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/repeater"
    }
  ]
}
)";
