//1.a Deklarasi library yang dipakai
  //#include <LiquidCrystal.h>

//1.b Deklarasi variabel untuk perhitungan kendali PID
  float PID;
  float et, et_1; //--> et: error sekarang, et_1: error sebelumnya
  float eint, eint_1, eint_update; //eint: integral error sekarang, eint_1: integral error sebelum
  float edif; // edif: differential error
  float Kp, Ti, Td, Ki, Kd; // Parameter desain kendali baik scr ZN1, ZN2, CC, IMC, TF based, SSbased
  float SV, PV; // SV: Setpoint Value, PV: Process Value (umpan balik sensor,
  int MV; //MV: manipulated value, sinyal kendali ke output PWM harus integer [0 255]

//1.c Deklarasi variable untuk menghitung Time Sampling
  unsigned long t; //hasil perhitungan fungsi millis();
  double t_1, Ts; // t_1 hasil perhitungan fungsi millis() sebelumnya. Ts: Time sampling

//1.d Deklarasi variable untuk display SV dan PV, penting untuk mengevaluasi kinerja kendali
  float interval_elapsed; //waktu interval yang sudah dilalui
  float interval_limit; // batas interval agar nilai SV, PV dimunculkan
  //LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//1.e Deklarasi pendukung lainnya
  int start; //untuk menjalankan & menghentikan looping arduino

//deklarasi potensio
  float pot;
  #define pot2 3
//deklarasi ultrasonik
  #define echoPin 12 //Echo Pin, OREN
  #define trigPin 11 //Trigger Pin, MERAH
  float duration, distance, distance2; //waktu untuk kalkulasi jarak

//deklarasi kipas 
  int o_kipas = 6;
  
void setup() {
  // put your setup code here, to run once:
  //2.a Setup nilai kendali
    Kp = 18.15; // Kcr = 6.474
    Ti = 0; //Isikan sesuai hasil desain
    Td = 0; //Isikan sesuai hasil desain
    //--> Hitung Ki
    if (Ti==0){
      Ki=0; //untuk menghindari error akibat pembagian 0
    }
    else {
      Ki=Kp/Ti;
    }
    //--> Hitung Kd
    Kd=Kp*Td;

    //---
    et_1=0; //di looping berikutnya nilai et akan di update dg perhitungan
    eint_1=0;

    //2.b Setup untuk DISPLAY
    interval_limit = 0.1; //isikan mau setiap rentang berapa detik nilai akan di display
    interval_elapsed= 0; //nilai awal dari set 0, krn blm melakukan perhitungan

    // men set, besaran dan satuan di LCD
    /*lcd.begin(16,2);
    lcd.setCursor(0,0); //letak penulisan di kolom 0 baris 0 (titik awal)
    lcd.print("SV:");
    lcd.setCursor(0, 1);
    lcd.print("PV:");// karakter yang ditulis
    lcd.setCursor(11, 0); //letak untuk menulis RPM
    lcd.print("RPM"); //ganti sesuai satuan plant, misalnya mbar, L/H, cm, C dll
    lcd.setCursor(11, 1); //letak menulis RPM
    lcd.print("RPM");
    */
    //2.c Setup untuk time sampling
    t=millis(); //acuan awal untuk menghitung Ts
    delay (100); // Isikan dalam milli detik. Pd awalnya di set 0, lali perhatikan besarnya delay rata2
                // melalui serial.print lalu ganti nilai 0. Hal ini untuk menghindari Ts = 0, dimana akan
                // menyebabkan perhitungan differential error jd salah [edif=(et-et_1)/Ts]

    //2.d Setup pin yang dipakagi
    pinMode(7, OUTPUT);
    pinMode(8, INPUT); //Keluaran pin 13 akan dimasukan ke pin ini untuk start-stop looping
    //pinMode(6, OUTPUT); //Keluaran sinyal kendali
    pinMode(o_kipas, OUTPUT);
    
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    //2.e Setup untuk sistem
    digitalWrite(7,HIGH);
    //digitalWrite(8,LOW);
    Serial.begin(9600);
}   

