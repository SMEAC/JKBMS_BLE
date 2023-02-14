
void Datenanalyse() {

  uint8_t j,i;
  
  Battery_Voltage = 0.0;
  for(j=0,i=7; i<38; j++,i+=2) {
    cellVoltage[j] = (((int)receivedBytes_main[i] << 8 | receivedBytes_main[i-1])*0.001);
    Battery_Voltage += cellVoltage[j];
  }

  Serial.print("Cell Voltages = ");
  for(i=0; i<16; i++) {
    Serial.print(cellVoltage[i], 3); 
    i<15 ? Serial.print("V, ") : Serial.println("V");
  }
  Serial.print("Battery Voltage = ");
  Serial.print(Battery_Voltage, 3);
  Serial.println("V");

  Percent_Remain = ((int)receivedBytes_main[173]); 
  Serial.print("Percent Remaining = ");
  Serial.print(Percent_Remain);
  Serial.println("%");

  Capacity_Remain = (((int)receivedBytes_main[177] << 24 | receivedBytes_main[176] << 16 | receivedBytes_main[175] << 8 | receivedBytes_main[174])*0.001);     
  Serial.print("Capacity Remaining = ");
  Serial.print(Capacity_Remain);
  Serial.println("Ah");

  Nominal_Capacity = (((int)receivedBytes_main[181] << 24 | receivedBytes_main[180] << 16 | receivedBytes_main[179] << 8 | receivedBytes_main[178])*0.001); 
  Serial.print("Nominal Capacity = ");
  Serial.print(Nominal_Capacity);
  Serial.println("Ah");

  Percent_Remain_calc = Capacity_Remain / Nominal_Capacity;
  Serial.print("Percent Remaining (calc) = ");
  Serial.print(Percent_Remain_calc);
  Serial.println("%");    


  Battery_Power = (((int)receivedBytes_main[157] << 24 | (int)receivedBytes_main[156] << 16 | (int)receivedBytes_main[155] << 8 | (int)receivedBytes_main[154])*0.001);
  Serial.print("Battery Power = ");
  Serial.print(Battery_Power);
  Serial.println("W"); 

  Charge_Current = (((int)receivedBytes_main[161] << 24 | receivedBytes_main[160] << 16 | receivedBytes_main[159] << 8 | receivedBytes_main[158])*0.001);
  Serial.print("Battery Current = ");
  Serial.print(Charge_Current);
  Serial.println("A"); 

  new_data = false;

}
