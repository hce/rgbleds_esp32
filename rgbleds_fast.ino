
#include <Wire.h>
#include <WiFi.h>

#include <SPI.h>
#define FASTLED_ESP32_SPI_BUS HSPI
#define FASTLED_ALL_PINS_HARDWARE_SPI
#include <FastLED.h>
//#include <BluetoothSerial.h>
#include <LuaWrapper.h>
#include <DNSServer.h>

#include "buntesprog.h"
#include "page.h"
#include "queued.h"

static const char ssid[] = "WS2801 LED stripe";
static const uint8_t data_pin = 13;
static const uint8_t clock_pin = 14;
static const int strip_len = 160;
static const int localPort = 12399;
static int packetSize = 0;
//static const char psk_[] = "";

static WiFiServer server;
static WiFiClient client;
static LuaWrapper Lua;
static unsigned long lua_runtime;
static CRGB leds[strip_len];
static String lua_script;
static String header;
static String currentLine;
static String lua_script_string;
static DNSServer dnsServer;
static IPAddress apIP(192, 168, 4, 1);
//static BluetoothSerial SerialBT;

static int lua_wrapper_delay(lua_State *lua_state) {
  dnsServer.processNextRequest();
  if (!client) {
    client = server.available();
  }
  if (client && client.connected()) {
    Serial.println("WiFi client detected (Lua delay function)");
    lua_pushliteral(lua_state, "Timeout");
    return lua_error(lua_state);
  } else {
    int amount = luaL_checkinteger(lua_state, 1);
    delay(amount);
    return 0;
  }
}

static int lua_wrapper_timeout(lua_State *lua_state) {
  dnsServer.processNextRequest();
  if (!client) {
    client = server.available();
  }
  if (client && client.connected()) {
    Serial.println("WiFi client detected (Lua timeout)");
    lua_pushliteral(lua_state, "Timeout");
    return lua_error(lua_state);
  } else {
    return 0;
  }
}

static int lua_wrapper_set_leds(lua_State *lua_state) {
  for (int i = 0; i < strip_len; ++i) {
    lua_rawgeti(lua_state, 1, i + 1);
    leds[i] = lua_tointeger(lua_state, -1);
    lua_pop(lua_state, 1);
  }
  FastLED.show();
  return 0;
}

static int lua_wrapper_sethook_hack(lua_State *lua_state) {
  lua_sethook(lua_state, (const lua_Hook) &lua_wrapper_timeout, LUA_MASKCOUNT, 1000);
  return 0;
}

static int lua_wrapper_nop(lua_State *lua_state) {
  return 0;
}

void setup() {
  Serial.begin(9600);
  Serial.println("WS2801 LED stripe online");
  
  Serial.println("Starting AP");
  WiFi.softAP(ssid);
  server.begin(80);

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);  
  dnsServer.start(53, "*", apIP);
  
  FastLED.addLeds<WS2801, data_pin, clock_pin, RGB>(leds, strip_len);
  leds[0] = 255;
  leds[1] = 255 << 8;
  leds[2] = 255 << 16;
  FastLED.show();

  Lua.Lua_register("delay", (const lua_CFunction) &lua_wrapper_delay);
  Lua.Lua_register("set_leds", (const lua_CFunction) &lua_wrapper_set_leds);
  Lua.Lua_register("lua_wrapper_sethook_hack", (const lua_CFunction) &lua_wrapper_sethook_hack);
  String fooScript = "lua_wrapper_sethook_hack();\nset_leds({255, 65535, 255})\n";
  Lua.Lua_dostring(&fooScript);
  Lua.Lua_register("lua_wrapper_sethook_hack", (const lua_CFunction) &lua_wrapper_nop);

  for (int i = 0; i < buntes_programm_lua_len; ++i) {
    lua_script_string += buntes_programm_lua[i];
  }
}

void loop() {
  dnsServer.processNextRequest();
  int pinR, pinG, pinB;
  if (!client || !client.connected()) {
    client = server.available();
  }
  if (client && client.connected()) {
    header = "";
    Serial.println("Client connected!");
    long timeout = millis() + 5000;
    while(client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (header.length() > 1024) {
          Serial.println("Header LEN exceeded");
          header = "";
          return;
        }
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            if (header.indexOf("POST /script") >= 0) {
              bool inScript = false;
              lua_script_string = "";
              while(client.available()) {
                c = client.read();
                if (c == '\n') {
                  if (inScript) {
                    if (currentLine.indexOf("----") == 0) {
                      inScript = false;
                    } else {
                      if (lua_script_string.length() >= 65536) {
                        Serial.println("Script length exceeded");
                        lua_script_string = "";
                        return;
                      }
                      lua_script_string += currentLine;
                      lua_script_string += '\n';
                    }
                  } else {
                    if (currentLine.indexOf("Content-Disposition") >= 0 &&
                      currentLine.indexOf("script") >= 0) {
                        inScript = true;
                    }
                  }
                  currentLine = "";
                } else {
                  if (currentLine.length() > 512) {
                    Serial.println("Line length exceeded!");
                    currentLine = "";
                    return;
                  }
                  currentLine += c;
                }
              }
              Serial.println(currentLine);
              client.println("HTTP/1.1 200 OK Yun");
              client.println("Content-Type: text/html; charset=utf8");
              client.println("Connection: close");
              client.println();
              for (int i = 0; i < queued_html_len; ++i) {
                client.write(queued_html[i]);
              }
            } else {
              client.println("HTTP/1.1 200 OK Yun");
              client.println("Content-Type: text/html; charset=utf8");
              client.println("Connection: close");
              client.println();
              for (int i = 0; i < page_html_len; ++i) {
                const char c = page_html[i];
                if (c == '\x01') {
                  const int string_len = lua_script_string.length();
                  for (int j = 0; j < string_len; ++j) {
                    const char cc = lua_script_string[j];
                    switch(cc) {
                    case '&':
                      client.print("&amp;");
                      break;
                    case '<':
                      client.print("&lt;");
                      break;
                    case '>':
                      client.print("&gt;");
                      break;
                    case '"':
                      client.print("&quot;");
                      break;
                    default:
                      client.write(cc);
                    }
                  }
                } else {
                  client.write(c);
                }
              }
            }
            client.println();
            client.stop();
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          if (currentLine.length() > 512) {
            Serial.println("Line length exceeded!");
            currentLine = "";
            return;
          }
          currentLine += c;
        }
      }
      if (timeout < millis()) {
        break;
      }
    }
    header = "";
    Serial.println();
    Serial.println("Disconnecting client.");
    client.stop();
  } else {
    delay(3000);
  }

  if (lua_script_string.length() > 0) {
    Serial.println("BEGINNING LUA SCRIPT");
    Serial.println(lua_script_string.length());
    Lua.Lua_dostring(&lua_script_string);
  }
}
