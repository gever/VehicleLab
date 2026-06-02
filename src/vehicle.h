#pragma once

// ---------------------------------------------------------------------------
// vehicle.h – Abstraction layer for steering and drive actuators.
//
// Replace the stub implementations in vehicle.cpp with real PWM / GPIO
// calls once the hardware is finalised.
// ---------------------------------------------------------------------------

// Initialise GPIO / PWM channels.  Call once from setup().
void vehicle_init();

// Set drive speed.
//   speed : -100 (full reverse) … 0 (stop) … +100 (full forward)
void vehicle_set_speed(int speed);

// Set steering angle.  Clamped to [steer_min, steer_max] (see below).
//   degrees : negative = left, 0 = straight, positive = right
void vehicle_set_turn(int degrees);

// Immediately stop drive and centre steering.
void vehicle_stop();

// Getters (used by /status endpoint)
int vehicle_get_speed();
int vehicle_get_turn();

// Per-vehicle steering limits (persisted in NVS, defaults −45 / +45).
// vehicle_set_steer_limits() saves to NVS and applies immediately.
void vehicle_set_steer_limits(int min_deg, int max_deg);
int  vehicle_get_steer_min();
int  vehicle_get_steer_max();
