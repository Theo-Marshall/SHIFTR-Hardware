#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include <Arduino.h>

class Calculations {
 public:
  static double calculateTotalForce(double totalWeight, double grade, double speed);
  static double calculateGearedTotalForce(double totalForce, double gearRatio, double defaultGearRatio);
  static uint8_t calculateFECResistancePercentageValue(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance);
  static uint16_t calculateFECTrackResistanceGrade(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance);

 private:
  static double const gravity;
  static double const rollingResistanceCoefficient;
  static double const windResistanceCoefficient;
};

#endif