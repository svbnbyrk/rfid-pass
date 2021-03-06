#include <Servo.h>
#include <LiquidCrystal.h>
#include <EEPROM.h> //Okuma ve yazma işini UID ile EEPROM a kaydedicez.
#include <SPI.h> //RC522 modülü SPI protokolünü kullanır.
#include <MFRC522.h> //Mifare RC522 kütüphanesi

#define COMMON_ANODE //kullandığınız ledlere göre yanık veya sönük(H-L) kodlarını düzenleyiniz.
#ifdef COMMON_ANODE
#define LED_ON HIGH
#define LED_OFF LOW
#else
#define LED_ON LOW
#define LED_OFF HIGH
#endif
 
#define redLed A5 // Led pinlerini seçtik.
#define greenLed A4
#define blueLed A2
Servo myservomotor; // sınıfın bir örneğini aldık
#define wipeB A3 //Master Card'ı silmek için buton pinini sseçtik.(Dilerseniz buton koymayın direk 3.pini GND ye verin.)




boolean match=false;
boolean programMode=false; //Programlama modu başlatma.
 
int successRead; //Başarılı bir şekilde sayı okuyabilmek için integer atıyoruz.
 
byte storedCard[4]; //Kart EEPROM tarafından okundu.
byte readCard[4]; //RFID modül ile ID tarandı.
byte masterCard[4]; //Master kart ID'si EEPROM'a aktarıldı.
 
/*
MOSI: Pin 11 / ICSP-4
MISO: Pin 12 / ICSP-1
SCK : Pin 13 / ICSP-3
SS(SDA) : Pin 10 (Configurable)
RST : Pin 9 (Configurable)
*/
LiquidCrystal lcd(7, 6, 5, 4, 3, 2); 
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN,RST_PIN); //MFRC522 örneği oluşturduk.
///////////////////////YÜKLEME/////////////////////
 
