#include <Calculations.h>
#include <Arduino.h>

double const Calculations::gravity = 9.81;
double const Calculations::rollingResistanceCoefficient = 0.00415;
double const Calculations::windResistanceCoefficient = 0.51;

double Calculations::calculateTotalForce(double totalWeight, double grade, double speed) {
  double gravityForce = gravity * sin(atan(grade/100)) * totalWeight;
  printf("Gravity force: %f\n", gravityForce);

  double rollingForce = gravity * cos(atan(grade/100)) * totalWeight * rollingResistanceCoefficient;
  printf("Rolling force: %f\n", rollingForce);

  double dragForce = 0.5 * windResistanceCoefficient * pow(speed, 2);
  printf("Drag force: %f\n", dragForce);

  return gravityForce + rollingForce + dragForce;
}

double Calculations::calculateGearedTotalForce(double totalForce, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  printf("Relative gear ratio: %f\n", relativeGearRatio);
  if (totalForce < 0) {
    return totalForce + (abs(totalForce) * (relativeGearRatio - 1));
  }
  return totalForce * relativeGearRatio;
}

uint8_t Calculations::calculateFECResistancePercentageValue(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance) {
  double gearedTotalForce = calculateGearedTotalForce(calculateTotalForce(totalWeight, grade, speed), gearRatio, defaultGearRatio);
  if (gearedTotalForce < 0) {
    return 0;
  }
  if ((maximumResistance != 0) && (gearedTotalForce <= maximumResistance)) {
    return round(gearedTotalForce / maximumResistance * 200);
  } 
  return 200;
}

uint16_t Calculations::calculateFECTrackResistanceGrade(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance) {
  double totalForce = calculateTotalForce(totalWeight, grade, speed);

  return 0;
}


