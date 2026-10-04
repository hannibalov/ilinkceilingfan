#include <RCSwitch.h>

RCSwitch rf = RCSwitch();
const int RX_PIN = 27;

unsigned long lastTime = 0;
const unsigned long FRAME_TIMEOUT_MS = 100;
const unsigned long MAX_FRAMES_PER_BURST = 20;
const unsigned int MAX_RAW_PULSES = 100;

struct Frame {
  unsigned long value;
  unsigned long timestamp;
  int bitlength;
  int protocol;
  int delay_us;
  unsigned int rawLength;
  unsigned int rawData[MAX_RAW_PULSES];
};

Frame frameBuffer[MAX_FRAMES_PER_BURST];
int frameCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  rf.enableReceive(RX_PIN);

  Serial.println();
  Serial.println("=== ESP32 RF RECEIVER - BURST CAPTURE ===");
  Serial.print("RX pin: ");
  Serial.println(RX_PIN);
  Serial.println("Capturing bursts with per-frame raw timing snapshots.");
  Serial.println("NOTE: For ground-truth timings use fan_controls/esphome_capture.yaml");
  Serial.println();
}

void copyRawData(Frame* frame) {
  unsigned int rawLength = 1 + rf.getReceivedBitlength() * 2;
  if (rawLength > MAX_RAW_PULSES) {
    rawLength = MAX_RAW_PULSES;
  }

  unsigned int* raw = rf.getReceivedRawdata();
  frame->rawLength = rawLength;
  for (unsigned int i = 0; i < rawLength; i++) {
    frame->rawData[i] = raw[i];
  }
}

void printEsphomeFormat(const unsigned int* raw, unsigned int rawLength) {
  Serial.print("ESPHome code: [");
  for (unsigned int i = 0; i < rawLength; i++) {
    if (i > 0) {
      Serial.print(", ");
    }
    int signedValue = (i % 2 == 0) ? -(int)raw[i] : (int)raw[i];
    Serial.print(signedValue);
  }
  Serial.println("]");
}

void loop() {
  if (!rf.available()) {
    if (frameCount > 0 && (millis() - lastTime) > FRAME_TIMEOUT_MS) {
      printBurst();
      frameCount = 0;
    }
    return;
  }

  unsigned long value = rf.getReceivedValue();
  unsigned long now = millis();

  if (value == 0) {
    Serial.println("[Unknown/unsupported frame]");
  } else if (frameCount < MAX_FRAMES_PER_BURST) {
    Frame* frame = &frameBuffer[frameCount];
    frame->value = value;
    frame->timestamp = now;
    frame->bitlength = rf.getReceivedBitlength();
    frame->protocol = rf.getReceivedProtocol();
    frame->delay_us = rf.getReceivedDelay();
    copyRawData(frame);

    frameCount++;

    Serial.print("[Frame ");
    Serial.print(frameCount);
    Serial.print("] Value: 0x");
    Serial.print(value, HEX);
    Serial.print(" | Time delta: ");
    if (frameCount > 1) {
      Serial.print(now - frameBuffer[frameCount - 2].timestamp);
      Serial.print("ms");
    } else {
      Serial.print("(first)");
    }
    Serial.print(" | Bits: ");
    Serial.print(frame->bitlength);
    Serial.print(" | Delay: ");
    Serial.print(frame->delay_us);
    Serial.println(" us");
  }

  lastTime = now;
  rf.resetAvailable();
}

void printBurst() {
  Serial.println();
  Serial.println("======= BURST CAPTURE =======");
  Serial.print("Total frames: ");
  Serial.println(frameCount);

  unsigned long primaryValue = frameBuffer[0].value;
  int repeats = 0;
  for (int i = 1; i < frameCount; i++) {
    if (frameBuffer[i].value == primaryValue) {
      repeats++;
    }
  }

  Serial.print("Primary value: 0x");
  Serial.println(primaryValue, HEX);
  Serial.print("Identical repeats: ");
  Serial.println(repeats);

  Serial.println();
  Serial.println("Detailed Frame Breakdown:");
  Serial.println("Frame | Value          | Time(ms) | Gap(ms) | Bits | Delay(us)");
  Serial.println("------|----------------|----------|---------|------|----------");

  for (int i = 0; i < frameCount; i++) {
    Serial.print(i + 1);
    Serial.print("     | 0x");
    if (frameBuffer[i].value < 0x10000000) {
      Serial.print("0");
    }
    Serial.print(frameBuffer[i].value, HEX);
    Serial.print(" | ");
    Serial.print(frameBuffer[i].timestamp);
    Serial.print("     | ");

    if (i > 0) {
      Serial.print(frameBuffer[i].timestamp - frameBuffer[i - 1].timestamp);
    } else {
      Serial.print("0");
    }
    Serial.print("   | ");
    Serial.print(frameBuffer[i].bitlength);
    Serial.print("    | ");
    Serial.println(frameBuffer[i].delay_us);
  }

  Serial.println();
  Serial.println("Rolling Code Analysis:");
  Serial.println("Frame | Upper 16 bits | Byte 2 | Byte 1 (rolling) | Lower 16 bits");
  Serial.println("------|---------------|--------|-----------------|---------------");

  for (int i = 0; i < frameCount; i++) {
    unsigned long val = frameBuffer[i].value;

    Serial.print(i + 1);
    Serial.print("     | 0x");
    Serial.print((val >> 16) & 0xFFFF, HEX);
    Serial.print("        | 0x");
    Serial.print((val >> 8) & 0xFF, HEX);
    Serial.print("   | 0x");
    Serial.print(val & 0xFF, HEX);
    Serial.print("              | 0x");
    Serial.println(val & 0xFFFF, HEX);
  }

  Serial.println();
  Serial.println("Raw pulse data (frame 1, captured at receive time):");
  for (unsigned int i = 0; i < frameBuffer[0].rawLength; i++) {
    Serial.print(frameBuffer[0].rawData[i]);
    Serial.print(" ");
    if ((i + 1) % 10 == 0) {
      Serial.println();
    }
  }
  Serial.println();
  printEsphomeFormat(frameBuffer[0].rawData, frameBuffer[0].rawLength);

  if (frameCount > 1) {
    Serial.println();
    Serial.println("Raw pulse data (last frame, for comparison):");
    Frame* last = &frameBuffer[frameCount - 1];
    for (unsigned int i = 0; i < last->rawLength; i++) {
      Serial.print(last->rawData[i]);
      Serial.print(" ");
      if ((i + 1) % 10 == 0) {
        Serial.println();
      }
    }
    Serial.println();
    printEsphomeFormat(last->rawData, last->rawLength);
  }

  Serial.println("==============================");
  Serial.println();
}
