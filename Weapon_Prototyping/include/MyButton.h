class MyButton {
  private:
    // Define variables
    int pin;
    bool buttonReturn = 0;
    bool toggleState = 0;
    bool previousState1 = 0;
    bool previousState2 = 1;

    unsigned long intialPress;
    
  public:
    MyButton(int pin) {
      this->pin = pin;
      pinMode(pin, INPUT_PULLUP);
    }

    bool updateButton() {
      if (digitalRead(pin) == 0 && buttonReturn == 1) {
        toggleState = !toggleState;
        intialPress = millis();
        buttonReturn = 0;
      }

      if (digitalRead(pin) == 1)
        buttonReturn = 1;

      return !digitalRead(pin);
    }

    bool getToggle() {
      if (toggleState == 1)
        return 1;
      else
        return 0;
    }

    bool getInitialPress() {
      if (digitalRead(pin) == 0 && previousState1 == 1) {
        previousState1 = digitalRead(pin);
        return 1;
      }
      previousState1 = digitalRead(pin);
      return 0;
    }

    bool getInitialRelease() {
      if (digitalRead(pin) == 1 && previousState2 == 0) {
        previousState2 = digitalRead(pin);
        return 1;
      }
      previousState2 = digitalRead(pin);
      return 0;
    }

    int getTimePressed() {
      if (!digitalRead(pin))
        return millis() - intialPress;
      else {
        intialPress = millis();
        return -1;
      }
    }

};
