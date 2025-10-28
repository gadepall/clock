#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// ------------------- 7-seg + BCD -------------------
#define BCD_PORT PORTD
#define BCD_DDR DDRD
#define BCD_MASK 0b00111100   // PD2 to PD5

#define COMMON_PORT_D PORTD
#define COMMON_DDR_D DDRD
#define COMMON_MASK_D 0b11000000  // PD6, PD7

#define COMMON_PORT_B PORTB
#define COMMON_DDR_B DDRB
#define COMMON_MASK_B 0b00001111  // PB0–PB3

// ------------------- Buttons -------------------
#define BTN_EDIT_PIN     PC0   // Button 1: Enter/exit edit
#define BTN_NEXT_PIN     PC1   // Button 2: Select next digit
#define BTN_INC_PIN      PC2   // Button 3: Increment
#define BTN_DEC_PIN      PC3   // Button 4: Decrement

// ------------------- Clock Vars -------------------
volatile int seconds = 0, minutes = 0, hours = 0;
volatile uint8_t tick_enable = 1;     // 1 = running, 0 = paused

// ------------------- Edit Mode -------------------
volatile uint8_t edit_mode = 0;
volatile uint8_t edit_digit = 0;      // 0=hr10,1=hr1,2=min10,3=min1,4=sec10,5=sec1
volatile uint16_t blink_counter = 0;
volatile uint8_t blink_state = 0;

// ------------------- Setup -------------------
void setup() {
    // BCD pins
    BCD_DDR |= BCD_MASK;
    BCD_PORT &= ~BCD_MASK;

    // Common anode pins
    COMMON_DDR_D |= COMMON_MASK_D;
    COMMON_DDR_B |= COMMON_MASK_B;
    COMMON_PORT_D &= ~COMMON_MASK_D;
    COMMON_PORT_B &= ~COMMON_MASK_B;

    // Buttons input with pull-up
    DDRC &= ~((1 << BTN_EDIT_PIN) | (1 << BTN_NEXT_PIN) |
              (1 << BTN_INC_PIN) | (1 << BTN_DEC_PIN));
    PORTC |= (1 << BTN_EDIT_PIN) | (1 << BTN_NEXT_PIN) |
             (1 << BTN_INC_PIN) | (1 << BTN_DEC_PIN);

    // Timer1 → 10 Hz interrupt for faster blinking
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC mode, prescaler 1024
    OCR1A = 1562; // 16MHz/1024 = 15625Hz → 10Hz interrupt (15625/10 = 1562.5)
    TIMSK1 = (1 << OCIE1A);

    sei();
}

// ------------------- Timer ISR -------------------
ISR(TIMER1_COMPA_vect) {
    static uint8_t second_counter = 0;
    
    // Update clock at 1Hz (every 10 interrupts)
    if (tick_enable) {
        second_counter++;
        if (second_counter >= 10) {
            second_counter = 0;
            seconds++;
            if (seconds == 60) {
                seconds = 0;
                minutes++;
                if (minutes == 60) {
                    minutes = 0;
                    hours = (hours + 1) % 24;
                }
            }
        }
    }

    // blink counter for edit mode - toggle every interrupt (0.1s)
    if (edit_mode) {
        blink_counter++;
        if (blink_counter >= 1) { // toggle every 0.1s
            blink_counter = 0;
            blink_state ^= 1;
        }
    }
}

// ------------------- Helpers -------------------
void setBCD(int value) {
    BCD_PORT = (BCD_PORT & ~BCD_MASK) | ((value & 0x0F) << 2);
}

// ------------------- Display -------------------
void displayTime(int hours, int minutes, int seconds) {
    int digits[6] = {
        hours / 10, hours % 10,
        minutes / 10, minutes % 10,
        seconds / 10, seconds % 10
    };

    for (int i = 0; i < 6; i++) {
        // If editing and this digit is selected and blink_state=1 → blank
        if (edit_mode && i == edit_digit && blink_state) {
            // Blank → skip enabling display
            COMMON_PORT_D &= ~COMMON_MASK_D;
            COMMON_PORT_B &= ~COMMON_MASK_B;
            _delay_ms(2);
            continue;
        }

        // Set the appropriate common cathode line
        if (i < 2) { // Hours
            COMMON_PORT_D = (COMMON_PORT_D & ~COMMON_MASK_D) | ((1 << (i + 6)) & COMMON_MASK_D);
            COMMON_PORT_B &= ~COMMON_MASK_B;
        } else if (i < 4) { // Minutes
            COMMON_PORT_B = (COMMON_PORT_B & ~COMMON_MASK_B) | ((1 << (i - 2)) & COMMON_MASK_B);
            COMMON_PORT_D &= ~COMMON_MASK_D;
        } else { // Seconds
            COMMON_PORT_B = (COMMON_PORT_B & ~COMMON_MASK_B) | ((1 << (i - 2)) & COMMON_MASK_B);
            COMMON_PORT_D &= ~COMMON_MASK_D;
        }

        setBCD(digits[i]);
        _delay_ms(2);

        // Turn off all digits
        COMMON_PORT_D &= ~COMMON_MASK_D;
        COMMON_PORT_B &= ~COMMON_MASK_B;
    }
}

