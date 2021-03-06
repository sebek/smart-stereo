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

#define BYTE_POSITION(x) (3 + (16 * (x - 1)))
#define WITH_LEEWAY(val, compare) ((val - LEEWAY) <= compare && (val + LEEWAY) >= compare)
#define IR_DURATION(from) (((pin_interrupts[from + 1] - pin_interrupts[from]) + \
          (pin_interrupts[from + 2] - pin_interrupts[from + 1])) / 1000.000)

int inPin = 2;

volatile unsigned long last_interrupt = 0;
volatile int num_interrupts = 0;
volatile unsigned long pin_interrupts[300];

typedef struct packet {
  byte address;
  byte address_ext;
  byte command;
} Packet;

Packet packet = { .address = 0x0, .address_ext = 0x0, .command = 0x0 };

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

byte read_byte(int start) {
  byte my_byte = 0x0;
  int bit_count = 0;
  
  for (int i = start; i < (start + 16); i += 2) {
    float duration = IR_DURATION(i);

    // Is a one (1), zeroes are already set 
    if (WITH_LEEWAY(duration, 2.25)) {
      my_byte |= 1 << bit_count;
    }

    bit_count++;
  }
    
  return my_byte;
}

bool decode_ir() {
  float start = IR_DURATION(0);
  Serial.print("Startbit: " + String(start) + " ms");

  if (WITH_LEEWAY(start, 11.3)) {
    Serial.print(" - Is Repeating\n\n");
    return false;
  } else if (!WITH_LEEWAY(start, 13.5)) {
    Serial.print(" - Not a valid startbit\n\n");
    return false;
  }

  Serial.print(" - Startbit correct\n");

  packet.address = read_byte(BYTE_POSITION(1));
  packet.address_ext = read_byte(BYTE_POSITION(2));
  packet.command = read_byte(BYTE_POSITION(3));

  byte command_inv = read_byte(BYTE_POSITION(4));
  
  if (!((packet.command ^ command_inv) & command_inv)) {
    Serial.println("Could not validate command");
    return false;
  }

  return true;
}

void print_packet() {
    Serial.print("Address: ");
    print_hex(packet.address, 2);
    Serial.print(" ");
    print_hex(packet.address_ext, 2);
    Serial.print(", ");

    Serial.print("Command: ");
    print_hex(packet.command, 2);
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
    if (decode_ir()) {
      print_packet();
    }

    reset();
  }

  delay(10);
}
