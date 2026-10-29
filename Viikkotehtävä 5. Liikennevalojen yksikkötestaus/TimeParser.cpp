//tavoittelen 2 pistettä Yksinkertaiset testit ja parseri sulautetussa ohjelmassa + 1p suoritus: Lisätään testikeissejä
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "TimeParser.h"

// time format: HHMMSS (6 characters)
int time_parse(char *time) {

    // how many seconds, default returns error
    int seconds = TIME_LEN_ERROR;

    // TODO: Check that string is not null
    // Tarkistetaan null-pointteri ja oikea pituus (6 merkkiä)
    if (time == NULL || strlen(time) != 6) {
        return TIME_LEN_ERROR; // Tai TIME_ARRAY_ERROR (-1)
    }

    // Tarkistetaan, että kaikki merkit ovat numeroita
    for (int i = 0; i < 6; i++) {
        if (!isdigit(time[i])) {
            return -4; // KORJATTU: Testi odottaa arvoa -4
        }
    }
    // Parse values from time string
    // For example: 124033 -> 12hour 40min 33sec
    int values[3];
    values[2] = atoi(time+4); // seconds
    time[4] = 0;
    values[1] = atoi(time+2); // minutes
    time[2] = 0;
    values[0] = atoi(time); // hours
    
    // Now you have:
    // values[0] hour
    // values[1] minute
    // values[2] second

    // TODO: Add boundary check time values: below zero or above limit not allowed
    // limits are 59 for minutes, 23 for hours, etc
    if (values[0] < 0 || values[0] > 23 ||
        values[1] < 0 || values[1] > 59 ||
        values[2] < 0 || values[2] > 59) {
        return -3; // KORJATTU: Testi odottaa arvoa -3
    }

    // TODO: Calculate return value from the parsed minutes and seconds
    // Otherwise error will be returned!
    // KORJATTU: Lisätty tunnit (values[0] * 3600) mukaan laskuun!
    seconds = (values[0] * 3600) + (values[1] * 60) + values[2];
    
    // Tarkistetaan että aika ei ole 0 sekuntia
    // KORJATTU: Kommentoitu pois, jotta keskiyön (000000 = 0s) testi ei kaadu
    /*
    if (seconds == 0) {
        return -5; // Tai mikä tahansa -5 virhekoodi
    }
    */

    return seconds;
}
