#include <Calculations.h>
#include <cmath>
#include <stdio.h>
#include <Logger.h>

double const Calculations::pi = 3.14159;
double const Calculations::gravity = 9.81;
double const Calculations::rollingResistanceCoefficient = 0.00415; // Asphalt Road
double const Calculations::windResistanceCoefficient = 0.51; // Road bike + touring position
double const Calculations::wheelDiameter = 0.7;

/**
 * Calculates the gravitational resistance based on the total weight and the grade
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @return Gravitational resistance in N
 */
double Calculations::calculateGravitationalResistance(double totalWeight, double grade) {
  double gravitationalResistance = gravity * sin(atan(grade/100)) * totalWeight;
  Logger::logDebug("Gravitational resistance: %f", gravitationalResistance);
  return gravitationalResistance;
}

/**
 * Calculates the rolling resistance based on the total weight
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @return Rolling resistance in N
 */
double Calculations::calculateRollingResistance(double totalWeight) {
  double rollingResistance = gravity * totalWeight * rollingResistanceCoefficient;
  Logger::logDebug("Rolling resistance: %f", rollingResistance);
  return rollingResistance;
}

/**
 * Calculates the wind resistance based on the bicycle and wind speed
 * 
 * @param bicycleSpeed Speed of bicycle in m/s
 * @param windSpeed Speed of wind in m/s (positive for headwind, negative for tailwind)
 * @return Wind resistance in N
 */
double Calculations::calculateWindResistance(double bicycleSpeed, double windSpeed) {
  double windResistance = 0.5 * windResistanceCoefficient * pow(bicycleSpeed + windSpeed, 2);
  Logger::logDebug("Wind resistance: %f", windResistance);
  return windResistance;
}

/**
 * Calculates the total force based on the total weight, grade and speed
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @param grade Grade in percentage
 * @param bicycleSpeed Speed in m/s
 * @return Total force in N
 */
double Calculations::calculateTotalResistance(double totalWeight, double grade, double bicycleSpeed) {
  double gravitationalResistance = calculateGravitationalResistance(totalWeight, grade);
  double rollingResistance = calculateRollingResistance(totalWeight);
  double windResistance = calculateWindResistance(bicycleSpeed, 0);
  double totalResistance = gravitationalResistance + rollingResistance + windResistance;
  Logger::logDebug("Total resistance: %f", totalResistance);
  return totalResistance;
}

double Calculations::calculateGearedForce(double force, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  //printf("Relative gear ratio: %f\n", relativeGearRatio);
  if (force < 0) {
    return force * (1 / relativeGearRatio);
  }
  return force * relativeGearRatio;
}

uint8_t Calculations::calculateFECBasicResistancePercentageValue(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio, uint16_t maximumResistance) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  double gearedSpeed = calculateGearedSpeed(measuredSpeed, relativeGearRatio);
  double gearedTotalForce = calculateGearedForce(calculateTotalResistance(totalWeight, grade, gearedSpeed), gearRatio, defaultGearRatio);
  //printf("Geared total force: %f\n", gearedTotalForce);
  if (gearedTotalForce < 0) {
    return 0;
  }
  if ((maximumResistance != 0) && (gearedTotalForce <= maximumResistance)) {
    return round(gearedTotalForce / maximumResistance * 200);
  } 
  return 200;
}

uint16_t Calculations::calculateFECTrackResistanceGrade(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  double gearedSpeed = calculateGearedSpeed(measuredSpeed, relativeGearRatio);
  double totalForce = calculateTotalResistance(totalWeight, grade, gearedSpeed);
  double gearedTotalForce = calculateGearedForce(totalForce, gearRatio, defaultGearRatio);
  double calculatedGrade = calculateGradeFromTotalForce(gearedTotalForce, totalWeight, measuredSpeed, gearRatio, defaultGearRatio);
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

uint16_t Calculations::calculateFECTargetPowerValue(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  double gearedSpeed = calculateGearedSpeed(measuredSpeed, relativeGearRatio);
  double gearedTotalForce = calculateGearedForce(calculateTotalResistance(totalWeight, grade, gearedSpeed), gearRatio, defaultGearRatio);
  double power = gearedTotalForce * gearedSpeed;
  if (power < 0) {
    power = 0;
  }
  printf("Calculated power: %f\n", power);
  return (power * 4);
}

double Calculations::calculateSpeed(uint8_t cadence, double wheelDiameter, double gearRatio) {
  return ((cadence * gearRatio) * (wheelDiameter * pi) / 60);
}

/**
 * Calculates the geared speed based on the measured speed and the relative gear ratio
 * 
 * @param measuredSpeed Measured speed in m/s
 * @param relativeGearRatio Relative gear ratio (gearRatio / defaultGearRatio)
 * @return Geared speed in m/s
 */
double Calculations::calculateGearedSpeed(double measuredSpeed, double relativeGearRatio) {
  double gearedSpeed = measuredSpeed * relativeGearRatio;
  Logger::logDebug("Measured speed: %f", measuredSpeed);
  Logger::logDebug("Realtive gear ratio: %f", relativeGearRatio);
  Logger::logDebug("Geared speed: %f", gearedSpeed);
  return gearedSpeed;
}