void setup()
{
lcd.begin(16, 2); // LCD ekran arayüzü başlatır ve ekran boyutları (genişlik ve yükseklik) belirtir
//Arduino Pin Konfigrasyonu
pinMode(redLed,OUTPUT);
pinMode(greenLed,OUTPUT);
pinMode(blueLed,OUTPUT);
pinMode(wipeB,INPUT_PULLUP);

myservomotor.attach(8);  // arduinonun 0. pinini çıkış yaptık.
myservomotor.write(0);  // motora ilk 0.derecesinden başlaması komutunu verdik

//Kapının kilitli olduğundan enin olun.
digitalWrite(redLed,LED_OFF); //Ledin sönük olduğundan emin olun.
digitalWrite(greenLed,LED_OFF); //Ledin sönük olduğundan emin olun.
digitalWrite(blueLed,LED_OFF); //Ledin sönük olduğundan emin olun.
 
//Protokol konfigrasyonu
Serial.begin(9600); //PC ile seri iletim başlat.
SPI.begin(); //MFRC522 donanımı SPI protokolünü kullanır.
mfrc522.PCD_Init(); //MFRC522 donanımını başlat.

lcd.clear(); 
lcd.setCursor(6,0);
lcd.print("RFID "); 
lcd.setCursor(0,1);
lcd.setCursor(2,1);
lcd.print("Gecis Sistemi");
  

Serial.println(F("-----Sisteme Hosgeldin----- ")); //Hata ayıklama amacıyla
ShowReaderDetails(); //PCD ve MFRC522 kart okuyucu ayrıntılarını göster.
 
//Silme kodu butona basıldığında EEPROM içindeki bilgileri siler.
if(digitalRead(wipeB)==LOW)
{
//Butona bastığınızda pini düşük almalısınız çünkü buton toprağa bağlı.
lcd.clear();
lcd.print("Butona bastiniz");   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.print("EEPROM Siliniyor!");
delay(5000);

digitalWrite(redLed,LED_ON); //Kırmızı led sildiğimizi bilgilendirmek amacıyla yanık kalır.
Serial.println(F("Silme butonuna basildi."));
Serial.println(F("5 saniye icinde iptal edebilirsiniz.."));
Serial.println(F("Tum kayitlar silinecek ve bu islem geri alinamayacak."));
delay(5000); //Kullanıcıya iptal işlemi için yeterli zaman verin.

if (digitalRead(wipeB)==LOW) //Düğmeye basılmaya devam ediliyorsa EEPROM u sil.
{
Serial.println(F("EEPROM silinmeye baslaniyor."));
for (int x=0; x<EEPROM.length();x=x+1) //EEPROM adresinin döngü sonu
{
if(EEPROM.read(x)==0) //EEPROM adresi sıfır olursa
{
 
}
else
{
EEPROM.write(x,0);
}
}
lcd.clear();
lcd.print("Kayitlar Silindi!");
delay(2000);

Serial.println(F("EEPROM Basariyla Silindi.."));
digitalWrite(redLed,LED_OFF);
delay(200);
digitalWrite(redLed,LED_ON);
delay(200);
digitalWrite(redLed,LED_OFF);
delay(200);
digitalWrite(redLed,LED_ON);
delay(200);
digitalWrite(redLed,LED_OFF);
}
 
else
{
lcd.clear();
lcd.print("Silme islemi");
lcd.setCursor(0, 1);
lcd.print("iptal edildi.");
delay(2000);
Serial.println(F("Silme islemi iptal edildi."));
digitalWrite(redLed,LED_OFF);
}
}
 
/*Master Kart tanımlı mı kontrol edilecek.Eğer kullanıcı Master Kart seçmediyse
yeni bir Master Kart EEPROM a kaydedilecek.EEPROM kayıtlarının haricinde 143 EEPROM adresi tutulabilir.
EEPROM adresi sihirli sayısı 143.
*/
 
if (EEPROM.read(1) != 143)
{
lcd.clear();
lcd.print("Master kart");
lcd.setCursor(0, 1);
lcd.print("Tanimlayiniz!");

Serial.println(F("Sistemde kayitli master card bulunmamaktadir."));
Serial.println(F("Master Karti secmek icin kartinizi okutunuz.."));
do
{
successRead=getID(); //successRead 1 olduğu zaman okuyucu düzenlenir aksi halde 0 olucaktır.
digitalWrite(blueLed,LED_ON); //Master Kartın kaydedilmesi için gösterildiğini ifade eder.
delay(200);
digitalWrite(blueLed,LED_OFF);
delay(200);
}
while(!successRead); //Başarılı bir şekilde okuyamadıysa Başarılı okuma işlemini yapmicaktır.
for (int j=0; j<4; j++) //4 kez döngü
{
EEPROM.write(2+j,readCard[j]); //UID EPPROM a yazıldı, 3. adres başla.
}
EEPROM.write(1,143); //EEPROM a Master Kartı kaydettik.
lcd.clear();
lcd.print("Master Card:");
lcd.setCursor(0, 1);
lcd.print(readCard[1,143]);
delay(2000);
lcd.clear();
 
lcd.setCursor(6,0);
lcd.print("RFID "); 
lcd.setCursor(0,1);
lcd.setCursor(2,1);
lcd.print("Gecis Sistemi");

Serial.println(F("Master Kart kaydedildi.."));
}
 
Serial.println(F("*****************"));
Serial.println(F("Master Kart ID:"));
for(int i=0; i<4; i++) //EEPROM da Master Kartın UID'si okundu.
{ //Master Kart yazıldı.
masterCard[i]=EEPROM.read(2+i);
Serial.print(masterCard[i],HEX);
}
 
Serial.println("");
Serial.println(F("***************"));
Serial.println(F("Sistem hazir!"));
Serial.println(F("Lutfen kartinizi okutunuz..."));
cycleLeds(); //Herşeyin hazır olduğunu kullanıcıya haber vermek için geri bildirim.
}
void loop()
{
do
{
successRead=getID(); //SuccessRead 1 olursa kartı oku aksi halde 0
if(programMode)
{
cycleLeds(); //Program modu geri bildirimi yeni kart okutulmasını bekliyor.
}
else
{
normalModeOn(); //Normal mod, mavi led açık diğer tümü kapalı.
}
}
while(!successRead);
if(programMode)
{
if(isMaster(readCard)) //Master Kart tekrar okutulursa programdan çıkar.
{
lcd.clear();
lcd.print("Master kart");   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.print("okundu");
delay(2000);
lcd.clear();
lcd.print("Cikis yapildi!");
delay(2000);
lcd.clear();
lcd.setCursor(6,0);
lcd.print("RFID "); 
lcd.setCursor(0,1);
lcd.setCursor(2,1);
lcd.print("Gecis Sistemi");



Serial.println(F("Master Kart Okundu.."));
Serial.println(F("Programdan cikis yapiliyor.."));
Serial.println(F("*********************"));
programMode=false;
return;
}
else
{
if(findID(readCard)) //Okunan kart silinmek isteniyorsa
{
deleteID(readCard);
int count=EEPROM.read(0);
lcd.clear();
lcd.print("Okunan kart");   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.print("silindi!");
delay(2000);
lcd.clear();
lcd.setCursor(6,0);
lcd.print(count);   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.print("Kullanici aktif");

Serial.println(F("Okunan kart siliniyor.."));
Serial.println(F("****************"));
}
else //Okunan kart kaydedilmek isteniyorsa
{
writeID(readCard);
int count=EEPROM.read(0);
lcd.clear();
lcd.print("Okunan kart");   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.print("EEPROM'a");
delay(2000);
lcd.clear();
lcd.print("kayit ediliyor.");
delay(2000);
lcd.clear();
lcd.setCursor(6,0);
lcd.print(count);   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.print("Kullanici aktif");
 
Serial.println(F("Okunan kart ID'si EEPROM'a kaydediliyor.."));

Serial.println(F("**************************"));
}
}
}
else
{
if(isMaster(readCard)) //Master Kart okunursa programa giriş yapılıyor.
{
programMode=true;


Serial.println(F("Programa giris yapiliyor..."));
int count=EEPROM.read(0);
Serial.print(F("Sistemde suan  "));
Serial.print(count);
Serial.print(F(" adet kullanici aktif durumdadir."));
Serial.println("");
Serial.println(F("Eklemek veya cikarmak istediginiz karti okutunuz."));
Serial.println(F("///////////\\\\\\\\\\\\"));

lcd.clear();
lcd.setCursor(6,0);
lcd.print(count);   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.print("Kullanici aktif");
}
else
{
if(findID(readCard))
{
lcd.clear();
lcd.print("ID Eslesti");   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.print("Hosgeldiniz!");
delay(3000);
lcd.clear();


Serial.println(F("ID numarasi eslesti..."));
Serial.println(F("Gecis izni verildi!"));
granted(); //300 milisaniyede kilitli kapıyı aç.
lcd.setCursor(6,0);
lcd.print("RFID "); 
lcd.setCursor(0,1);
lcd.setCursor(2,1);
lcd.print("Gecis Sistemi");

}
else
{
lcd.clear();
lcd.print("ID Eslesmedi");   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.setCursor(4, 1);
lcd.print("UYARI!");
delay(3000);
lcd.clear();
Serial.println(F("ID numarasi eslesmedi..."));
Serial.println(F("Gecis izni verilmedi!"));
denied();
lcd.setCursor(6,0);
lcd.print("RFID "); 
lcd.setCursor(0,1);
lcd.setCursor(2,1);
lcd.print("Gecis Sistemi");

}

}
}
}
 
