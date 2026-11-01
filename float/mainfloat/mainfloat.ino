#include "ArduPID.h"
#include "MS5837.h"
#include <DS1307RTC.h>
#include <SD.h>
#include <SPI.h>
#include <Servo.h>
#include <TimeLib.h>
#include <Wire.h>
const int chipSelect = BUILTIN_SDCARD;
MS5837 sensor;
#define RYLR Serial2
#define lights Serial1

int holding = 0;     // how long the float has been maintaing depth
int dataIndex = 0;   // current data point index
float depth = 0.0f;  // meters
float output = 0.0f; // micro seconds
char message;

bool SIM = false; // weather or not its in simualtion mode

float descentDepth = 2; // meters

float setpoint = descentDepth;

float surfaceDepth = 0; // the depth offset in meters

Servo syringe;

double Kp = 360.0, Ki = 18, Kd = 0.0;

float fastTol = 0.20; // meters
float slowTol = 0.05; // meters
float fastMiSc = 315; // micro seconds
float slowMiSc = 50;  // micro seconds

void controller() {
    // this is an adapted form of Dwayne's "five state machine". see
    float error = setpoint - depth;

    if (fabs(error) <= slowTol) {
        output = 1327;
        Serial.println("HOLD_WINDOW");
    } else if (error > fastTol) {
        output = 1327 + fastMiSc;
        Serial.println("FAST_DOWN");
    } else if (error > slowTol) {
        output = 1327 + slowMiSc;
        Serial.println("SLOW_DOWN");
    } else if (error < -fastTol) {
        output = 1327 - fastMiSc;
        Serial.println("FAST_UP");
    } else {
        output = 1327 - slowMiSc;
        Serial.println("SLOW_UP");
    }
}

void sendRadioMessage(String msg) {
    RYLR.print("AT+SEND=");
    RYLR.print(
        "82"); // this NEEDS to be the correct radio id for the base station.
    RYLR.print(",");
    RYLR.print(msg.length());
    RYLR.print(",");
    RYLR.print(msg);
    RYLR.print("\r\n");
}

void checkForCommand() {
    if (RYLR.available() > 0) {
        String line = RYLR.readStringUntil(10, 1000);
        if (line[1] == 82 && line[2] == 67) {
            message = line[10];
        }
        Serial.println(line);
        line = "";
    }
}

void relay() {
    int lineindex = 0;

    File myFile = SD.open("data.txt"); // open the file
    while (true) {
        updateDepth();
        controller();

        syringe.writeMicroseconds(int(output));

        checkForCommand();

        if (message == 'U') { // This means the float has been recovered and
                              // should upload data
            message = 0;      // set the message to null so it doesn't get
                              // interprated twice
            syringe.write(0);
            lights.println("w");
            break;
        }
    }
    while (myFile.available()) {
        String dataLine = myFile.readStringUntil(10, 1000); // get just one line
        delay(500);
        dataLine.trim(); // remove any nonsense

        if (dataLine.length() > 0) {
            sendRadioMessage(dataLine);
            Serial.println("Sent: " + dataLine);
        }

        // wait for the topside to acknowledge
        while (message != 'n' or dataLine == "stop") {
            checkForCommand();
            dataLine = "";
            if (message == 'r') {
                sendRadioMessage(dataLine);
                message = 0; // set the message to null so it doesn't get
                             // interprated twice
            }
        }
        message =
            0; // set the message to null so it doesn't get interprated twice
    }
    sendRadioMessage("stop");
}
void updateDepth() {
    if (SIM) {
        if (Serial.available() > 0) {
            String line = Serial.readStringUntil('\n');
            if (line.length() > 0) {
                depth = line.toFloat();
            }
        }
    } else {
        sensor.read();
        // see caluclations
        // https://docs.google.com/spreadsheets/d/1Wab4LbDww71lHrJmSqJ-hdIlBZDHO0Bx2nWjerMkGF4/edit?usp=sharing

        float extra = 1;
        // idr why is its even like that
        // i remembered
        // our depth sensor spits out wrong depth values but they are roughly
        // linear with the real ones. the math stuff is to convert that to
        // normal
        depth = max(0, (((-104 * ((sensor.depth() * -1)) + 45.1) / 100) -
                        surfaceDepth)) +
                extra;
    }
}

