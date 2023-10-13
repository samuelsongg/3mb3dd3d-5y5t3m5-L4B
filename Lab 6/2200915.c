#include <stdio.h> 
#include "pico/stdlib.h" // For sleep_ms()
 
float Kp = 2.0; 
float Ki = 0.2; 
float Kd = 0.02; 
 
// Function to compute the control signal 
float compute_pid(float setpoint, float current_value, float *integral, float *prev_error) { 
    float error = setpoint - current_value;  // Reversed the error calculation 
 
    *integral += error; 
 
    float derivative = error - *prev_error; 
 
    float control_signal = Kp * error + Ki * (*integral) + Kd * derivative; 
 
    *prev_error = error;  // Set previous error to current error 
 
    return control_signal; 
} 
 
int main() { 
    stdio_init_all();

    float setpoint = 100.0;  // Desired position 
    float current_value = 0.0;  // Current position 
    float integral = 0.0;  // Integral term 
    float prev_error = 0.0;  // Previous error term 
 
    float time_step = 0.1; 
    int num_iterations = 100;  // Add a missing semicolon 
 
    // Simulate the control loop 
    for (int i = 0; i < num_iterations; i++) { 
        float control_signal = compute_pid(setpoint, current_value, &integral, &prev_error);  // Pass the address of prev_error 
 
        float motor_response = control_signal * 0.05;  // Motor response model 
 
        current_value += motor_response;  // Update the current value 
 
        printf("Iteration %d: Control Signal = %f, Current Position = %f\n", i, control_signal, current_value); 
 
        sleep_ms(time_step * 1000);  // Convert to milliseconds
    } 
 
    return 0; 
}

/*** end of file ***/
