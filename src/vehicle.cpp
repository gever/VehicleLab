#include "vehicle.h"
#include <Arduino.h>
#include <Preferences.h>

// ---------------------------------------------------------------------------
// vehicle.cpp – Steering servo + drive motor control.
//
// Pin assignments:
//   STEERING_PIN  – servo signal      GPIO 12   (ledc channel 0, 50 Hz)
//   MOTOR_PWM_PIN – motor speed PWM   GPIO 26   (ledc channel 1, 1 kHz)
//   MOTOR_DIR_PIN – motor direction   GPIO 27
//
// Servo pulse timing (standard RC):
//   1 000 µs  → full left  (−45°)
//   1 500 µs  → centre     (  0°)
//   2 000 µs  → full right (+45°)
//
// With ledc at 50 Hz and 16-bit resolution the period is 20 000 µs.
// One ledc count = 20 000 µs / 65 536 ≈ 0.3052 µs.
//   1 000 µs → 3277 counts
//   1 500 µs → 4915 counts
//   2 000 µs → 6554 counts
// ---------------------------------------------------------------------------

static constexpr int STEERING_PIN  = 12;
static constexpr int MOTOR_PWM_PIN = 26;
static constexpr int MOTOR_DIR_PIN = 27;

// ledc channels
static constexpr int SERVO_CH = 0;
static constexpr int MOTOR_CH = 1;

// Servo pulse widths in ledc counts (16-bit, 50 Hz)
static constexpr uint32_t SERVO_COUNT_MIN    = 3277;  // 1 000 µs → −45°
static constexpr uint32_t SERVO_COUNT_CENTER = 4915;  // 1 500 µs →   0°
static constexpr uint32_t SERVO_COUNT_MAX    = 6554;  // 2 000 µs → +45°

// NVS namespace for vehicle preferences (separate from wifi credentials)
static constexpr const char* NVS_NS = "vprefs";

static int s_speed     = 0;
static int s_turn      = 0;
static int s_steer_min = -45;  // loaded from NVS in vehicle_init()
static int s_steer_max =  45;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void servo_write(int degrees) {
    uint32_t count = map(degrees, -45, 45, SERVO_COUNT_MIN, SERVO_COUNT_MAX);
    ledcWrite(SERVO_CH, count);
}

static void load_steer_limits() {
    Preferences p;
    p.begin(NVS_NS, /*readOnly=*/true);
    s_steer_min = p.getInt("steer_min", -45);
    s_steer_max = p.getInt("steer_max",  45);
    p.end();
    // Sanity-clamp in case stored values are out of range
    if (s_steer_min < -45) s_steer_min = -45;
    if (s_steer_max >  45) s_steer_max =  45;
    if (s_steer_min >= s_steer_max) { s_steer_min = -45; s_steer_max = 45; }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void vehicle_init() {
    // Load per-vehicle tuning from NVS before anything else
    load_steer_limits();
    Serial.printf("[vehicle] steer limits: %d° … %d°\n", s_steer_min, s_steer_max);

    // --- Steering servo (GPIO 12, channel 0, 50 Hz, 16-bit) ---
    ledcSetup(SERVO_CH, 50, 16);
    ledcAttachPin(STEERING_PIN, SERVO_CH);

    // Centre the servo immediately so the wheels start straight
    servo_write(0);
    s_turn = 0;
    Serial.println("[vehicle] Steering servo centred on GPIO 12");

    // --- Drive motor (GPIO 26 PWM + GPIO 27 direction) ---
    // TODO: uncomment when motor hardware is connected
    // ledcSetup(MOTOR_CH, 1000, 8);
    // ledcAttachPin(MOTOR_PWM_PIN, MOTOR_CH);
    // pinMode(MOTOR_DIR_PIN, OUTPUT);

    Serial.println("[vehicle] init OK");
}

void vehicle_set_speed(int speed) {
    if (speed >  100) speed =  100;
    if (speed < -100) speed = -100;
    s_speed = speed;

    // TODO: write to motor ESC / H-bridge, e.g.:
    //   digitalWrite(MOTOR_DIR_PIN, speed >= 0 ? HIGH : LOW);
    //   ledcWrite(MOTOR_CH, map(abs(speed), 0, 100, 0, 255));
    Serial.printf("[vehicle] speed → %d\n", s_speed);
}

void vehicle_set_turn(int degrees) {
    // Clamp to per-vehicle limits (not the hard ±45° servo range)
    if (degrees > s_steer_max) degrees = s_steer_max;
    if (degrees < s_steer_min) degrees = s_steer_min;
    s_turn = degrees;

    servo_write(s_turn);
    Serial.printf("[vehicle] turn → %d°\n", s_turn);
}

void vehicle_stop() {
    vehicle_set_speed(0);
    vehicle_set_turn(0);
    Serial.println("[vehicle] STOP");
}

int vehicle_get_speed()    { return s_speed;     }
int vehicle_get_turn()     { return s_turn;      }
int vehicle_get_steer_min(){ return s_steer_min; }
int vehicle_get_steer_max(){ return s_steer_max; }

void vehicle_set_steer_limits(int min_deg, int max_deg) {
    // Clamp to servo hard limits and ensure min < max
    if (min_deg < -45) min_deg = -45;
    if (max_deg >  45) max_deg =  45;
    if (min_deg >= max_deg) return;  // reject nonsensical values

    s_steer_min = min_deg;
    s_steer_max = max_deg;

    Preferences p;
    p.begin(NVS_NS, /*readOnly=*/false);
    p.putInt("steer_min", s_steer_min);
    p.putInt("steer_max", s_steer_max);
    p.end();

    Serial.printf("[vehicle] steer limits updated: %d° … %d°\n",
                  s_steer_min, s_steer_max);

    // Re-clamp current steering position if now out of range
    vehicle_set_turn(s_turn);
}
