#include <unity.h>
#include <stdio.h>
#include <Calculations.h>

double totalWeight = 100.0 + 10.0; // 100kg user and 10kg bike weight
double defaultGearRatio = double(34) / double(14); // chainring 34 and sprocket 14 teeth

void test_calculations(void) {
  double totalForce = Calculations::calculateTotalForce(totalWeight, 0.0, 10.0);
  TEST_ASSERT_MESSAGE(true, "this is logged");
  TEST_ASSERT(true);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_calculations);
  UNITY_END();
  return 0;
}