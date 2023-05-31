#include <Arduino.h>
#include <MIDI.h>
#include <FastLED.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "Connecto Patronum";
const char* password = "Smashthepatriarchy1";

AsyncWebServer server(80);

// MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);
MIDI_CREATE_DEFAULT_INSTANCE();

int lastTime;
bool pressed = true;

const uint16_t PixelCount = 144; // make sure to set this to the number of pixels in your strip

#define DATA_PIN 2
#define NUM_LEDS 144
CRGB leds[NUM_LEDS];

#define numberOfKeys 144
float note_values[numberOfKeys];
float decay_array[numberOfKeys];
float decay_rate;
float decay_rate_slow = 0.999;
float decay_rate_fast = 0.9;
float decay_cutoff = 10;
int hue;
long unsigned int update_rate = 100;
int effect_mode = 1;
int inputSource = 2;
int LED_offset = 0;
int static_color = 0;
uint8_t gHue = 0;
bool holding_pattern = true;
long unsigned int move_rate = 10;

int max_brightness = 128;

//------------------------------------------------------------------------

// Function to update LEDs based on decay array
void updateLEDs() {
  for (int i = 0; i < numberOfKeys; i++) {
    float x = decay_array[i];
    if (x < decay_cutoff) { decay_array[i] = 0.0; }
    else { decay_array[i] = x * decay_rate; }
    // int v = round(decay_array[i] * (float)max_brightness);
    int v = round(decay_array[i]);
    // leds[i] = CRGB(v, 0, 0);
    leds[i] = CHSV(0, 255, v);
  }
  FastLED.show();
}

// Function to update LEDs based on decay array
void updateLEDs_colourCycle() {
  if (millis() - lastTime > move_rate) {
    lastTime = millis();
    hue = (hue + 1) % 256;
  }

  for (int i = 0; i < numberOfKeys; i++) {
    // float x = decay_array[i];
    if (note_values[i] < decay_cutoff) { note_values[i] = 0.0; }
    else { note_values[i] = note_values[i] * decay_array[i]; }
    // int v = round(decay_array[i] * (float)max_brightness);
    int v = round(note_values[i]);
    // leds[i] = CRGB(v, 0, 0);
    leds[i] = CHSV(hue, 255, v);
  }
  FastLED.show();
}

// Function to update LEDs based on decay array
void updateLEDs_random() {
  // if (millis() - lastTime > update_rate) {
  //   lastTime = millis();
    hue = random(0,255);
  // }

  for (int i = 0; i < numberOfKeys; i++) {
    // float x = decay_array[i];
    if (note_values[i] < decay_cutoff) { note_values[i] = 0.0; }
    else { note_values[i] = note_values[i] * decay_array[i]; }
    // int v = round(decay_array[i] * (float)max_brightness);
    int v = round(note_values[i]);
    // leds[i] = CRGB(v, 0, 0);
    leds[i] = CHSV(hue, 255, v);
  }
  FastLED.show();
}

// Function to update LEDs based on decay array
void updateLEDs_staticRainbow() {
  // if (millis() - lastTime > update_rate) {
  //   lastTime = millis();
  //   hue = (hue + 1) % 256;
  // }

  for (int i = 0; i < numberOfKeys; i++) {
    hue = map(i, 0, numberOfKeys, 0, 255);
    if (note_values[i] < decay_cutoff) { note_values[i] = 0.0; }
    else { note_values[i] = note_values[i] * decay_array[i]; }
    int v = round(note_values[i]);
    leds[i] = CHSV(hue, 255, v);
  }
  FastLED.show();
}

// Function to update LEDs based on decay array
void updateLEDs_movingRainbow() {
  if (millis() - lastTime > move_rate) {
    lastTime = millis();
    LED_offset = (LED_offset + 1);
  }

  for (int i = 0; i < numberOfKeys; i++) {
    hue = (map(i, 0, numberOfKeys, 0, 255) + LED_offset) % 255;
    if (note_values[i] < decay_cutoff) { note_values[i] = 0.0; }
    else { note_values[i] = note_values[i] * decay_array[i]; }
    int v = round(note_values[i]);
    leds[i] = CHSV(hue, 255, v);
  }
  FastLED.show();
}