////////////////////ERİŞİM İZNİ//////////////////////
 
void granted()
{
digitalWrite(blueLed,LED_OFF); //mavi led sönük
digitalWrite(redLed,LED_OFF); //kırmızı led sönük
digitalWrite(greenLed,LED_ON); //yeşil led yanık
 for(int derece = 0; derece < 90; derece=derece+10){   // 180 derecelik açı yap
myservomotor.write(derece);
delay(25);   // her 1 derece açıyı 10 milisaniyede yapacak
}
delay(4000);
myservomotor.write(0);
delay(1000); //1 saniye sonra yeşil led de sönücek
}
 
////////////////ERİŞİM İZNİ VERİLMEDİ/////////////////////

void denied()
{
digitalWrite(greenLed,LED_OFF); //yeşil ledin sönük olduğundan emin olun
digitalWrite(blueLed,LED_OFF); //mavi ledin sönük olduğuna emin olun
digitalWrite(redLed,LED_ON); //kırmızı led yanık
delay(1000);
}
 
/////////////////////KART OKUYUCU UID AYARLAMA////////////////////////////
 
int getID()
{
//Kart okuyucuyu hazır ediyoruz
if(!mfrc522.PICC_IsNewCardPresent()) //yeni bir kart okutun vve devam edin.
{
return 0;
}
if(!mfrc522.PICC_ReadCardSerial()) //kartın serial numarasını alın ve devam edin.
{
return 0;
}
//4 ve 7 byte UID'ler mevcut biz 4 byte olanı kullanıcaz
Serial.println(F("Kartin ID degeri tarandi..."));
for (int i=0; i<4; i++)
{
readCard[i]=mfrc522.uid.uidByte[i];
Serial.print(readCard[i],HEX);
}
Serial.println("");
mfrc522.PICC_HaltA(); //Okuma durduruluyor.
return 1;
}
 
