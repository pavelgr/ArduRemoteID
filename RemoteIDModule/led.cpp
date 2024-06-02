#include <Arduino.h>

#include "led.h"

Led led;

void Led::init(void)
{
    if (done_init) {
        return;
    }
    done_init = true;
    
    next_state = LedState::INIT;
	state = next_state;
        
#ifdef PIN_STATUS_LED
    pinMode(PIN_STATUS_LED, OUTPUT);
#endif

#ifdef WS2812_LED_PIN
	on_off = true;
    last_led_strip_ms = 0;

#ifdef WS2812_LED_POWER_PIN
    pinMode(WS2812_LED_POWER_PIN, OUTPUT);
    digitalWrite(WS2812_LED_POWER_PIN, HIGH);
#endif    
    pinMode(WS2812_LED_PIN, OUTPUT);
    
    ledStrip.begin();
    ledStrip.setBrightness(20);
#endif
}

void Led::update(void)
{
    init();

    const uint32_t now_ms = millis();

	if (state != next_state) {
		state = next_state;
#ifdef WS2812_LED_PIN
		on_off = true;
		last_led_strip_ms = 0;
#endif
	}

#ifdef PIN_STATUS_LED
    switch (state) {
    case LedState::ARM_OK: {
        digitalWrite(PIN_STATUS_LED, STATUS_LED_OK);
        last_led_trig_ms = now_ms;
        break;
    }

    default:
        if (now_ms - last_led_trig_ms > 100) {
            digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
            last_led_trig_ms = now_ms;
        }
        break;
    }
#endif

#ifdef WS2812_LED_PIN
	if (now_ms - last_led_strip_ms >= 500) {
		ledStrip.clear();

		if (on_off) {
			switch (state) {
			  case LedState::ARM_OK:
				ledStrip.fill(ledStrip.Color(0, 255, 0), 0, WS2812_LED_COUNT); 
				break;

			  default:
				ledStrip.fill(ledStrip.Color(255, 0, 0), 0, WS2812_LED_COUNT); 
				break;
			}
		}
		
		ledStrip.show();
		last_led_strip_ms = now_ms;
		on_off = !on_off;
	}
#endif
}

