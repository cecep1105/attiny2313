#define F_CPU 16000000UL   // kristal eksternal 16MHz
#include <avr/io.h>
#include <util/delay.h>

// ---- Sensor / soft-ADC (PB0=AIN0 sensor, PB1=AIN1 ref, PB2=OC0A pwm) ----
#define REF_BIT     PB1
#define COMP_BIT PB0   // baca output LM393 (HIGH = sensor > referensi)
#define PWM_BIT  PB2   // OC0A -> RC filter -> LM393 IN1-




// ---- LED bar (level 1..8) ----
#define LED1_BIT PD0
#define LED2_BIT PD1
#define LED3_BIT PD2
#define LED4_BIT PB3
#define LED5_BIT PB4
#define LED6_BIT PB5
#define LED7_BIT PB6
#define LED8_BIT PB7

// ---- Pompa / relay ----
#define PUMP_BIT PD6
#define RELAY_ACTIVE_HIGH 1   // ganti 0 kalau modul relay-nya active-LOW

#define PUMP_ON_LEVEL  4   // sesuai spek: 4 LED nyala -> pompa ON
#define PUMP_OFF_LEVEL 3   // hysteresis biar relay nggak nge-flap di batas

static inline void relay_write(uint8_t on) {
    uint8_t state = RELAY_ACTIVE_HIGH ? on : !on;
    if (state) PORTD |= (1 << PUMP_BIT);
    else       PORTD &= ~(1 << PUMP_BIT);
}



void softadc_init(void) {
    DDRB  &= ~(1<<COMP_BIT);   // PB0 = input
    PORTB &= ~(1<<COMP_BIT);   // pull-up internal off, LM393 sudah py pull-up sendiri
    DDRB  |= (1<<PWM_BIT);

    TCCR0A = (1<<COM0A1) | (1<<WGM01) | (1<<WGM00);
    TCCR0B = (1<<CS00);
    OCR0A = 0;
    // ACSR tidak dipakai lagi, hapus baris itu
}

uint8_t softadc_read(void) {
    uint8_t result = 0;
    for (int8_t bit = 7; bit >= 0; bit--) {
        uint8_t test = result | (1 << bit);
        OCR0A = test;
        _delay_ms(5);
        if (PINB & (1<<COMP_BIT))   // LM393 output HIGH -> sensor > referensi
            result = test;
    }
    return result;
}

uint8_t value_to_level(uint8_t v) {
    uint8_t level = (v >> 5) + 1;      // 0..255 -> 1..8
    // Kalau sensor kalian "kebalik" (basah = tegangan rendah),
    // ganti baris atas jadi: level = 8 - (v >> 5);
    return level;
}

void show_level(uint8_t level) {
    if (level>=1) PORTD |= (1<<LED1_BIT); else PORTD &= ~(1<<LED1_BIT);
    if (level>=2) PORTD |= (1<<LED2_BIT); else PORTD &= ~(1<<LED2_BIT);
    if (level>=3) PORTD |= (1<<LED3_BIT); else PORTD &= ~(1<<LED3_BIT);
    if (level>=4) PORTB |= (1<<LED4_BIT); else PORTB &= ~(1<<LED4_BIT);
    if (level>=5) PORTB |= (1<<LED5_BIT); else PORTB &= ~(1<<LED5_BIT);
    if (level>=6) PORTB |= (1<<LED6_BIT); else PORTB &= ~(1<<LED6_BIT);
    if (level>=7) PORTB |= (1<<LED7_BIT); else PORTB &= ~(1<<LED7_BIT);
    if (level>=8) PORTB |= (1<<LED8_BIT); else PORTB &= ~(1<<LED8_BIT);
}

int main(void) {
    DDRD |= (1<<LED1_BIT)|(1<<LED2_BIT)|(1<<LED3_BIT)|(1<<PUMP_BIT);
    DDRB |= (1<<LED4_BIT)|(1<<LED5_BIT)|(1<<LED6_BIT)|(1<<LED7_BIT)|(1<<LED8_BIT);
    relay_write(0);

    softadc_init();

    uint8_t pump_on = 0;
    while (1) {
        uint8_t level = value_to_level(softadc_read());
        show_level(level);

        if (!pump_on && level >= PUMP_ON_LEVEL)  pump_on = 1;
        if ( pump_on && level <= PUMP_OFF_LEVEL) pump_on = 0;
        relay_write(pump_on);

        _delay_ms(1000);
    }
}