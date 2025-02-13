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
  log_d("Gravitational resistance: %f", gravitationalResistance);
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
  log_d("Rolling resistance: %f", rollingResistance);
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
  log_d("Wind resistance: %f", windResistance);
  return windResistance;
}

/**
 * Calculates the FE-C basic resistance percentage value for the simulation
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @param grade Grade in percentage
 * @param measuredSpeed Measured speed from trainer in m/s
 * @param cadence Cadence in RPM
 * @param gearRatio Requested gear ratio
 * @param defaultGearRatio Default gear ratio of the bike (chainring / sprocket)
 * @param maximumResistance Maximum resistance in N from trainer
 * @return FE-C basic resistance percentage value in 0.5% (0x00 = 0%, 0xC8 = 100%)
 */
uint8_t Calculations::calculateFECBasicResistancePercentageValue(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio, uint16_t maximumResistance) {
  uint8_t basicResistancePercentageValue = 200;

  // calculate the relative gear ratio
  double relativeGearRatio = calculateRelativeGearRatio(gearRatio, defaultGearRatio);

  // calculate the geared speed and wind resistance as the gear ratio affects the speed
  double gearedSpeed = calculateGearedValue(measuredSpeed, relativeGearRatio);
  double gearedWindResistance = calculateWindResistance(gearedSpeed, 0);
  
  // calculate the resistances 
  double gravitationalResistance = calculateGravitationalResistance(totalWeight, grade);
  double rollingResistance = calculateRollingResistance(totalWeight);

  // calculate the geared gravitational and total resistance (that would apply with the specified gear ratio)
  double gearedGravitationalResistance = calculateGearedValue(gravitationalResistance, relativeGearRatio);
  double gearedTotalResistance = gearedGravitationalResistance + rollingResistance + gearedWindResistance;
  
  if (gearedTotalResistance < 0) {
    basicResistancePercentageValue = 0;
  }
  if ((maximumResistance != 0) && (gearedTotalResistance <= maximumResistance)) {
    basicResistancePercentageValue = round(gearedTotalResistance / maximumResistance * 200);
  } 
  log_d("FE-C Basic resistance value: %d (%f%% of %dN)", basicResistancePercentageValue, basicResistancePercentageValue / 2.0, maximumResistance);
  return basicResistancePercentageValue;
}

/**
 * Calculates the FE-C track resistance grade for the simulation
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @param grade Grade in percentage
 * @param measuredSpeed Measured speed from trainer in m/s
 * @param cadence Cadence in RPM
 * @param gearRatio Requested gear ratio
 * @param defaultGearRatio Default gear ratio of the bike (chainring / sprocket)
 * @return FE-C resistance grade in 0.01% (0x4E20 = 0%)
 */
uint16_t Calculations::calculateFECTrackResistanceGrade(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio) {
  uint16_t trackResistanceGrade = 0x4E20;
  
  // calculate the relative gear ratio
  double relativeGearRatio = calculateRelativeGearRatio(gearRatio, defaultGearRatio);

  // calculate the resistances
  double gravitationalResistance = calculateGravitationalResistance(totalWeight, grade);
  double rollingResistance = calculateRollingResistance(totalWeight);
  double windResistance = calculateWindResistance(measuredSpeed, 0);

  // calculate the total resistance (that would normally be requested with the specified grade)
  double totalResistance = gravitationalResistance + rollingResistance + windResistance;
  
  // calculate the geared speed and wind resistance as the gear ratio affects the speed
  double gearedSpeed = calculateGearedValue(measuredSpeed, relativeGearRatio);
  double gearedWindResistance = calculateWindResistance(gearedSpeed, 0);

  // calculate the geared gravitational and total resistance (that would apply with the specified gear ratio)
  double gearedGravitationalResistance = calculateGearedValue(gravitationalResistance, relativeGearRatio);
  double gearedTotalResistance = gearedGravitationalResistance + rollingResistance + gearedWindResistance;

  // calculate the geared grade based on the theoretical gravitational resistance
  double theoreticalGravitationalResistance = gearedTotalResistance - rollingResistance - windResistance;
  double calculatedGrade = tan(asin(theoreticalGravitationalResistance / totalWeight / gravity)) * 100;

  trackResistanceGrade += (calculatedGrade * 100);
  log_d("FE-C Track resistance grade: %d (%f%)", trackResistanceGrade, calculatedGrade);
  return trackResistanceGrade;
}

/**
 * Calculates the FE-C target power value for the simulation
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @param grade Grade in percentage
 * @param measuredSpeed Measured speed from trainer in m/s
 * @param cadence Cadence in RPM
 * @param gearRatio Requested gear ratio
 * @param defaultGearRatio Default gear ratio of the bike (chainring / sprocket)
 * @return FE-C target power value in 0.25W (0x00 = 0W)
 */
uint16_t Calculations::calculateFECTargetPowerValue(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio) {
  uint16_t targetPowerValue = 0;

  // calculate the relative gear ratio
  double relativeGearRatio = calculateRelativeGearRatio(gearRatio, defaultGearRatio);

  // calculate the resistances
  double gravitationalResistance = calculateGravitationalResistance(totalWeight, grade);
  double rollingResistance = calculateRollingResistance(totalWeight);
  
  // calculate the geared speed and wind resistance as the gear ratio affects the speed
  double gearedSpeed = calculateGearedValue(measuredSpeed, relativeGearRatio);
  double gearedWindResistance = calculateWindResistance(gearedSpeed, 0);

  // calculate the geared gravitational and total resistance (that would apply with the specified gear ratio)
  double gearedGravitationalResistance = calculateGearedValue(gravitationalResistance, relativeGearRatio);
  double gearedTotalResistance = gearedGravitationalResistance + rollingResistance + gearedWindResistance;
  
  // calculate the power based on the total resistance and speed
  double power = gearedTotalResistance * gearedSpeed;
  if (power < 0) {
    power = 0;
  }
  targetPowerValue = round(power * 4);
  log_d("FE-C Target power value: %d (%fW)", targetPowerValue, power);
  return targetPowerValue;
}

/**
 * Calculates the speed based on the cadence, wheel diameter and gear ratio
 * 
 * @param cadence Cadence in RPM
 * @param wheelDiameter Wheel diameter in m
 * @param gearRatio Gear ratio
 * @return Speed in m/s
 */
double Calculations::calculateSpeed(uint8_t cadence, double wheelDiameter, double gearRatio) {
  return ((cadence * gearRatio) * (wheelDiameter * pi) / 60);
}

/**
 * Calculates the geared value based on the relative gear ratio
 * 
 * @param originalValue Original value (unit doesn't matter)
 * @param gearRatio Gear ratio
 * @return Geared value (unit as original value)
 */
double Calculations::calculateGearedValue(double originalValue, double gearRatio) {
  double gearedValue = originalValue * gearRatio;
  if (originalValue < 0) {
    gearedValue = originalValue * (1 / gearRatio);
  }
  log_d("Original / Ratio / Geared: %f / %f / %f", originalValue, gearRatio, gearedValue);
  return gearedValue;
}

/**
 * Calculates the relative gear ratio based on the software requested and hardware installed gear ratio
 * 
 * @param gearRatio Requested gear ratio
 * @param defaultGearRatio Default gear ratio of the bike (chainring / sprocket)
 * @return Relative gear ratio
 */
double Calculations::calculateRelativeGearRatio(double gearRatio, double defaultGearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  log_d("Relative gear ratio: %f (%f / %f)", relativeGearRatio, gearRatio, defaultGearRatio);
  return relativeGearRatio;
}