// ------------------- Main -------------------
int main() {
    setup();

    while (1) {
        // --- Button 1: toggle edit mode ---
        if (!(PINC & (1 << BTN_EDIT_PIN))) {
            _delay_ms(50);
            if (!(PINC & (1 << BTN_EDIT_PIN))) {
                edit_mode ^= 1;
                if (edit_mode) {
                    tick_enable = 0;   // pause clock
                    edit_digit = 0;    // start from hour tens
                    blink_state = 0;   // start with digit visible
                    blink_counter = 0; // reset blink counter
                } else {
                    tick_enable = 1;   // resume clock
                }
                while (!(PINC & (1 << BTN_EDIT_PIN)));
            }
        }

        // --- Button 2: move to next digit ---
        if (edit_mode && !(PINC & (1 << BTN_NEXT_PIN))) {
            _delay_ms(50);
            if (!(PINC & (1 << BTN_NEXT_PIN))) {
                edit_digit = (edit_digit + 1) % 6; // move to next digit
                blink_state = 0;   // reset blink state to visible
                blink_counter = 0; // reset blink counter
                while (!(PINC & (1 << BTN_NEXT_PIN)));
            }
        }

        // --- Button 3: increment ---
        if (edit_mode && !(PINC & (1 << BTN_INC_PIN))) {
            _delay_ms(50);
            if (!(PINC & (1 << BTN_INC_PIN))) {
                switch (edit_digit) {
                    case 0: // Hour tens
                        hours = ((hours / 10 + 1) % 3) * 10 + (hours % 10);
                        if (hours >= 24) hours = 0;
                        break;
                    case 1: // Hour units
                        hours = (hours / 10) * 10 + ((hours % 10 + 1) % 10);
                        if (hours >= 24) hours = 0;
                        break;
                    case 2: // Minute tens
                        minutes = ((minutes / 10 + 1) % 6) * 10 + minutes % 10;
                        break;
                    case 3: // Minute units
                        minutes = (minutes / 10) * 10 + ((minutes % 10 + 1) % 10);
                        break;
                    case 4: // Second tens
                        seconds = ((seconds / 10 + 1) % 6) * 10 + seconds % 10;
                        break;
                    case 5: // Second units
                        seconds = (seconds / 10) * 10 + ((seconds % 10 + 1) % 10);
                        break;
                }
                while (!(PINC & (1 << BTN_INC_PIN)));
            }
        }

        // --- Button 4: decrement ---
        if (edit_mode && !(PINC & (1 << BTN_DEC_PIN))) {
            _delay_ms(50);
            if (!(PINC & (1 << BTN_DEC_PIN))) {
                switch (edit_digit) {
                    case 0: // Hour tens
                        hours = ((hours / 10 + 2) % 3) * 10 + (hours % 10);
                        if (hours >= 24) hours = 20 + (hours % 10);
                        break;
                    case 1: // Hour units
                        hours = (hours / 10) * 10 + ((hours % 10 + 9) % 10);
                        if (hours >= 24) hours = 23;
                        break;
                    case 2: // Minute tens
                        minutes = ((minutes / 10 + 5) % 6) * 10 + minutes % 10;
                        break;
                    case 3: // Minute units
                        minutes = (minutes / 10) * 10 + ((minutes % 10 + 9) % 10);
                        break;
                    case 4: // Second tens
                        seconds = ((seconds / 10 + 5) % 6) * 10 + seconds % 10;
                        break;
                    case 5: // Second units
                        seconds = (seconds / 10) * 10 + ((seconds % 10 + 9) % 10);
                        break;
                }
                while (!(PINC & (1 << BTN_DEC_PIN)));
            }
        }

        // Always display
        displayTime(hours, minutes, seconds);
    }
}
