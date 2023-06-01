#include <Arduino.h>
#include <MIDI.h>
#include <FastLED.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "Connecto Patronum";
const char* password = "Smashthepatriarchy1";

AsyncWebServer server(80);

MIDI_CREATE_DEFAULT_INSTANCE();

int lastTime = 0;
bool pressed = true;

// const uint16_t PixelCount = 144; // make sure to set this to the number of pixels in your strip
#define numberOfKeys 88

#define DATA_PIN 2
#define NUM_LEDS 144
CRGB leds[NUM_LEDS];
CRGB leds_temp[numberOfKeys];


#define startNote 21 // the decimal value of the 1st note


float note_values[numberOfKeys];
float decay_array[numberOfKeys];
int ledSums[numberOfKeys];
int ledLayout[numberOfKeys];

#define w 2 // white
#define b 1 // black
#define numberOfOctaves 8
#define numberOfnotesInOctave 12
int key_layout[numberOfnotesInOctave * numberOfOctaves] = {w,b,w,b,w,w,b,w,b,w,b,w,w,b,w,b,w,w,b,w,b,w,b,w,w,b,w,b,w,w,b,w,b,w,b,w,w,b,w,b,w,w,b,w,b,w,b,w,w,b,w,b,w,w,b,w,b,w,b,w,w,b,w,b,w,w,b,w,b,w,b,w,w,b,w,b,w,w,b,w,b,w,b,w,w,b,w,b,w,w,b,w,b,w,b,w};

float decay_rate;
float decay_rate_slow = 0.999;
float decay_rate_fast = 0.9;
float decay_cutoff = 30;
int hue;
long unsigned int update_rate = 100;
int effect_mode = 1;
int inputSource = 2;
int inputSource_prev = 2;
int LED_offset = 0;
int static_color = 0;
uint8_t gHue = 0;
long unsigned int total_keys_pressed = 0;
int move_rate = 128;
int counter = 0;

float max_brightness = 255.0;
float brightness_scaler = 1.0;

int precomputedHues[numberOfKeys];
//------------------------------------------------------------------------

void setupKeyLayout() {
  // now set up arrays for the notes we care about
  int sum = 1;
  for (int i = 0; i < numberOfKeys; i++) {
      int noteIndex = (startNote + i) % (numberOfnotesInOctave * numberOfOctaves);
      // add led count at this index to the running sum
      ledSums[i] += sum;
      sum += key_layout[noteIndex];
      // capture how many LEDs are at this index
      ledLayout[i] = key_layout[noteIndex];
  }
}

void setLEDsToPianoLayout() {
  for(int i = 0; i < numberOfKeys; i++) {
    int x = ledLayout[i];
    int y = ledSums[i];

    leds[y] = leds_temp[i];
    if(x == w) {
      leds[y+1] = leds_temp[i];
    }
  }
  FastLED.show();
}

void precomputeHues() {
  for (int i = 0; i < numberOfKeys; i++) {
    precomputedHues[i] = round(map(i, 0, numberOfKeys, 0, 255));
    // Serial.print(precomputedHues[i]);
  }
  // Serial.println("Complete");
}

void decayNoteValue(int i) {
  if (note_values[i] < decay_cutoff) { note_values[i] = 0.0; }
  else { note_values[i] = note_values[i] * decay_array[i]; }
}

void updateLEDs(int i, int hue) {
  int v = round(note_values[i]);
  leds_temp[i] = CHSV(hue, 255, v);
}

void clearStrip() {
  for(int i = 0; i < numberOfKeys; ++i) {
    hue = 0;
    decay_array[i] = 0.0;
    updateLEDs(i, hue);
  }
  inputSource_prev = inputSource;
}

// void updateLEDs() {
//   for (int i = 0; i < numberOfKeys; i++) {
//     decayNoteValue(i);
//     updateLEDs(i, 0);
//   }
//   FastLED.show();
// }

void updateLEDs_colourCycle() {
  if (counter > move_rate) {
    counter = 0;
  } else {
    ++counter;
  }

  if (hue > 255) {
    hue = 0;
  }
  else if (counter == 0) {
    ++hue;
  }

  for (int i = 0; i < numberOfKeys; i++) {
    decayNoteValue(i);
    updateLEDs(i, hue);
  }
  // FastLED.show();
  setLEDsToPianoLayout();
}

