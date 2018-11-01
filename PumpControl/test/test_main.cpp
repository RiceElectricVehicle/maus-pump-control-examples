#include <Arduino.h>
#include <string.h>
#include <unity.h>
#include <test_code.cpp>

void test_setup() {
  pinSetup();
  delay(100);
  // BOOST_ENBL output
  TEST_ASSERT_MESSAGE(DDRC & bit(2), "BOOST_ENBL DIR FAIL");
  TEST_ASSERT_MESSAGE(PORTC & bit(2), "BOOST_ENBL STATE FAIL");
  // encoders inputs and pullup
  TEST_ASSERT_MESSAGE(!(DDRB & bit(1)), "ENC1 DIR FAIL");
  TEST_ASSERT_MESSAGE(PORTB & bit(1), "ENC1 PULLUP FAIL");
  TEST_ASSERT_MESSAGE(!(DDRB & bit(0)), "ENC0 DIR FAIL");
  TEST_ASSERT_MESSAGE(PORTB & bit(0), "ENC0 PULLUP FAIL");
  // PUMP_PWM output
  TEST_ASSERT_MESSAGE(DDRD & bit(6), "PUMP_PWM DIR FAIL");
  // PUMP_SENSE input
  TEST_ASSERT_MESSAGE(!(DDRD & bit(3)), "PUMP_SNSE DIR FAIL");
  // nI2C_ENBL
  TEST_ASSERT_MESSAGE(DDRC & bit(1), "nI2C_ENBL DIR FAIL");
  TEST_ASSERT_FALSE_MESSAGE(PORTC & bit(1), "nI2C_ENVL STATE FAIL");
}

void test_interrupt_setup() {
  interruptSetup();
  // Enable PB PCINT
  TEST_ASSERT_BITS_HIGH_MESSAGE(ENCODER_PORT_TO_PCICR, PCICR,
                                "PB PCICR ENBL FAIL");
  // Enable PB1 PCINT
  TEST_ASSERT_BITS_HIGH_MESSAGE(ENCODER_A_TO_PCINT, PCMSK0,
                                "PB1 PCINT ENBL FAIL");
  // Enable PB2 PCINT
  TEST_ASSERT_BITS_HIGH_MESSAGE(ENCODER_B_TO_PCINT, PCMSK0,
                                "PB0 PCINT ENBL FAIL");
  // Enable INT1 on the falling edge
  TEST_ASSERT_BITS_HIGH_MESSAGE(bit(3), EICRA, "INT1 FALLING EDGE FAIL");
  // enable INT1
  TEST_ASSERT_BITS_HIGH_MESSAGE(bit(1), EIMSK, "INT1 ENBL FAIL");
  // Prescaler for Timer1
  TEST_ASSERT_BITS_HIGH_MESSAGE(bit(2), TCCR1B, "TIMER1 CLOCK PRESCALE FAIL");
  // TIMER1OVF
  TEST_ASSERT_BITS_HIGH_MESSAGE(bit(0), TIMSK1, "TIMER1_OVF ENBL FAIL");
}

void test_getStrokeRate() {
  t1 = millis();
  delay(100);
  t2 = millis() - t1;
  double poop = getStrokeRate();
  TEST_ASSERT(poop = 0.1);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_setup);
  RUN_TEST(test_interrupt_setup);
  RUN_TEST(test_getStrokeRate);

  UNITY_END();
}
void loop() {}
