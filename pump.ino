#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

// ---- Soft-ADC (successive approximation via PWM + LM393) ----
#define PWM_BIT   PB2   // OC0A -> RC filter -> referensi LM393 (IN1-, IN2-)
#define COMP_BIT  PB0   // LM393 OUT1 (sensor vs referensi)
#define POT_BIT   PB1   // LM393 OUT2 (trimpot vs referensi) -> ambang pompa

// ---- Indikator & pompa ----
#define LED_BIT   PD6   // 1 LED, kedip makin cepat makin kering
#define PUMP_BIT  PB7   // relay pompa
#define RELAY_ACTIVE_HIGH 1

static inline void relay_write(uint8_t on) {
    uint8_t state = RELAY_ACTIVE_HIGH ? on : !on;
    if (state) PORTB |= (1 << PUMP_BIT);
    else       PORTB &= ~(1 << PUMP_BIT);
}

void softadc_init(void) {
    DDRB  &= ~((1<<COMP_BIT)|(1<<POT_BIT));  // input, baca output LM393
    PORTB &= ~((1<<COMP_BIT)|(1<<POT_BIT));  // pull-up internal off (LM393 sudah py pull-up eksternal)
    DDRB  |= (1<<PWM_BIT);

    TCCR0A = (1<<COM0A1) | (1<<WGM01) | (1<<WGM00);
    TCCR0B = (1<<CS00);
    OCR0A = 0;
}

uint8_t softadc_read_generic(volatile uint8_t *pin_reg, uint8_t bitmask) {
    uint8_t result = 0;
    for (int8_t bit = 7; bit >= 0; bit--) {
        uint8_t test = result | (1 << bit);
        OCR0A = test;
        _delay_ms(5);
        if (*pin_reg & bitmask)
            result = test;
    }
    return result;
}

uint8_t value_to_level(uint8_t v) {
    return (v >> 5) + 1;   // 0-255 -> level 1-8
}

void show_level_blink(uint8_t level) {
    uint8_t half_period = 500 / level;
    if (half_period < 20) half_period = 20;

    for (uint8_t i = 0; i < level; i++) {
        PORTD |= (1<<LED_BIT);
        for (uint16_t t = 0; t < half_period; t++) _delay_ms(1);
        PORTD &= ~(1<<LED_BIT);
        for (uint16_t t = 0; t < half_period; t++) _delay_ms(1);
    }
}

int main(void) {
    DDRD |= (1<<LED_BIT);
    DDRB |= (1<<PUMP_BIT);
    relay_write(0);
    softadc_init();

    uint8_t pump_on = 0;
    while (1) {
        uint8_t level   = value_to_level(softadc_read_generic(&PINB, (1<<COMP_BIT)));
        uint8_t pot_raw = softadc_read_generic(&PINB, (1<<POT_BIT));

        uint8_t pump_on_level  = value_to_level(pot_raw);
        uint8_t pump_off_level = (pump_on_level > 1) ? pump_on_level - 1 : 1;

        show_level_blink(level);   // sekaligus jadi delay ~1 detik per loop

        if (!pump_on && level >= pump_on_level)  pump_on = 1;
        if ( pump_on && level <= pump_off_level) pump_on = 0;
        relay_write(pump_on);
    }
}
