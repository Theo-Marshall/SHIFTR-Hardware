#include <unity.h>
#include <stdio.h>
#include <Calculations.h>
#include <Logger.h>

double zwiftGears[] = {0.75, 0.87, 0.99, 1.11, 1.23, 1.38, 1.53, 1.68, 1.86, 2.04, 2.22, 2.40, 2.61, 2.82, 3.03, 3.24, 3.49, 3.74, 3.99, 4.24, 4.54, 4.84, 5.14, 5.49};

double totalWeight = 100.0 + 10.0; // 100kg user and 10kg bike weight
double defaultGearRatio = double(34) / double(14); // chainring 34 and sprocket 14 teeth
double cadence = 85;
double gearRatio = 2.4;
double pi = 3.14159;
double grade = 1;
double measuredSpeed = 7;

void test_calculations(void) {
  Logger::defaultLogLevel = LOG_LEVEL_INFO;

  uint16_t resistanceGrade;
  double gearedGrade;
  int gearNumber = 1;
  for(double zwiftGear : zwiftGears) {
    resistanceGrade = Calculations::calculateFECTrackResistanceGrade(totalWeight, grade, measuredSpeed, cadence, zwiftGear, defaultGearRatio);
    gearedGrade = (resistanceGrade - 0x4E20) / 100.0;
    log_i("Zwift gear %d (%f) - Requested grade: %f -> Geared grade: %f", gearNumber, zwiftGear, grade, gearedGrade);
    gearNumber++;
  }
  
  TEST_ASSERT(true);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_calculations);
  UNITY_END();
  return 0;
}