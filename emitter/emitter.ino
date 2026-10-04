#include <RCSwitch.h>

const int TX_PIN = 26;

// Raw pulse sequences from RCSwitch captures (unsigned durations).
// timings[0] is the sync carrier-OFF gap — transmission starts LOW (H1 fix).
unsigned int remoteA_FAN_OFF[] = {
  10897, 327, 1052, 1050, 350, 352, 1054, 1076, 312, 376, 1045, 1060, 329, 1072, 334, 362,
  1052, 1033, 372, 1026, 378, 1021, 385, 311, 1088, 318, 1084, 1016, 389, 311, 1091, 1014,
  390, 308, 1094, 309, 1091, 311, 1090, 312, 1088, 1017, 390, 308, 1094, 1010, 393, 1007,
  398, 296, 1104, 1001, 402, 300, 1103, 1001, 403, 997, 406, 994, 412, 287, 1115, 287, 1111
};
int remoteA_FAN_OFF_len = 65;

unsigned int remoteB_FAN_OFF[] = {
  10891, 331, 1044, 1064, 335, 357, 1051, 1086, 300, 392, 1028, 1067, 319, 1081, 327, 370,
  1046, 356, 1067, 333, 1068, 1034, 370, 1031, 374, 324, 1078, 1026, 377, 320, 1080, 322,
  1083, 320, 1080, 1024, 380, 1018, 386, 316, 1085, 1016, 388, 310, 1094, 1011, 391, 1008,
  397, 301, 1099, 302, 1099, 1007, 399, 299, 1101, 301, 1100, 305, 1095, 307, 1096, 1009, 396
};
int remoteB_FAN_OFF_len = 65;

const int BURST_COUNT = 7;
const int BURST_DELAY_MS = 30;
const int INTER_COMMAND_DELAY_MS = 2000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);

  Serial.println("=== RF TX BURST TEST (RAW PULSES, POLARITY CORRECTED) ===");
  Serial.print("TX Pin: ");
  Serial.println(TX_PIN);
  Serial.print("Burst count: ");
  Serial.println(BURST_COUNT);
  Serial.print("Burst delay: ");
  Serial.print(BURST_DELAY_MS);
  Serial.println("ms");
  Serial.println();
}

void sendRawPulses(unsigned int* pulses, int numPulses) {
  noInterrupts();

  // H1: timings[0] is sync gap (carrier OFF), not carrier ON
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(pulses[0]);

  for (int i = 1; i < numPulses; i++) {
    digitalWrite(TX_PIN, (i % 2 == 1) ? HIGH : LOW);
    delayMicroseconds(pulses[i]);
  }

  digitalWrite(TX_PIN, LOW);
  interrupts();
}

void sendBurst(unsigned int* pulses, int numPulses, const char* label) {
  Serial.println("------------------------------");
  Serial.print("Sending: ");
  Serial.println(label);
  Serial.print("Burst: ");
  Serial.print(BURST_COUNT);
  Serial.println(" repetitions (raw pulses)");
  Serial.println("Transmitting...");

  for (int i = 0; i < BURST_COUNT; i++) {
    sendRawPulses(pulses, numPulses);
    delay(BURST_DELAY_MS);

    if (i == 0) {
      Serial.print("  Pulse 1");
    } else if (i == BURST_COUNT - 1) {
      Serial.print(" -> Pulse ");
      Serial.print(i + 1);
      Serial.println(" done");
    } else {
      Serial.print(".");
    }
  }

  Serial.println("Burst complete");
}

void loop() {
  sendBurst(remoteA_FAN_OFF, remoteA_FAN_OFF_len, "Remote A - Fan OFF");
  delay(INTER_COMMAND_DELAY_MS);

  sendBurst(remoteB_FAN_OFF, remoteB_FAN_OFF_len, "Remote B - Fan OFF");
  delay(INTER_COMMAND_DELAY_MS);

  Serial.println();
  Serial.println("Waiting 5 seconds before next cycle...");
  delay(5000);
}