void ShowReaderDetails() {
// Get the MFRC522 software version
byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
Serial.print(F("MFRC522 Software Version: 0x"));
Serial.print(v, HEX);
if (v == 0x91)
Serial.print(F(" = v1.0"));
else if (v == 0x92)
Serial.print(F(" = v2.0"));
else
Serial.print(F(" (bilinmeyen)"));
Serial.println("");
 
if ((v == 0x00) || (v == 0xFF)) {
lcd.print("***UYARI***");   // Ekrana Yazı Yazdırılır.
lcd.setCursor(0, 1);    // ekranın alt satırına yazı yazdırmak için cursor konumlandırılır.
lcd.print("Donanım hatasi!!!");
Serial.println(F("DİKKAT! Haberlesme yetmezligi, MFRC522'yi dogru bagladiginizdan emin olun."));
while(true);
}
}
//////////////////Cycle Leds(Program Modu)///////////////////////
 
void cycleLeds() {
digitalWrite(redLed, LED_OFF); // kırmızı ledin sönük olduğundan emin olun
digitalWrite(greenLed, LED_ON); // yeşil ledin yanık olduğundan emin olun
digitalWrite(blueLed, LED_OFF); // mavi ledin sönük olduğundan emin olun
delay(200);
digitalWrite(redLed, LED_OFF); // kırmızı ledin sönük olduğundan emin olun
digitalWrite(greenLed, LED_OFF); // yeşil ledin sönük olduğundan emin olun
digitalWrite(blueLed, LED_ON); // mavi ledin yanık olduğundan emin olun
delay(200);
digitalWrite(redLed, LED_ON); // kırmızı ledin yanık olduğundan emin olun
digitalWrite(greenLed, LED_OFF); // yeşil ledin sönük olduğundan emin olun
digitalWrite(blueLed, LED_OFF); // mavi ledin sönük olduğundan emin olun
delay(200);
}
/////////////////Normal Led Modu//////////////////////////////
 
void normalModeOn () {
digitalWrite(blueLed, LED_ON); // mavi led yanık ve program kart okumaya hazır
digitalWrite(redLed, LED_OFF); // kırmızı ledin sönük olduğundan emin olun
digitalWrite(greenLed, LED_OFF); // yeşil ledin sönük olduğundan emin olun
 // kapının kilitli olduğundan emin olun
}
 
///////////////////////EEPROM için ID Okuma////////////////////////
 
void readID( int number )
{
int start=(number*4)+2; //başlama pozisyonu
for (int i=0; i<4; i++) //4 byte alamabilmek için 4 kez döngü kurucaz
{
storedCard[i]=EEPROM.read(start+i); // EEPROM dan diziye okunabilen değerler atayın.
}
}
 
////////////////////EEPROM a ID Ekleme///////////////////////////////////
 
void writeID(byte a[])
{
if (!findID(a)) //biz eeprom a yazmadan önce önceden yazılıp yazılmadığını kontrol edin.
{
int num=EEPROM.read(0);
int start= (num*4)+6;
num++;
EEPROM.write(0,num);
for(int j=0;j<4;j++)
{
EEPROM.write(start+j,a[j]);
}
successWrite();
Serial.println(F("ID basarili bicimde EEPROM a kayit edildi."));
}
else
{
failedWrite();
Serial.println(F("Basarisiz! Yanlıs ID veya kotu EEPROM. :-("));
}
}
/////////////////////////EEPROM'dan ID Silme////////////////////////
 
void deleteID(byte a[])
{
if (!findID(a)) //Önceden EEPROM'dan silinen karta sahipmiyiz kontrol et.
{
failedWrite(); //değilse
Serial.println(F("ID degeri taninmadi... :-("));
}
else
{
int num=EEPROM.read(0);
int slot;
int start;
int looping;
int j;
int count=EEPROM.read(0); //Kart numarasını saklayan ilk EEPROM'un ilk byte'ını oku
slot=findIDSLOT(a);
start=(slot*4)+2;
looping=((num-slot)*4);
num--; //tek sayacı azaltma
EEPROM.write(0,num);
for(j=0;j<looping;j++)
{
EEPROM.write(start+j,EEPROM.read(start+4+j));
}
for(int k=0;k<4;k++)
{
EEPROM.write(start+j+k,0);
}
successDelete();
Serial.println(F("Basarili bir sekilde ID kaydi EEPROM'dan silinmistir.."));
}
}
///////////////Byte'ların Kontrolü///////////////////
 
