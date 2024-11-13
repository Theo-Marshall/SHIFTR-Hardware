#include <iostream>
#include <cmath>

#define DEFAULT_GEAR_RATIO 2.4

int main() {
  double gravity = 9.81;
  double weight = 100 + 10;
  
  double grade = -1;

  double gravityForceOld = (weight * grade) * gravity / 100;
  printf("Gravity force old: %f\n", gravityForceOld);

  double gravityForce = gravity * sin(atan(grade/100)) * weight;
  printf("Gravity force: %f\n", gravityForce);

  double rollingForceOld = weight * gravity * 0.004;
  printf("Rolling force old: %f\n", rollingForceOld);

  double rollingForce = gravity * cos(atan(grade/100)) * weight * 0.004;
  printf("Rolling force: %f\n", rollingForce);

  double totalForceOld = gravityForceOld + rollingForceOld;
  printf("Total force old: %f\n", totalForceOld);

  double totalForce = gravityForce + rollingForce;
  printf("Total force: %f\n", totalForce);

  // 0.75 0.87 0.99 1.11 1.23 1.38 1.53 1.68 1.86 2.04 2.22 2.40 2.61 2.82 3.03 3.24 3.49 3.74 3.99 4.24 4.54 4.84 5.14 5.49
  double relativeGearRatio = 1.68 / DEFAULT_GEAR_RATIO;
  printf("Relative gear ratio: %f\n", relativeGearRatio);

  // add relative gear ratio to rolling force
  rollingForce = rollingForce * relativeGearRatio; 

  // add relative gear ratio to gravity force
  if (gravityForce >= 0) {
    gravityForce = gravityForce * relativeGearRatio; 
  } else {
    gravityForce = gravityForce / relativeGearRatio; 
  }
  totalForce = gravityForce + rollingForce;

  printf("New gravity force: %f\n", gravityForce);
  printf("New rolling force: %f\n", rollingForce);
  printf("New total force: %f\n", totalForce);


}