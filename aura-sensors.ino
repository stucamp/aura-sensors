#include <HardwareSerial.h>

#include "UBLOX.h"
#include "config.h"

// shared with imu module
volatile bool new_imu_data = false;
int gyros_calibrated = 0; // 0 = uncalibrated, 1 = calibration in progress, 2 = calibration finished
float imu_calib[10]; // the 'safe' and calibrated version of the imu sensors

float receiver_norm[SBUS_CHANNELS];
float autopilot_norm[SBUS_CHANNELS];
float actuator_norm[SBUS_CHANNELS];
uint16_t actuator_pwm[PWM_CHANNELS];

bool new_gps_data = false;
UBLOX gps(3); // ublox m8n
gdata uBloxData;

void setup() {
    // put your setup code here, to run once:

    Serial.begin(DEFAULT_BAUD);
    delay(600); // needed delay before attempting to print anything
    
    Serial.print("\nAura Sensors: Rev "); Serial.println(FIRMWARE_REV);
    
    // The following code (when enabled) will force setting a specific device serial number.
    // set_serial_number(108);
    read_serial_number();
    
    if ( /* true || */ !config_read_eeprom() ) {
        config_load_defaults();
        config_write_eeprom();
    }

    // Serial.print("F_CPU: "); Serial.println(F_CPU);
    // Serial.print("F_PLL: "); Serial.println(F_PLL);
    // Serial.print("BAUD2DIV: "); Serial.println(BAUD2DIV(115200));
    
    Serial.print("Serial Number: ");
    Serial.println(read_serial_number());
    delay(100);

    // initialize the IMU
    imu_setup();
    delay(100);

    // initialize the SBUS receiver
    sbus_setup();

    // initialize PWM output
    pwm_setup();

    // initialize the gps receiver
    gps.begin(115200); 
}

void loop() {
    // put your main code here, to run repeatedly:
    static elapsedMillis myTimer = 0;
    if ( new_imu_data ) {
        cli();
        new_imu_data = false;
        sei();
        update_imu();
    }
    
    while ( sbus_process() ); // keep processing while there is data in the uart buffer

    /* look for a good GPS data packet */
    while ( gps.read(&uBloxData) ) {
        new_gps_data = true;
    }
    
    if ( myTimer > 500 ) {
        myTimer = 0;
        if ( gyros_calibrated == 2 ) {
            imu_print();
            //write_pilot_in_ascii();
            //write_gps_ascii();
        }  
    }
}

