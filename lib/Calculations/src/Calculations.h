#include <cstdint>
#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#if defined(ESP32)
  #include <Arduino.h>
#endif
class Calculations {
 public:
  static double calculateGravitationalResistance(double totalWeight, double grade);
  static double calculateRollingResistance(double totalWeight);
  static double calculateWindResistance(double bicycleSpeed, double windSpeed);

  static double calculateTotalResistance(double totalWeight, double grade, double bicycleSpeed);
  static double calculateGearedForce(double force, double gearRatio, double defaultGearRatio);
  static uint8_t calculateFECBasicResistancePercentageValue(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio, uint16_t maximumResistance);
  static uint16_t calculateFECTrackResistanceGrade(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio);
  static uint16_t calculateFECTargetPowerValue(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio);
  static double calculateGradeFromTotalForce(double force, double totalWeight, double speed, double gearRatio, double defaultGearRatio);
  static double calculateSpeed(uint8_t cadence, double wheelDiameter, double gearRatio);
  static double calculateGearedValue(double originalValue, double gearRatio);
  
  static double const pi;
  static double const gravity;
  static double const rollingResistanceCoefficient;
  static double const windResistanceCoefficient;
  static double const wheelDiameter;

 private:
};

#endif