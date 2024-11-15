#include <Calculations.h>
#include <Arduino.h>

double const Calculations::gravity = 9.81;
double const Calculations::rollingResistanceCoefficient = 0.00415;
double const Calculations::windResistanceCoefficient = 0.51;

double Calculations::calculateTotalForce(double totalWeight, double grade, double speed) {
  double gravityForce = gravity * sin(atan(grade/100)) * totalWeight;
  //printf("Gravity force: %f\n", gravityForce);
  printf("Grade: %f\n", grade);    
  printf("Speed: %f\n", speed);
  double rollingForce = gravity * totalWeight * rollingResistanceCoefficient;
  //printf("Rolling force: %f\n", rollingForce);

  double dragForce = 0.5 * windResistanceCoefficient * pow(speed, 2);
  //printf("Drag force: %f\n", dragForce);

  //printf("Total force: %f\n", gravityForce + rollingForce + dragForce);
  return gravityForce + rollingForce + dragForce;
}

double Calculations::calculateGearedForce(double force, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  //printf("Relative gear ratio: %f\n", relativeGearRatio);
  if (force < 0) {
    return force + (abs(force) * (relativeGearRatio - 1));
  }
  return force * relativeGearRatio;
}

uint8_t Calculations::calculateFECResistancePercentageValue(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance) {
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

uint16_t Calculations::calculateFECTrackResistanceGrade(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio) {
  double gearedTotalForce = calculateGearedForce(calculateTotalForce(totalWeight, grade, speed), gearRatio, defaultGearRatio);
  double calculatedGrade = calculateGradeFromTotalForce(gearedTotalForce, totalWeight, speed);
  printf("Calculated grade %f\n", calculatedGrade);
  return 0x4E20 + (calculatedGrade * 100);
}

double Calculations::calculateGradeFromTotalForce(double force, double totalWeight, double speed) {
  double rollingForce = gravity * totalWeight * rollingResistanceCoefficient;
  double dragForce = 0.5 * windResistanceCoefficient * pow(speed, 2);
  double gravityForce = force - rollingForce - dragForce;
  //printf("Calculated gravity force: %f\n", gravityForce);
  return tan(asin(gravityForce / totalWeight / gravity)) * 100;
}