void updateLEDs_incremental() {
  for (int i = 0; i < numberOfKeys; i++) {
    if (decay_array[i] == decay_rate_slow) { // check to see what notes are pressed
      hue = total_keys_pressed % 255;
    }
    decayNoteValue(i);
    updateLEDs(i, hue);
  }
  // FastLED.show();
  setLEDsToPianoLayout();
}

void updateLEDs_staticRainbow() {
  for (int i = 0; i < numberOfKeys; i++) {
    hue = precomputedHues[i];
    decayNoteValue(i);
    updateLEDs(i, hue);
  }
  // FastLED.show();
  setLEDsToPianoLayout();
}

void updateLEDs_movingRainbow() {
  if (counter > move_rate) {
    counter = 0;
  } else {
    ++counter;
  }

  if (LED_offset > numberOfKeys) {
    LED_offset = 0;
  }
  else if (counter == 0) {
    ++LED_offset;
  }

  for (int i = 0; i < numberOfKeys; i++) {
    int x = (i + LED_offset) % numberOfKeys;
    hue = precomputedHues[x];
    decayNoteValue(i);
    updateLEDs(i, hue);
  }
  // FastLED.show();
  setLEDsToPianoLayout();
}

void updateLEDs_staticColor() {
  for (int i = 0; i < numberOfKeys; i++) {
    decayNoteValue(i);
    updateLEDs(i, static_color);
  }
  // FastLED.show();
  setLEDsToPianoLayout();
}

//------------------------------------------------------------------------
int setBrightness(float velocity) {
  return map(velocity, 0, 127, 0, max_brightness);
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  // handy for debugging
  // MIDI.sendNoteOn(pitch, velocity, channel);

  total_keys_pressed++;

  int note = pitch - startNote;

  decay_array[note] = decay_rate_slow;
  note_values[note] = setBrightness((float)velocity); //(float)velocity * brightness_scaler;
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // handy for debugging
  // MIDI.sendNoteOff(pitch, velocity, channel);

  int note = pitch - startNote;

  decay_array[note] = decay_rate_fast;

}

void fake_MIDI() {
  int randomVelocity = random(decay_cutoff, 128);
  int randomPitch = random(numberOfKeys);
  long unsigned int randomKeyPressRate = random(250);

  if (millis() - lastTime > randomKeyPressRate) {
    lastTime = millis();
    decay_array[randomPitch] = decay_rate_slow;
    // float x = map((float)randomVelocity, 0, 127, 0, max_brightness);
    // note_values[randomPitch] = x;
    note_values[randomPitch] = setBrightness((float)randomVelocity);  //(float)randomVelocity * brightness_scaler;

    total_keys_pressed++;
  }

}

void solid() {
  for(int i = 0; i < numberOfKeys; ++i) {
    note_values[i] = setBrightness(127.0);
    decay_array[i] = 1.0;
  }
}

//------------------------------------------------------------------------

void test_MIDIoutput() {
  if (millis() - lastTime > 500) {
    lastTime = millis();

    // send something
    if (pressed == true) {
      MIDI.sendNoteOn(42, 100, 1);
      pressed = false; }
    else {
      MIDI.sendNoteOff(42, 100, 1);
      pressed = true; }
  }
}

//------------------------------------------------------------------------

