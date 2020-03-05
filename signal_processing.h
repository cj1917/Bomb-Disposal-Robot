/* 
 * File:   signal_processing.h
 * Author: charl
 *
 * Created on 21 February 2020, 18:21
 *
 */

#ifndef SIGNAL_PROCESSING_H
#define	SIGNAL_PROCESSING_H

// structure to store properties for each sensor
struct Sensor { 
    unsigned int raw_data; // raw PWM duty cycle
    unsigned int smoothed_signal; // signal after smoothing and processing
};

void init_sensor(void);
char classify_data(unsigned int smoothed_data, unsigned int *smoothed);

#endif	/* SIGNAL_PROCESSING_H */