boolean checkTwo (byte a[],byte b[])
{
if (a[0] != NULL)
match=true;
for (int k=0; k<4; k++)
{
if(a[k] != b[k])
{
match=false;
}
if(match)
{
return true;
}
else
{
return false;
}
}
}
 
//////////////////////Boşluk Bulma////////////////////////////
 
int findIDSLOT (byte find[] )
{
int count = EEPROM.read(0); //EEPROM ile ilk byte ı okuyacağız.
for(int i=1; i<=count; i++) //Döngüdeki her EEPROM girişi için
{
readID(i); //EEPROM daki ID yi okuyacak ve Storedcard[4] de saklayacağız.
if (checkTwo(find, storedCard)) // Saklı Kartlar da olup olmadığının kontrolü.
{ //aynı ID'e sahip kart bulursa geçişe izin vericek.
return i; //kartın slot numarası
break; //aramayı durdurucak.
}
}
}
 
///////////////////////EEPROM'da ID Bulma//////////////////////
 
boolean findID (byte find[] )
{
int count = EEPROM.read(0); //EEPROM'daki ilk byte'ı oku
for(int i=1; i <= count; i++) //Önceden giriş yapılmış mı kontrolü.
{
readID(i);
if(checkTwo(find,storedCard) )
{
return true;
break;
}
else
{
//değilse return false
}
}
return false;
}
///////////////////Başarılı Şekilde EEPROM'a Yazma//////////////////
 
void successWrite()
{
digitalWrite(blueLed, LED_OFF); // mavi ledin sönük olduğundan emin olun.
digitalWrite(redLed, LED_OFF); // kırmızı ledin sönük olduğundan emin olun.
digitalWrite(greenLed, LED_OFF); // yeşil ledin sönük olduğundan emin olun.
delay(200);
digitalWrite(greenLed, LED_ON); // yeşil ledin yanık olduğundan emin olun.
delay(200);
digitalWrite(greenLed, LED_OFF); // yeşil ledin sönük olduğundan emin olun.
delay(200);
digitalWrite(greenLed, LED_ON); // yeşil ledin yanık olduğundan emin olun.
delay(200);
digitalWrite(greenLed, LED_OFF); // yeşil ledin sönük olduğundan emin olun.
delay(200);
digitalWrite(greenLed, LED_ON); // yeşil ledin yanık olduğundan emin olun.
delay(200);
}
 
/////////////////////EEPROM'a Yazma İşlemi Başarısız/////////////////
 
void failedWrite()
{
digitalWrite(blueLed, LED_OFF); // mavi ledin sönük olduğundan emin olun.
digitalWrite(redLed, LED_OFF); // kırmızı ledin sönük olduğundan emin olun.
digitalWrite(greenLed, LED_OFF); // yeşil ledin sönük olduğundan emin olun.
delay(200);
digitalWrite(redLed, LED_ON); // kırmızı ledin yanık olduğundan emin olun.
delay(200);
digitalWrite(redLed, LED_OFF); // kırmızı ledin sönük olduğundan emin olun.
delay(200);
digitalWrite(redLed, LED_ON); // kırmızı ledin yanık olduğundan emin olun.
delay(200);
digitalWrite(redLed, LED_OFF); // kırmızı ledin sönük olduğundan emin olun.
delay(200);
digitalWrite(redLed, LED_ON); // kırmızı ledin yanık olduğundan emin olun.
delay(200);
}
 
////////////////////Silme İşlemi Başarılı///////////////////////////
 
void successDelete()
{
digitalWrite(blueLed, LED_OFF); // mavi ledin sönük olduğundan emin olun.
digitalWrite(redLed, LED_OFF); // kırmızı ledin sönük olduğundan emin olun.
digitalWrite(greenLed, LED_OFF); // yeşil ledin sönük olduğundan emin olun.
delay(200);
digitalWrite(blueLed, LED_ON); // mavi ledin yanık olduğundan emin olun.
delay(200);
digitalWrite(blueLed, LED_OFF); // mavi ledin sönük olduğundan emin olun.
delay(200);
digitalWrite(blueLed, LED_ON); // mavi ledin yanık olduğundan emin olun.
delay(200);
digitalWrite(blueLed, LED_OFF); // mavi ledin sönük olduğundan emin olun.
delay(200);
digitalWrite(blueLed, LED_ON); // mavi ledin yanık olduğundan emin olun.
delay(200);
}
 
////////////////Master Kartın Doğruluğunun Tespiti///////////////////
 
boolean isMaster (byte test[])
{
if(checkTwo(test,masterCard))
return true;
else
return false;
 
}
