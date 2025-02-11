#include <unity.h>
#include <stdio.h>
#include <Calculations.h>

double totalWeight = 100.0 + 10.0; // 100kg user and 10kg bike weight
double defaultGearRatio = double(34) / double(14); // chainring 34 and sprocket 14 teeth
double cadence = 85;
double gearRatio = 1.68;
double wheelDiameter = 0.7;
double pi = 3.14159;
double grade = 1;
double measuredSpeed = 7;

void test_calculations(void) {
  uint16_t resistanceGrade = Calculations::calculateFECTrackResistanceGrade(totalWeight, grade, measuredSpeed, cadence, wheelDiameter, gearRatio, defaultGearRatio);
  printf("FEC resistance grade: %d\n", resistanceGrade);
    
  TEST_ASSERT_MESSAGE(true, "this is logged");
  TEST_ASSERT(true);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_calculations);
  UNITY_END();
  return 0;
}