void potensio(){
  // Potensio
  pot=analogRead(A0)*0.1427;
  digitalWrite(pot2, HIGH);

}

void ultrasonik(){
  // Ultrasonik
  digitalWrite(trigPin, LOW);delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
 
//perhitungan untuk dijadikan jarak
  distance = duration/58.2;

// kalibrasi sensor
  distance2=( 42.18-distance);
  //d3 = distance2*3.48;
  //distance2 = map(distance,42.18, 9.18, 0.0, 32.7);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  //3. membaca kondisi variabel start
  start=digitalRead(8);

  //4. cek apakah nilai 'start' atau yang masuk ke pin 8 berlogika 1 (5 V) atau tidak
  if (start == 1){

//5. pembacaan SV, PV
  //5.a Pembacaan SV
      potensio();
      SV=pot;
      //SV=10; // kuantisasi [0 s/d 1023] ke [0 s/d 5], 5/1023 = 0.004887
      
  //5.b Pembacaan PV
      ultrasonik();
      PV=distance2; //kuantisasi [0 s/d 1023] ke [0 s/d 5], 5/1023 = 0.004887
      
  //5.c Hitung time sampling (Ts)
      t=millis(); //menghitung selisih dg instruksi millis () sblmnya
      Ts=(t - t_1)/100; //Ts sdh dalam detik;
  //5.d hitung Error
      et=SV-PV;

  //5.e Hitung Integral Error
      eint_update=((et + et_1)*Ts)/2; //menghitung luasan yang dibentuk oleh error terbaru
      eint=eint_1 + eint_update; //menghitung luas total
  //5.f Hitung Differential error
      edif=(et - et_1)/Ts;  //menghitung perubahan error
  //5.g Hitung PID
      PID = Kp*et + Ki*eint + Kd*edif; 

//6. Membatasi keluaran PID, disesuaikan kondisi hardware Plant.
  //6.a Pembatasan hasil perhitungan, perlu dianalisa, selain dibatasi bisa juga di scale-down
      if (PID > 255){
        PID = 255;
      }
      else if (PID < 0){
        PID=0;
      }
      else {
        PID=PID;
      }
  //6.b Setelah dibatasi disesuaikan dengan keterbatasan embedded system
      //PID = PID; //Keterbatasan output arduino [0 s.d 5], di hardware dikembalikan lagi (dikali 2)
      MV=PID; // Nilai disesuaikan dengan 8 bit output 2^8=255
      analogWrite(o_kipas,MV); //Mengirimkan sinyal kendali ke PWM 6

//7. Display
  //7.a Hitung waktu display
      interval_elapsed = interval_elapsed + Ts; //mengupdate interval_elapsed[IE]
  //7.b Cek apakah nilai interval_elapsed (IE) sudah memenuhi atau > interval_limit(IL)
      if (interval_elapsed >= interval_limit){
  //7.c jika memenuhi nilai akan ditampilkan ke serial plotter dan LCD, kemudian IE di reset
      //untuk menampilkan di serial plotter
        Serial.print(40);
        Serial.print(" ");
        Serial.print(SV); //pengali disesuaikan dengan nilai sensor
        Serial.print(" ");
        Serial.println(PV);
      //Untuk menampilkan di LCD
      /*
        lcd.setCursor(3,0);
        lcd.print(SV);
        lcd.setCursor(3,1);
        lcd.print(PV);
      */
      }

//8 set parameter untuk perhitungan looping selanjutnya
      et_1=et; //di looping berikutnya nilai et akan di update dengan perhitungan
      eint_1=eint; //di looping berikutnya nilai eint akan diupdate dg perhitungan
      t_1=t;

//3 cek nilai 'start' untuk memastikan syarat looping 'while' masih terpenuhi atau tidak
//      start=digitalRead(8);
    Serial.print("pin8");
    Serial.println(start);          
  }
  else{
    start = 0;
    MV = 0;
  }
}
