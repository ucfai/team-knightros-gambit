// There are 9 different slope settings per circle, 1/8 and 8 have only one setting,
// the other seven all have small and large step settings
int pulsesPerSlope[NUM_CIRCLES][NUM_SLOPES_PER_QUARTER_CIRCLE * 2 - 2];

enum SlopeToIndex
{
    SLOPE_HORIZONTAL_FAST = 0,
    SLOPE_HORIZONTAL_SLOW = 1,
    SLOPE_EIGHTH = 2,
    SLOPE_QUARTER_FAST = 3,
    SLOPE_QUARTER_SLOW = 4,
    SLOPE_HALF_FAST = 5,
    SLOPE_HALF_SLOW = 6,
    SLOPE_ONE_FAST = 7,
    SLOPE_ONE_SLOW = 8,
    SLOPE_TWO_FAST = 9,
    SLOPE_TWO_SLOW = 10,
    SLOPE_FOUR_FAST = 11,
    SLOPE_FOUR_SLOW = 12,
    SLOPE_EIGHT = 13,
    SLOPE_VERTICAL_FAST = 14,
    SLOPE_VERTICAL_SLOW = 15
};


// Makes a full circle of the given size (0=largest, NUM_CIRCLES-1=smallest) 
// starting from the given quadrant (0=top, 1=left, 2=bottom, 3=right).
// calculatePulsesPerSlope() must be called before makeCircle()
void makeCircle(int circle, int firstQuarter)
{
    bool firstPass = true;
    int slopeIndex;

    setScale(xMotor, EIGHTH_STEPS);
    digitalWrite(xMotor[STEP_PIN], LOW);

    for (int quarter = firstQuarter; quarter != firstQuarter || firstPass; quarter = (quarter + 1) % 4)
    {
        firstPass = false;

        // Set Y-direction
        if (quarter == 0 || quarter == 1)
            digitalWrite(yMotor[DIR_PIN], DOWN);
        else
            digitalWrite(yMotor[DIR_PIN], UP);

        // Set X-direction
        if (quarter == 0 || quarter == 3)
            digitalWrite(xMotor[DIR_PIN], LEFT);
        else
            digitalWrite(xMotor[DIR_PIN], RIGHT);

        // Set starting point for the while loop
        if (quarter == 0 || quarter == 2)
            slopeIndex = SLOPE_HORIZONTAL_SLOW;
        else
            slopeIndex = NUM_SLOPES_PER_QUARTER_CIRCLE - 1;

        // Iterate through pulsesPerSlope
        while (slopeIndex >= SLOPE_HORIZONTAL_FAST  &&  slopeIndex <= SLOPE_VERTICAL_SLOW)
        {
            // Y-scale
            if (slopeIndex == SLOPE_TWO_SLOW || slopeIndex == SLOPE_QUARTER_FAST)
                setScale(yMotor, QUARTER_STEPS);    
            else if (slopeIndex == SLOPE_FOUR_SLOW || slopeIndex == SLOPE_HALF_FAST)
                setScale(yMotor, HALF_STEPS);    
            else if (slopeIndex == SLOPE_EIGHT || slopeIndex == SLOPE_VERTICAL_FAST || slopeIndex == SLOPE_ONE_FAST ||
                     slopeIndex == SLOPE_FOUR_FAST || slopeIndex == SLOPE_TWO_FAST)
                setScale(yMotor, WHOLE_STEPS);    
            else
                setScale(yMotor, EIGHTH_STEPS);    

            // X-scale
            if (slopeIndex == SLOPE_EIGHTH || slopeIndex == SLOPE_HORIZONTAL_FAST || slopeIndex == SLOPE_ONE_FAST ||
                slopeIndex == SLOPE_QUARTER_FAST || slopeIndex == SLOPE_HALF_FAST)
                setScale(xMotor, WHOLE_STEPS);
            else if (slopeIndex == SLOPE_QUARTER_SLOW || slopeIndex == SLOPE_TWO_FAST)
                setScale(xMotor, HALF_STEPS);
            else if (slopeIndex == SLOPE_HALF_SLOW || slopeIndex == SLOPE_FOUR_FAST)
                setScale(xMotor, QUARTER_STEPS);
            else 
                setScale(xMotor, EIGHTH_STEPS);

            // Send pulses for the current slopeIndex
            // Separate loops for vertical and horizontal cases to optimize performance
            if (slopeIndex == SLOPE_VERTICAL_SLOW || slopeIndex == SLOPE_VERTICAL_FAST)
            {
                for (int i = 0; i < pulsesPerSlope[circle][slopeIndex]; i++)
                {
                    // Y-Step
                    digitalWrite(yMotor[STEP_PIN], LOW);
                    digitalWrite(yMotor[STEP_PIN], HIGH);
                }
            }
            else if (slopeIndex == SLOPE_HORIZONTAL_SLOW || slopeIndex == SLOPE_HORIZONTAL_FAST)
            {
                for (int i = 0; i < pulsesPerSlope[circle][slopeIndex]; i++)
                {
                    // X-Step
                    digitalWrite(xMotor[STEP_PIN], LOW);
                    digitalWrite(xMotor[STEP_PIN], HIGH);
                }
            }
            else
            {
                for (int i = 0; i < pulsesPerSlope[circle][slopeIndex]; i++)
                {
                    // X-Step
                    digitalWrite(xMotor[STEP_PIN], LOW);
                    digitalWrite(xMotor[STEP_PIN], HIGH);

                    // Y-Step
                    digitalWrite(yMotor[STEP_PIN], LOW);
                    digitalWrite(yMotor[STEP_PIN], HIGH);
                }
            }

            // Increment or Decrement slopeIndex
            if (quarter == 0 || quarter == 2)
                slopeIndex++;
            else
                slopeIndex--;
        }
    }
}

