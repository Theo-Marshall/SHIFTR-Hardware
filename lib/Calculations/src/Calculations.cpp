#include <Calculations.h>
#include <cmath>
#include <stdio.h>

double const Calculations::pi = 3.14159;
double const Calculations::gravity = 9.81;
double const Calculations::rollingResistanceCoefficient = 0.00415;
double const Calculations::windResistanceCoefficient = 0.51;

double Calculations::calculateTotalForce(double totalWeight, double grade, double speed) {
  double gravityForce = gravity * sin(atan(grade/100)) * totalWeight;
  printf("Gravity force: %f\n", gravityForce);
  printf("Grade: %f\n", grade);    
  printf("Speed: %f\n", speed);
  double rollingForce = gravity * totalWeight * rollingResistanceCoefficient;
  printf("Rolling force: %f\n", rollingForce);

  double dragForce = 0.5 * windResistanceCoefficient * pow(speed, 2);
  printf("Drag force: %f\n", dragForce);

  printf("Total force: %f\n", gravityForce + rollingForce + dragForce);
  return gravityForce + rollingForce + dragForce;
}

double Calculations::calculateGearedForce(double force, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  //printf("Relative gear ratio: %f\n", relativeGearRatio);
  if (force < 0) {
    return force * (1 / relativeGearRatio);
  }
  return force * relativeGearRatio;
}

uint8_t Calculations::calculateFECResistancePercentageValue(double totalWeight, double grade, uint8_t cadence, double wheelDiameter, double gearRatio, double defaultGearRatio, uint16_t maximumResistance) {
  double speed = calculateSpeed(cadence, wheelDiameter, gearRatio);
  double gearedTotalForce = calculateGearedForce(calculateTotalForce(totalWeight, grade, speed), gearRatio, defaultGearRatio);
  //printf("Geared total force: %f\n", gearedTotalForce);
  if (gearedTotalForce < 0) {
    return 0;
  }
  if ((maximumResistance != 0) && (gearedTotalForce <= maximumResistance)) {
    return round(gearedTotalForce / maximumResistance * 200);
  } 
  return 200;
}

uint16_t Calculations::calculateFECTrackResistanceGrade(double totalWeight, double grade, uint8_t cadence, double wheelDiameter, double gearRatio, double defaultGearRatio) {
  double speed = calculateSpeed(cadence, wheelDiameter, gearRatio);
  double gearedTotalForce = calculateGearedForce(calculateTotalForce(totalWeight, grade, speed), gearRatio, defaultGearRatio);
  double calculatedGrade = calculateGradeFromTotalForce(gearedTotalForce, totalWeight, speed, gearRatio, defaultGearRatio);
  //printf("Grade requested: %f\n", grade);    
  //printf("Calculated grade %f\n", calculatedGrade);
  return 0x4E20 + (calculatedGrade * 100);
}

double Calculations::calculateGradeFromTotalForce(double force, double totalWeight, double speed, double gearRatio, double defaultGearRatio) {
  double rollingForce = gravity * totalWeight * rollingResistanceCoefficient;
  double dragForce = 0.5 * windResistanceCoefficient * pow(speed, 2);
  // re-apply the subtracted gear ratio by swapping ratio and default ratio
  double gravityForce = calculateGearedForce(force - rollingForce - dragForce, defaultGearRatio, gearRatio);
  //printf("Calculated gravity force: %f\n", gravityForce);
  return tan(asin(gravityForce / totalWeight / gravity)) * 100;
}

uint16_t Calculations::calculateFECTargetPowerValue(double totalWeight, double grade, uint8_t cadence, double wheelDiameter, double gearRatio, double defaultGearRatio) {
  double speed = calculateSpeed(cadence, wheelDiameter, gearRatio);
  double gearedTotalForce = calculateGearedForce(calculateTotalForce(totalWeight, grade, speed), gearRatio, defaultGearRatio);
  double power = gearedTotalForce * speed;
  if (power < 0) {
    power = 0;
  }
  return (power * 4);
}

double Calculations::calculateSpeed(uint8_t cadence, double wheelDiameter, double gearRatio) {
  return ((cadence * gearRatio) * (wheelDiameter * pi) / 60);
}

