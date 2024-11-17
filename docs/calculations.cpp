#include <iostream>
#include <cmath>
#include <string.h>

double calculateTotalForce(double totalWeight, double grade, double speed);
uint8_t calculateFECResistancePercentageValue(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance);
uint16_t calculateFECTrackResistanceGrade(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio);
double calculateGearedForce(double totalForce, double gearRatio, double defaultGearRatio);
double calculateGradeFromTotalForce(double force, double totalWeight, double speed, double gearRatio, double defaultGearRatio);
uint16_t calculateFECTargetPowerValue(double totalWeight, double grade, uint8_t cadence, double wheelDiameter, double gearRatio, double defaultGearRatio);

double const pi = 3.14159;
double const gravity = 9.81;
double const rollingResistanceCoefficient = 0.00415;
double const windResistanceCoefficient = 0.51;

int main() {
  
  //printf("Percentage Value: %d\n", calculateFECResistancePercentageValue(110, -4, 8, 2.4286, 2.4286, 200));
  //double totalForce = calculateTotalForce(110, -4, 8);
  //printf("Total force: %f\n", totalForce);
  //printf("Total force geared: %f\n", calculateGearedTotalForce(totalForce, 5, 2.4286));
  
  //printf("Track resistance grade: %d\n", calculateFECTrackResistanceGrade(110, -10, 8, 5.49, 2.4286));

  //printf("Geared total force: %f", calculateGearedTotalForce(-10,3,2));

  //printf("Percentage Value: %d\n", calculateFECResistancePercentageValue(110, 0, 8, 2.4286, 2.4286, 200));
  //printf("Percentage Value: %d\n", calculateFECResistancePercentageValue(110, 1, 8, 2.4286, 2.4286, 200));
  //printf("Percentage Value: %d\n", calculateFECResistancePercentageValue(110, 0, 8, 3.5, 2.4286, 200));
  //printf("Percentage Value: %d\n", calculateFECResistancePercentageValue(110, -1, 8, 2.4286, 2.4286, 200));


  //double force = calculateTotalForce(110, -2.3, 8);
  //printf("Force: %f\n", force);
  //printf("Grade: %f\n", calculateGradeFromTotalForce(force, 110, 8));

  
  //printf("Track resistance grade: %d\n", calculateFECTrackResistanceGrade(110, 0, 0, 2, 2.4286));
  //double gearedTotalForce = calculateGearedForce(calculateTotalForce(110, -7, 12), 2.4286, 2.4286);
  //printf("Geared total force: %f\n", gearedTotalForce);
  //printf("CGF: %f", calculateGearedForce(1, 0.75, 2.4286));

  printf("Target power: %d\n", calculateFECTargetPowerValue(110, -15, 30, 0.7, 2, 2.4286) / 4);

  return 0;
}

double calculateTotalForce(double totalWeight, double grade, double speed) {
  double gravityForce = gravity * sin(atan(grade/100)) * totalWeight;
  //printf("Gravity force: %f\n", gravityForce);
  //printf("Grade: %f\n", grade);    
  //printf("Speed: %f\n", speed);
  double rollingForce = gravity * totalWeight * rollingResistanceCoefficient;
  //printf("Rolling force: %f\n", rollingForce);

  double dragForce = 0.5 * windResistanceCoefficient * pow(speed, 2);
  //printf("Drag force: %f\n", dragForce);

  //printf("Total force: %f\n", gravityForce + rollingForce + dragForce);
  return gravityForce + rollingForce + dragForce;
}

double calculateGearedForce(double force, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  //printf("Relative gear ratio: %f\n", relativeGearRatio);
  if (force < 0) {
    return force * (1 / relativeGearRatio);
  }
  return force * relativeGearRatio;
}

uint8_t calculateFECResistancePercentageValue(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance) {
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

uint16_t calculateFECTrackResistanceGrade(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio) {
  double gearedTotalForce = calculateGearedForce(calculateTotalForce(totalWeight, grade, speed), gearRatio, defaultGearRatio);
  double calculatedGrade = calculateGradeFromTotalForce(gearedTotalForce, totalWeight, speed, gearRatio, defaultGearRatio);
  //printf("Grade requested: %f\n", grade);    
  //printf("Calculated grade %f\n", calculatedGrade);
  return 0x4E20 + (calculatedGrade * 100);
}

double calculateGradeFromTotalForce(double force, double totalWeight, double speed, double gearRatio, double defaultGearRatio) {
  double rollingForce = gravity * totalWeight * rollingResistanceCoefficient;
  double dragForce = 0.5 * windResistanceCoefficient * pow(speed, 2);
  // re-apply the subtracted gear ratio by swapping ratio and default ratio
  double gravityForce = calculateGearedForce(force - rollingForce - dragForce, defaultGearRatio, gearRatio);
  //printf("Calculated gravity force: %f\n", gravityForce);
  return tan(asin(gravityForce / totalWeight / gravity)) * 100;
}

uint16_t calculateFECTargetPowerValue(double totalWeight, double grade, uint8_t cadence, double wheelDiameter, double gearRatio, double defaultGearRatio) {
  double speed = ((cadence * gearRatio) * (wheelDiameter * pi) / 60);
  double gearedTotalForce = calculateGearedForce(calculateTotalForce(totalWeight, grade, speed), gearRatio, defaultGearRatio);
  double power = gearedTotalForce * speed;
  if (power < 0) {
    power = 0;
  }
  return (power * 4);
}

