#include "RepeaterCli.h"

#include <Arduino.h>

#include "SensorRepeaterApp.h"

namespace {

char command[160];

}  // namespace

void pollRepeaterCli(SensorRepeaterApp& mesh) {
  int len = strlen(command);
  while (Serial.available() && len < (int)sizeof(command) - 1) {
    char c = Serial.read();
    if (c != '\n') {
      command[len++] = c;
      command[len] = 0;
      Serial.print(c);
    }
    if (c == '\r') {
      break;
    }
  }
  if (len == (int)sizeof(command) - 1) {
    command[sizeof(command) - 1] = '\r';
  }

  if (len > 0 && command[len - 1] == '\r') {
    Serial.print('\n');
    command[len - 1] = 0;
    char reply[160];
    reply[0] = 0;
    mesh.handleCommand(0, command, reply);
    if (reply[0]) {
      Serial.print(F(" -> "));
      Serial.println(reply);
    }
    command[0] = 0;
  }
}
