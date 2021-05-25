/*
MIT License

Copyright (c) 2021 Sebastian Kemi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define LEEWAY 0.5
#define MAX_READ 10000

int inPin = 2;

volatile unsigned long last_interrupt = 0;
volatile int num_interrupts = 0;
volatile unsigned long pin_interrupts[300];

void print_hex(int num, int precision) {
      char tmp[16];
      char format[128];

      sprintf(format, "0x%%.%dX", precision);

      sprintf(tmp, format, num);
      Serial.print(tmp);
}

void handle_interrupt() {
  unsigned long now = micros();

  pin_interrupts[num_interrupts++] = now;
  last_interrupt = now;
}

bool with_leeway(float val, float compare_with) {
  float min = val - LEEWAY;
  float max = val + LEEWAY;

  return min <= compare_with && max >= compare_with;
}

float ir_duration(int from) {
  return ((pin_interrupts[from + 1] - pin_interrupts[from]) +
          (pin_interrupts[from + 2] - pin_interrupts[from + 1])) /
         1000.000;
}

byte read_byte(int start) {
  byte my_byte = 0x0;
  int bitnum = 0;
  
  for (int i = start; i < (start + (8 * 2)); i += 2) {
    float duration = ir_duration(i);
    
    if (with_leeway(duration, 2.25)) {
      my_byte |= 1 << bitnum;
    }

    bitnum++;
  }
    
  return my_byte;
}

void decode_ir() {
  float start = ir_duration(0);
  Serial.print("Startbit: " + String(start) + " ms");

  if (with_leeway(start, 11.3)) {
    Serial.print(" - Is Repeating\n\n");
    return;
  } else if (!with_leeway(start, 13.5)) {
    Serial.print(" - Not a valid startbit\n\n");
    return; 
  }

  Serial.print(" - Startbit correct\n");

  byte address = read_byte(3);
  byte ext_address = read_byte(19);

  byte command = read_byte(35);
  byte inv_command = read_byte(51);
  
  if (!((command ^ inv_command) & inv_command)) {
    Serial.println("Could not validate command");
    return;
  }

  Serial.print("Address: ");
  print_hex(address, 2);
  Serial.print(" ");
  print_hex(ext_address, 2);
  Serial.print(", ");

  Serial.print("Command: ");
  print_hex(command, 2);
  Serial.println("");
  Serial.println("");  
}

void reset() {
  num_interrupts = 0;
  memset(pin_interrupts, 0, sizeof(pin_interrupts));
}

void setup() {
  Serial.begin(9600);
  pinMode(inPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(inPin), handle_interrupt, CHANGE);
}

void loop() {
  if (num_interrupts > 0 && (micros() - last_interrupt) > MAX_READ) {
    decode_ir();
    reset();
  }

  delay(10);
}
