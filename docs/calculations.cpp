#include <iostream>
#include <cmath>

double calculateTotalForce(double totalWeight, double grade, double speed);
uint8_t calculateFECResistancePercentageValue(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance);
uint16_t calculateFECTrackResistanceGrade(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio);
double calculateGearedForce(double totalForce, double gearRatio, double defaultGearRatio);
double calculateGradeFromTotalForce(double force, double totalWeight, double speed);

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

  //printf("Track resistance grade: %d\n", calculateFECTrackResistanceGrade(110, 0, 8, 2.4, 2.4286));

  uint16_t chainring = 34;
  uint16_t sprocket = 14;
  double ratio = chainring / (double)sprocket;
  printf("Ratio: %f", ratio);

  uint64_t value = 451;
  int64_t* signedValue = *value;
  printf("Signedvalue: %d", signedValue);

  return 0;
}

double calculateTotalForce(double totalWeight, double grade, double speed) {
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

double calculateGearedForce(double force, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  if (force < 0) {
    return force + (abs(force) * (relativeGearRatio - 1));
  }
  return force * relativeGearRatio;
}

uint8_t calculateFECResistancePercentageValue(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance) {
  double gearedTotalForce = calculateGearedForce(calculateTotalForce(totalWeight, grade, speed), gearRatio, defaultGearRatio);
  printf("Geared total force: %f\n", gearedTotalForce);

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
  double calculatedGrade = calculateGradeFromTotalForce(gearedTotalForce, totalWeight, speed);
  return 0x4E20 + (calculatedGrade * 100);
}

double calculateGradeFromTotalForce(double force, double totalWeight, double speed) {
  double rollingForce = gravity * totalWeight * rollingResistanceCoefficient;
  double dragForce = 0.5 * windResistanceCoefficient * pow(speed, 2);
  double gravityForce = force - rollingForce - dragForce;
  printf("Calculated gravity force: %f\n", gravityForce);
  return tan(asin(gravityForce / totalWeight / gravity)) * 100;
}