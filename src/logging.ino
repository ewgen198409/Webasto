void logging(int ignit_fail, float temp_init, int seconds){
    // print all the interesting data

    //New debug variables 
    //Serial.print(" BTN: ");
    //Serial.print(push);
    //Serial.print(" | Glow: ");
    //Serial.print(debug_glow_plug_on);

    
    float water_percentage = (100.00/255.00) * debug_water_percent_map;
    // Serial.print(" WTR SP: ");
    // Serial.print(water_percentage);    
    // Serial.print("/");
    // Serial.print(debug_water_percent_map);    
    
    Serial.print(" | Term: ");
    //New debug variables 
    
    Serial.print(webasto_fail);
    Serial.print(" | Ign Fail #: ");
    Serial.print(ignit_fail);
//    Serial.print(" | SGo: ");
//    Serial.print(shower);
//    Serial.print(" | BGo: ");
//    Serial.print(burn);
//    Serial.print(" | OP Mode: ");
//    if(burn_mode == 0)
//      Serial.print("OFF");
//    if(burn_mode == 1)
//      Serial.print("Starting");
//    if(burn_mode == 2)
//      Serial.print("Running");
//    if(burn_mode == 3)
//      Serial.print("Shuting Down");
    //Serial.print(burn_mode);
    Serial.print(" | W Tmp: ");
    Serial.print(water_temp);
    Serial.print(" | E Tmp: ");
    Serial.print(exhaust_temp);
    if(burn_mode == 1)
    {
      Serial.print("/");
      Serial.print(temp_init+3);
    }
    Serial.print(" | Fan Speed %: ");
    Serial.print(fan_speed);
    Serial.print(" | Fuel HZ ");
    if(delayed_period>0)
      Serial.print(1000.00/delayed_period);
    Serial.print(" | Fuel Need: ");
    Serial.print(fuel_need);
   // Serial.print(" | Glow For (Sec): ");
   // Serial.print(glow_time);
    Serial.print(" | Glow Left: ");
    Serial.print(glow_left);
    //Serial.print(" | Shower TimeOut: ");
    //Serial.print(shower_timeout);
//    Serial.print(" | Lean Burn: ");
//    Serial.print(lean_burn);
    Serial.print(" | Cycle Time: ");
    Serial.print(seconds);
    Serial.print(" | Info: ");
    Serial.println(message); 
    Serial.print(" | Final fuel: ");
    Serial.println(final_fuel);
    Serial.print(" | State: ");
    Serial.println(currentState);

}