// Function to update LEDs based on decay array
void updateLEDs_staticColor() {
  if (millis() - lastTime > move_rate) {
    lastTime = millis();
    // LED_offset = (LED_offset + 1);
  }

  for (int i = 0; i < numberOfKeys; i++) {
    // hue = (map(i, 0, numberOfKeys, 0, 255) + LED_offset) % 255;
    if (note_values[i] < decay_cutoff) { note_values[i] = 0.0; }
    else { note_values[i] = note_values[i] * decay_array[i]; }
    int v = round(note_values[i]);
    leds[i] = CHSV(static_color, 255, v);
  }
  FastLED.show();
}

//------------------------------------------------------------------------

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  // handy for debugging
  // MIDI.sendNoteOn(pitch, velocity, channel);

  holding_pattern = false;

  decay_array[pitch] = decay_rate_slow;
  note_values[pitch] = (float)velocity * 2.0;
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // handy for debugging
  // MIDI.sendNoteOff(pitch, velocity, channel);

  decay_array[pitch] = decay_rate_fast;

}

void fake_MIDI() {
  int randomVelocity = random(128);
  int randomPitch = random(144);
  long unsigned int randomKeyPressRate = random(250);

  if (millis() - lastTime > randomKeyPressRate) {
    lastTime = millis();
    decay_array[randomPitch] = decay_rate_slow;
    float x = map((float)randomVelocity, 0, 127, 0, max_brightness);
    note_values[randomPitch] = x;
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

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

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
    css += "form[type=button] {margin-bottom:20px; width: 80%; display: flex; align-items: center; justify-content: center; flex-direction: row; flex-flow: row wrap;}"; // form updated
    css += "form[type=slider] {margin-bottom:10px; width: 100%; display: flex; align-items: left; justify-content: center; flex-direction: column;}"; // form updated
    css += "input[type=submit] {background-color: #f4645f; border: none; border-radius: 15px; color: white; padding: 20px 40px; text-align: center; text-decoration: none; font-size: 20px; margin: 4px 2px; cursor: pointer;}";
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
    css += "</script>";
    css += "</style></head>";
        
    String body = "<body>";
    body += "<h1>LED Piano Control Panel</h1>";

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
    form1 += "<input type=\"submit\" class=\""+ String(effect_mode == 3?"active":"") +"\" name=\"mode\" value=\"Random\">";
    form1 += "<input type=\"submit\" class=\""+ String(effect_mode == 4?"active":"") +"\" name=\"mode\" value=\"Colour Cycle\">";
    form1 += "<input type=\"submit\" class=\""+ String(effect_mode == 5?"active":"") +"\" name=\"mode\" value=\"Static Colour\">";
    form1 += "</form>";
    
    String section1 = "<body>";
    section1 += "<h1>Settings</h1>";

    String formRainbow = "<form type=\"slider\" id=\"rainbowForm\">";
    formRainbow += "<label for=\"effect_speed\">Efect Speed:</label><br>";
    formRainbow += "<input type=\"range\" id=\"effect_speed\" name=\"effect_speed\" min=\"0\" max=\"100\" value=\"50\" class=\"slider\" oninput=\"updateEffectSpeed(this.value)\">";
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
      } else if(mode == "Random") {
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
      int brightness = p->value().toInt();
      // Use the brightness value to set your LED brightness. This depends on your LED setup.
      // setBrightness(brightness);
      max_brightness = brightness;
    }
    request->redirect("/"); // Redirect back to the home page after setting the brightness
});

  server.on("/effect_speed", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("effect_speed", true)) {
      AsyncWebParameter* p = request->getParam("effect_speed", true);
      int effect_speed = p->value().toInt();
      // Use the brightness value to set your LED brightness. This depends on your LED setup.
      // setBrightness(brightness);
      move_rate = 100 - effect_speed;
    }
    request->redirect("/"); // Redirect back to the home page after setting the rainbow rate
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
    // just turn all pixels on
    break;
  default:
    break;
  }

  switch(effect_mode) {
    case 1:
      updateLEDs_staticRainbow();
      break;
    case 2:
      updateLEDs_movingRainbow();
      break;
    case 3:
      updateLEDs_random();
      break;
    case 4:
      updateLEDs_colourCycle();
      break;
    case 5:
      updateLEDs_staticColor();
      break;
  }

}