// Calculates and stores the number of pulses that need to be made at each slope
// Creates evenly spaced circles based on MILLIMETERS_PER_UNITSPACE, STEPS_PER_MILLIMETER, and NUM_CIRCLES 
void calculatePulsesPerSlope(){

    int outerRadius, deltaR;

    // Calculate the radius of the largest circle in eighth steps
    outerRadius = MILLIMETERS_PER_UNITSPACE * STEPS_PER_MILLIMETER * 8;

    // Ensure that each radius is evenly spaced from the center
    outerRadius = outerRadius - outerRadius % NUM_CIRCLES;

    // Calculate the spacing between the circles
    deltaR = outerRadius / NUM_CIRCLES;

    // Loop over each circle
    for (int circle = 0; circle < NUM_CIRCLES; circle++)
    {
        int radius, xStepsRemaining, yStepsRemaining;

        // Radius of the current circle
        radius = outerRadius - deltaR * circle;

        // StepsRemaining records the number of steps that need to be taken to reach the leftmost point on the circle
        // Initialize to the total number of steps per quarter circle. The starting point is the topmost point
        xStepsRemaining = radius;
        yStepsRemaining = radius;

        // Horizontal steps
        while(getInstantaneousSlope(radius, xStepsRemaining, yStepsRemaining) < 1.0/16.0)
        {
           pulsesPerSlope[circle][SLOPE_HORIZONTAL_SLOW]++;
           xStepsRemaining--;
        }

        // Eighth slope steps
        while(getInstantaneousSlope(radius, xStepsRemaining, yStepsRemaining) < 3.0/16.0  &&  xStepsRemaining >= 8)
        {
            pulsesPerSlope[circle][SLOPE_EIGHTH]++;
            xStepsRemaining -= 8;
            yStepsRemaining--;
        }

        // Quarter slope steps
        while(getInstantaneousSlope(radius, xStepsRemaining, yStepsRemaining) < 3.0/8.0  &&  xStepsRemaining >= 4)
        {
            pulsesPerSlope[circle][SLOPE_QUARTER_SLOW]++;
            xStepsRemaining -= 4;
            yStepsRemaining--;
        }

        // Half slope steps
        while(getInstantaneousSlope(radius, xStepsRemaining, yStepsRemaining) < 3.0/4.0  &&  xStepsRemaining >= 2)
        {
            pulsesPerSlope[circle][SLOPE_HALF_SLOW]++;
            xStepsRemaining -= 2;
            yStepsRemaining--;
        }

        // Slope=1 steps
        while(getInstantaneousSlope(radius, xStepsRemaining, yStepsRemaining) < 3.0/2.0)
        {
            pulsesPerSlope[circle][SLOPE_ONE_SLOW]++;
            xStepsRemaining--;
            yStepsRemaining--;
        }

        // Slope=2 steps
        while(getInstantaneousSlope(radius, xStepsRemaining, yStepsRemaining) < 3.0  &&  yStepsRemaining >= 2)
        {
            pulsesPerSlope[circle][SLOPE_TWO_SLOW]++;
            xStepsRemaining--;
            yStepsRemaining -= 2;
        }

        // Slope=4 steps
        while(getInstantaneousSlope(radius, xStepsRemaining, yStepsRemaining) < 6.0  &&  yStepsRemaining >= 4)
        {
            pulsesPerSlope[circle][SLOPE_FOUR_SLOW]++;
            xStepsRemaining--;
            yStepsRemaining -= 4;
        }

        // Slope=8 steps
        while(getInstantaneousSlope(radius, xStepsRemaining, yStepsRemaining) < 12.0  &&  yStepsRemaining >= 8)
        {
            pulsesPerSlope[circle][SLOPE_EIGHT]++;
            xStepsRemaining--;
            yStepsRemaining -= 8;
        }

        // Add any leftover x-steps to the beginning of the circle
        pulsesPerSlope[circle][SLOPE_HORIZONTAL_SLOW] += xStepsRemaining;

        // Remaining y-steps are vertical moves at the end
        pulsesPerSlope[circle][SLOPE_VERTICAL_SLOW] += yStepsRemaining;

        // Calculate the number of maximum scale steps and the number of leftover small steps at each slope
        // ex: 13 Horizontal steps at 1/8 scale becomes 1 step at 1/1 scale and 5 steps at 1/8 scale
        pulsesPerSlope[circle][SLOPE_HORIZONTAL_FAST] = pulsesPerSlope[circle][SLOPE_HORIZONTAL_SLOW] / 8;
        pulsesPerSlope[circle][SLOPE_HORIZONTAL_SLOW] %= 8;
        pulsesPerSlope[circle][SLOPE_QUARTER_FAST] = pulsesPerSlope[circle][SLOPE_QUARTER_SLOW] / 2;
        pulsesPerSlope[circle][SLOPE_QUARTER_SLOW] %= 2;
        pulsesPerSlope[circle][SLOPE_HALF_FAST] = pulsesPerSlope[circle][SLOPE_HALF_SLOW] / 4;
        pulsesPerSlope[circle][SLOPE_HALF_SLOW] %= 4;
        pulsesPerSlope[circle][SLOPE_ONE_FAST] = pulsesPerSlope[circle][SLOPE_ONE_SLOW] / 8;
        pulsesPerSlope[circle][SLOPE_ONE_SLOW] %= 8;
        pulsesPerSlope[circle][SLOPE_TWO_FAST] = pulsesPerSlope[circle][SLOPE_TWO_SLOW] / 4;
        pulsesPerSlope[circle][SLOPE_TWO_SLOW] %= 4;
        pulsesPerSlope[circle][SLOPE_FOUR_FAST] = pulsesPerSlope[circle][SLOPE_FOUR_SLOW] / 2;
        pulsesPerSlope[circle][SLOPE_FOUR_SLOW] %= 2;
        pulsesPerSlope[circle][SLOPE_VERTICAL_FAST] = pulsesPerSlope[circle][SLOPE_VERTICAL_SLOW] / 8;
        pulsesPerSlope[circle][SLOPE_VERTICAL_SLOW] %= 8;
    }
}

// Returns the instantaneous slope of a circle at the current x and y coordinates
// xStepsRemaining and yStepsRemaining must be less than or equal to the radius
float getInstantaneousSlope(int radius, int xStepsRemaining, int yStepsRemaining)
{
    // Return extremely steep slope when yStepsRemaining == 0
    if (yStepsRemaining == 0)
        return 10000.0;
    
    return ((float) radius - xStepsRemaining) / yStepsRemaining;
}
