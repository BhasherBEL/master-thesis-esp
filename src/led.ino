void blink(int high, int low) {
    //digitalWrite(LED, HIGH);
    delay(high);
    //digitalWrite(LED, LOW);
    delay(low);
}

void blinkN(int high, int low, int n) {
    for(int i = 0; i < n; i++) {
        blink(high, low);
    }
}

void blinkSym(int duration) {
    blink(duration, duration);
}

void blinkSymN(int duration, int n) {
    blinkN(duration, duration, n);
}
