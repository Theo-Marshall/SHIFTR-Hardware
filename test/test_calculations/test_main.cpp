#include <unity.h>
#include <stdio.h>
#include <Calculations.h>

void test_calculations(void) {
  double totalForce = Calculations::calculateTotalForce(100.0, 0.0, 10.0);
  TEST_ASSERT_MESSAGE(true, "this is logged");
  TEST_ASSERT(true);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_calculations);
  UNITY_END();
  return 0;
}