void wait() {
    syringe.writeMicroseconds(1000);
    Serial.println("WATING");
    while (true) {

        checkForCommand();
        // Serial.println(message);
        if (message == 'H') {
            message = 0; // set the message to null so it doesn't get
                         // interprated twice
            lights.print("r");
            syringe.writeMicroseconds(1000);
            RYLR.print("AT+SEND=82,2,hi");
            RYLR.print("\r\n");
            updateDepth();
            controller();

            surfaceDepth = depth;
        }
        if (message == 'D') {
            message = 0; // set the message to null so it doesn't get
                         // interprated twice
            RYLR.print("AT+SEND=82,2,good buoy");
            RYLR.print("\r\n");
            break;
        }
    }
}

void log() {

    File datafile = SD.open("data.txt", FILE_WRITE);
    datafile.print("T");
    datafile.print(second() + (minute() * 60));
    datafile.print("TP");
    datafile.print("P");
    datafile.print(depth);
    datafile.print("PN");
    datafile.print(dataIndex);
    datafile.println("N");
    datafile.close();
    dataIndex++;
}

void descend() {
    setpoint = descentDepth;

    holding = 0;
    while (true) {

        static elapsedMillis pollingTimer;

        updateDepth();
        controller();

        // send a debug message
        // tf is even going on here, like why did i do it like that?!??!
        sendRadioMessage(String(depth) + " : " + String(output) +
                         " ERROR: " + String(abs(descentDepth - depth)) + "," +
                         String(setpoint) + "HOLDE:" + String(holding));

        if (SIM) {
            Serial.print("D");
            Serial.println(output);
        } else {
            syringe.writeMicroseconds(int(output));
        }

        if (pollingTimer >= 5000) {
            pollingTimer = 0;
            log();

            if (abs(descentDepth - depth) < 0.33) {
                holding++;
            } else {
                holding = 0;
            }

            if (holding > 5) {
                break;
            }
        }
        checkForCommand();

        if (message == 'E') { // emergancy stop
            message = 0;      // set the message to null so it doesn't get
                              // interprated twice
            syringe.writeMicroseconds(1000);
            while (true) {
                delay(10);
            }
        }
    }
}

void ascend() {
    syringe.writeMicroseconds(1000);
    holding = 0;
    static elapsedMillis pollingTimer;

    while (true) {
        if (pollingTimer >= 1000) { // check every seccond if it's surfaced
                                    // we want it to stop bobbing before we do anything else
            pollingTimer = 0;
            updateDepth();
            if (abs(depth) < 0.05) {
                holding++;
            } else {
                holding = 0;
            }
        }
        if (holding == 5) {
            break;
        }
    }
}

void setup() { // this whole section is a mess and i've made peace with that

    Serial.begin(115200);
    RYLR.begin(115200);
    lights.begin(9600);
    while (!Serial && millis() < 4000)
        ; // Wait up to 4s for Serial Monitor
    Serial.println("--- STARTING SETUP ---");
    while (!RYLR) {
        Serial.println("D:");
    }
    if (!SIM) {
        syringe.attach(9);
    }
    Serial.println("conneted");
    RYLR.print("AT\r\n");
    Serial.println("Sent 'AT\\r\\n' command. Waiting for response...");
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("SD Card Initialization Failed!");
        return;
    }
    SD.remove("data.txt");
    if (SD.exists("data.txt")) {
        Serial.print(F("Could not delete file"));
        Serial.println("data.txt");
    } else {
        Serial.println(F("All files deleted"));
    }
    if (!SIM) {
        Wire.begin();
        sensor.setModel(MS5837::MS5837_02BA);
        while (!sensor.init()) {
            Serial.println("Init failed!");
            Serial.println("Are SDA/SCL connected correctly?");
            Serial.println("Blue Robotics Bar30: White=SDA, Green=SCL");
            Serial.println("\n\n\n");
            delay(500);
        }
        sensor.setFluidDensity(997);
    }
    delay(1000);
    Serial.setTimeout(10);

    setSyncProvider(RTC.get);
    if (timeStatus() != timeSet)
        Serial.println("Unable to sync with the RTC");
    else
        Serial.println("RTC has set the system time");
}

void loop() {
    // the lights take in single characters over serial corrisponding to
    // predefined colours

    if (!SIM) {
        lights.print("R");
        wait();
    }

    lights.println("y");
    descend();

    lights.println("g");
    ascend();

    lights.println("c");
    descend();

    lights.println("b");
    ascend();

    lights.println("m");
    relay();

    lights.println("w");
    while (true) {
    }
}