void setup() {

  precomputeHues();
  setupKeyLayout();

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  Serial.begin(115200);

  // Connect the handleNoteOn function to the library,
  // so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

  // Do the same for NoteOffs
  MIDI.setHandleNoteOff(handleNoteOff);

  // Initiate MIDI communications, listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    // Serial.println("Connecting to WiFi...");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html_head = "<html><head>";
    String css = "<style>";
    css += "body {font-family: Arial, Helvetica, sans-serif; padding:30px; background-color: #1e1e1e; color: #ddd; display: flex; align-items: center; justify-content: center; flex-direction: column;}";
    css += "h1, h2 {color: #f4645f; font-size: xx-large;}"; // h2 added
    css += "form[type=button] {margin: 20px; height: 300px; width: 100%; display: flex; align-items: center; justify-content: center; flex-wrap: wrap;}";
    css += "input[type=submit] {background-color: #f4645f; flex: 1 1 0; border: none; border-radius: 15px; color: white; padding: 60px; text-align: center; text-decoration: none; font-size: 20px; margin: 4px 2px; cursor: pointer;}";
    css += "form[type=slider] {margin-bottom:10px; width: 100%; display: flex; align-items: left; justify-content: center; flex-direction: column;}";
    css += ".active {background-color: #2d2d2d !important;}";
    css += "label { width: 100%;}";
    css += "input[type=range] {width: 100%; height: 50px;}";
    css += ".slider {width: 100%; height: 100%; appearance: none; outline: none; opacity: 0.7; transition: opacity .2s; background-color: #1e1e1e;}";
    css += ".slider:hover { opacity: 1; }";
    css += ".slider::-webkit-slider-thumb { appearance: none; width: 20px; height: 20px; background: #f4645f; cursor: pointer; border-radius: 100%;}";
    css += ".slider::-moz-range-thumb { width: 20px; height: 20px; background: #f4645f; cursor: pointer; border-radius: 100%;}";
    css += ".slider::-webkit-slider-runnable-track { width: 100%; height: 10px; cursor: pointer; background: #ddd; border-radius: 5px;}";
    css += ".slider::-moz-range-track { width: 100%; height: 10px; cursor: pointer; background: #ddd; border-radius: 5px;}";
    css += "</style>";
    css += "<script>";
    css += "function updateBrightness(value) {";
    css += "  var xhr = new XMLHttpRequest();";
    css += "  xhr.open('POST', '/brightness', true);";
    css += "  xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
    css += "  xhr.send('brightness=' + encodeURIComponent(value));";
    css += "}";
    css += "function updateEffectSpeed(value) {";
    css += "  var xhr = new XMLHttpRequest();";
    css += "  xhr.open('POST', '/effect_speed', true);";
    css += "  xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
    css += "  xhr.send('effect_speed=' + encodeURIComponent(value));";
    css += "}";
    css += "function updateStaticColor(value) {";
    css += "  var xhr = new XMLHttpRequest();";
    css += "  xhr.open('POST', '/color', true);";
    css += "  xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
    css += "  xhr.send('color=' + encodeURIComponent(value));";
    css += "}";
    css += "setInterval(function(){";
    css += "  fetch('/live_variable').then(response => response.text()).then(data => {";
    css += "    document.getElementById('liveVar').innerText = 'Keys Pressed: ' + data;";
    css += "  });";
    css += "}, 100);"; // Fetch every 1 second
    css += "</script>";
    css += "</style></head>";
        
    String body = "<body>";
    body += "<h1>LED Piano Control Panel</h1>";
    body += "<p id='liveVar'>Keys Pressed: </p>";

    String form0 = "<form type=\"button\" action=\"/ipSource\" method=\"POST\">";
    form0 += "<input type=\"submit\" class=\""+ String(inputSource == 1?"active":"") +"\" name=\"ipSource\" value=\"MIDI\">";
    form0 += "<input type=\"submit\" class=\""+ String(inputSource == 2?"active":"") +"\" name=\"ipSource\" value=\"Random\">";
    form0 += "<input type=\"submit\" class=\""+ String(inputSource == 3?"active":"") +"\" name=\"ipSource\" value=\"Solid\">";
    form0 += "</form>";

    String section0 = "<body>";
    section0 += "<h1>Effects</h1>";

    String form1 = "<form type=\"button\" action=\"/mode\" method=\"POST\">";
    form1 += "<input type=\"submit\" class=\""+ String(effect_mode == 1?"active":"") +"\" name=\"mode\" value=\"Static Rainbow\">";
    form1 += "<input type=\"submit\" class=\""+ String(effect_mode == 2?"active":"") +"\" name=\"mode\" value=\"Moving Rainbow\">";
    form1 += "<input type=\"submit\" class=\""+ String(effect_mode == 3?"active":"") +"\" name=\"mode\" value=\"Incremental\">";
    form1 += "<input type=\"submit\" class=\""+ String(effect_mode == 4?"active":"") +"\" name=\"mode\" value=\"Colour Cycle\">";
    form1 += "<input type=\"submit\" class=\""+ String(effect_mode == 5?"active":"") +"\" name=\"mode\" value=\"Static Colour\">";
    form1 += "</form>";
    
    String section1 = "<body>";
    section1 += "<h1>Settings</h1>";

    String formRainbow = "<form type=\"slider\" id=\"rainbowForm\">";
    formRainbow += "<label for=\"effect_speed\">Efect Speed:</label><br>";
    formRainbow += "<input type=\"range\" id=\"effect_speed\" name=\"effect_speed\" min=\"0\" max=\"255\" value=\"128\" class=\"slider\" oninput=\"updateEffectSpeed(this.value)\">";
    formRainbow += "</form>";

    String formDecayRate = "<form type=\"slider\" id=\"decayForm\" action=\"/decay_rate\" method=\"POST\">";
    formDecayRate += "<label for=\"rate\">Decay Rate:</label>";
    formDecayRate += "<input type=\"text\" id=\"rate\" name=\"rate\">";
    formDecayRate += "<input type=\"submit\" value=\"Submit\">";
    formDecayRate += "</form>";

    String formColor = "<form type=\"slider\" id=\"colorForm\">";
    formColor += "<label for=\"color\">Colour:</label><br>";
    formColor += "<input type=\"range\" id=\"color\" name=\"color\" min=\"0\" max=\"255\" value=\"0\" class=\"slider\" oninput=\"updateStaticColor(this.value)\">";
    formColor += "</form>";

    String formBrightness = "<form type=\"slider\">";
    formBrightness += "<label for=\"brightness\">Brightness:</label><br>";
    formBrightness += "<input type=\"range\" id=\"brightness\" name=\"brightness\" min=\"0\" max=\"255\" value=\"128\" class=\"slider\" oninput=\"updateBrightness(this.value)\">";
    formBrightness += "</form>";

    String html_end = "</body></html>";

    String html = html_head + css + body + form0 + section0 + form1 + section1 + formBrightness + formColor + formRainbow + html_end;
    request->send(200, "text/html", html);
});

