#include <iostream>
#include <cmath>

double calculateTotalForce(double totalWeight, double grade, double speed);
uint8_t calculateFECResistancePercentageValue(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance);
uint16_t calculateFECTrackResistanceGrade(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio);
double calculateGearedTotalForce(double totalForce, double gearRatio, double defaultGearRatio);

double const gravity = 9.81;
double const rollingResistanceCoefficient = 0.00415;
double const windResistanceCoefficient = 0.51;

int main() {
  
  //printf("Percentage Value: %d\n", calculateFECResistancePercentageValue(110, -4, 8, 2.4286, 2.4286, 200));
  //double totalForce = calculateTotalForce(110, -4, 8);
  //printf("Total force: %f\n", totalForce);
  //printf("Total force geared: %f\n", calculateGearedTotalForce(totalForce, 5, 2.4286));
  
  printf("Track resistance grade: %d\n", calculateFECTrackResistanceGrade(110, -2, 8, 1, 2.4286));

  
  return 0;
}

double calculateTotalForce(double totalWeight, double grade, double speed) {
  double gravityForce = gravity * sin(atan(grade/100)) * totalWeight;
  printf("Gravity force: %f\n", gravityForce);

  double rollingForce = gravity * cos(atan(grade/100)) * totalWeight * rollingResistanceCoefficient;
  printf("Rolling force: %f\n", rollingForce);

  double dragForce = 0.5 * windResistanceCoefficient * pow(speed, 2);
  printf("Drag force: %f\n", dragForce);

  return gravityForce + rollingForce + dragForce;
}

double calculateGearedTotalForce(double totalForce, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  printf("Relative gear ratio: %f\n", relativeGearRatio);
  if (totalForce < 0) {
    return totalForce + (abs(totalForce) * (relativeGearRatio - 1));
  }
  return totalForce * relativeGearRatio;
}

uint8_t calculateFECResistancePercentageValue(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio, uint16_t maximumResistance) {
  double gearedTotalForce = calculateGearedTotalForce(calculateTotalForce(totalWeight, grade, speed), gearRatio, defaultGearRatio);
  if (gearedTotalForce < 0) {
    return 0;
  }
  if ((maximumResistance != 0) && (gearedTotalForce <= maximumResistance)) {
    return round(gearedTotalForce / maximumResistance * 200);
  } 
  return 200;
}

uint16_t calculateFECTrackResistanceGrade(double totalWeight, double grade, double speed, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  int16_t gradeIntegerValue = grade * 100;
  double totalForce = calculateTotalForce(totalWeight, grade, speed);
  printf("Total force: %f\n", totalForce);
  double totalGearedForce = calculateGearedTotalForce(totalForce, gearRatio, defaultGearRatio);
  printf("Total geared force: %f\n", totalGearedForce);


  if (gradeIntegerValue > 0) {
    return 0x4E20 + (gradeIntegerValue * relativeGearRatio);
  } else if (gradeIntegerValue <= 0) {
    return 0x4E20 + (gradeIntegerValue + (abs(gradeIntegerValue) * (relativeGearRatio - 1)));
  } else if (gradeIntegerValue == 0) {
    return 0xFFFF;
  }
  /*
  
  double totalForce = calculateTotalForce(totalWeight, grade, speed);
  printf("Total force: %f\n", totalForce);
  double totalGearedForce = calculateGearedTotalForce(totalForce, gearRatio, defaultGearRatio);
  printf("Total geared force: %f\n", totalGearedForce);
  double forceRatio = totalGearedForce / totalForce;
  printf("Force ratio: %f\n", forceRatio);
  double diffForce = totalGearedForce - totalForce;
  printf("Diff force: %f\n", diffForce);
  
  int16_t gradeIntegerValue = grade * 100;
  double relativeGearRatio = gearRatio / defaultGearRatio;
  printf("Relative gear ratio: %f\n", relativeGearRatio);
  int16_t relativeGrade = gradeIntegerValue * relativeGearRatio;
  if (gradeIntegerValue < 0) {
    relativeGrade = gradeIntegerValue + (abs(gradeIntegerValue) * (relativeGearRatio - 1));
  }
  printf("Relative grade: %d\n", relativeGrade);

  uint16_t trackResistanceGrade = 0x4E20 + relativeGrade;
*/
/*
  double totalForce = calculateTotalForce(totalWeight, grade, speed);
  printf("Total force: %f\n", totalForce);
  double totalGearedForce = calculateGearedTotalForce(totalForce, gearRatio, defaultGearRatio);
  printf("Total geared force: %f\n", totalGearedForce);

  double forceRatio = totalGearedForce / totalForce;
  printf("Force ratio: %f\n", forceRatio);

  //double totalForce = calculateTotalForce(totalWeight, grade, speed);
  //uint16_t normalFECTrackResistanceGrade = 0x4E20 + (grade * 100);
*/
  return 0;
}