server.on("/ipSource", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ipSource", true)) {
      String ipSource = request->getParam("ipSource", true)->value();
      if(ipSource == "MIDI") {
        inputSource = 1;
      } else if(ipSource == "Random") {
        inputSource = 2;
      } else if(ipSource == "Solid") {
        inputSource = 3;
      }
      
    }
    request->redirect("/");
});

server.on("/mode", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("mode", true)) {
      String mode = request->getParam("mode", true)->value();
      if(mode == "Static Rainbow") {
        // Set Mode 1
        effect_mode = 1;
      } else if(mode == "Moving Rainbow") {
        // Set Mode 2
        effect_mode = 2;
      } else if(mode == "Incremental") {
        // Set Mode 3
        effect_mode = 3;
      } else if(mode == "Colour Cycle") {
        // Set Mode 4
        effect_mode = 4;
      } else if(mode == "Static Colour") {
        // Set Mode 5
        effect_mode = 5;
      }
    }
    request->redirect("/");
});

  server.on("/decay_rate", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("decay_rate_slow", true)) {
      AsyncWebParameter* p = request->getParam("decay_rate_slow", true);
      decay_rate_slow = p->value().toFloat();
    }
    else if (request->hasParam("decay_rate_fast", true)) {
      AsyncWebParameter* p = request->getParam("decay_rate_fast", true);
      decay_rate_fast = p->value().toFloat();
    }
    request->redirect("/"); // send(200, "text/plain", "Updated decay rate");
  });

  server.on("/color", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("color", true)) {
      AsyncWebParameter* p = request->getParam("color", true);
      int color = p->value().toInt();
      // Use the color value to set your LED colors. This depends on how your setColor function is implemented.
      static_color = color;
    }
    request->redirect("/"); // Redirect back to the home page after setting the color
});

  server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("brightness", true)) {
      AsyncWebParameter* p = request->getParam("brightness", true);
      max_brightness = p->value().toFloat();
      // Use the brightness value to set your LED brightness. This depends on your LED setup.
      // setBrightness(brightness);
    }
    request->redirect("/"); // Redirect back to the home page after setting the brightness
});

  server.on("/effect_speed", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("effect_speed", true)) {
      AsyncWebParameter* p = request->getParam("effect_speed", true);
      int effect_speed = p->value().toInt();
      // Use the brightness value to set your LED brightness. This depends on your LED setup.
      // setBrightness(brightness);
      move_rate = 255 - effect_speed;
    }
    request->redirect("/"); // Redirect back to the home page after setting the rainbow rate
});

  server.on("/live_variable", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(total_keys_pressed));
});

  // Start the server
  server.begin();

}

void loop() {

  switch (inputSource)
  {
  case 1:
    MIDI.read();
    break;
  case 2:
    fake_MIDI();
    break;
  case 3:
    solid();
    break;
  default:
    fake_MIDI();
    break;
  }

  if (inputSource != inputSource_prev) {
    clearStrip();
  }

  switch(effect_mode) {
    case 1:
      updateLEDs_staticRainbow();
      break;
    case 2:
      updateLEDs_movingRainbow();
      break;
    case 3:
      updateLEDs_incremental();
      break;
    case 4:
      updateLEDs_colourCycle();
      break;
    case 5:
      updateLEDs_staticColor();
      break;
  }

}