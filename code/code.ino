/*
 * SPDIFVolumeControl - Arduino sketch for volume control via SPDIF/TOSLINK
 * 
 * Author: Sola85
 * License: GNU General Public License v3.0 (GPL-3.0)
 * 
 * This code is part of the SPDIFVolumeControl project:
 * https://github.com/Sola85/spdif-volume-control
 *
 * Description:
 * Reads SPDIF audio, modifies volume controlled by IR remote,
 * and outputs audio to a second SPDIF port.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "AudioTools.h"
#include "MySPDIFOutput.h"
#include "src/spdif_rx.h"
#include <Adafruit_NeoPixel.h>

#define IR_RECEIVE_PIN      14
#define DECODE_NEC
#define EXCLUDE_EXOTIC_PROTOCOLS
#include <IRremote.hpp>


static constexpr uint8_t PIN_PICO_SPDIF_RX_DATA = 15;
volatile static bool stable_flg = false;
volatile static bool lost_stable_flg = false;

#define NeoPixelPIN 18 
#define NUMPIXELS   20 
Adafruit_NeoPixel pixels(NUMPIXELS, NeoPixelPIN, NEO_GRB + NEO_KHZ800);

SPDIFOutput out; 
bool connected = false;
bool ever_connected = false;

void on_stable_func(spdif_rx_samp_freq_t samp_freq){
    stable_flg = true;
    connected = true;
    ever_connected = true;
}

void on_lost_stable_func(){
    lost_stable_flg = true;
    connected = false;
}

void setup(){}

const float INCREMENT = 1.414213562; // one volume increment = scale by sqrt(2)
const float MIN_VOLUME = 1./(1<<6);  // At least 16-6 = 10 bit resolution

float volumeSetting = 0.5;
float volumeSettingBackup = 0;       // To retain volume across mute
long lastVolumeChange = 0;           // Timestamp in order to ignore volume 
                                     // changes that occur to rapidly

int16_t maxReceivedSample = 0;       //Volume of received audio

bool muted(){
  return volumeSetting == 0;
}

void changeVolume(bool increase){
  if (increase){
    Serial.println("+");
    if (muted()){
      toggleMute();
      return;
    }
    volumeSetting *= INCREMENT;
    if (volumeSetting > 2) volumeSetting = 2;
  } else {
    Serial.println("-");
    if (muted()) return;
    volumeSetting /= INCREMENT;
    if (volumeSetting < MIN_VOLUME) volumeSetting = MIN_VOLUME;
  }
}

void toggleMute(){
  if (muted()){
    volumeSetting = volumeSettingBackup;
  } else {
    volumeSettingBackup = volumeSetting;
    volumeSetting = 0;
  }
}

void showVolume2(){

  uint16_t vuMeterValue = maxReceivedSample*volumeSetting;
  Serial.printf("Max: %d -> %d ~ %.1f bits\n", 
                maxReceivedSample, vuMeterValue, 
                bitsRequiredToRepresent2(vuMeterValue));
  Serial.printf("Volume: %f ~ %.1f bits\n", volumeSetting, 
                bitsRequiredToRepresent2(volumeSetting*32767)); 

  maxReceivedSample = 0;  //Reset after showing value

  pixels.fill(pixels.Color(0, 0, 0));

  float f = bitsRequiredToRepresent2(vuMeterValue);
  f = 2*(f-7); // Scale to fit on 20 LEDs
  
  for (int i = 0; i < f; i++){
    pixels.setPixelColor(i, pixels.Color(0, 0, 10));
  }
  //Show volume setting (= vuMeterValue or largest possible sample)
  f = bitsRequiredToRepresent2(volumeSetting*32767);
  f = 2*(f-7);

  auto volumeColor = connected ? pixels.Color(0, 20, 0) : pixels.Color(20, 20, 0);
  // Show volume in red if larger than 1 -> distortions possible
  pixels.setPixelColor(f, f >= 18 ? pixels.Color(20, 0, 0) : volumeColor);
  pixels.show();  
}

// Return the effective number of bits required to represent a sample
float bitsRequiredToRepresent2(uint16_t sample){
  return log2(sample) + 1; //Plus 1, since samples are signed
}

// Write audio data to spdif_tx
void writeAudioData(){
  uint32_t read_count = 0;
  uint32_t* buff;
  uint32_t total_count = spdif_rx_get_fifo_count();
  while (read_count < total_count) {
      uint32_t get_count = spdif_rx_read_fifo(&buff, total_count - read_count);
      for (int j = 0; j < get_count; j++) {
          int16_t sample = ((buff[j] & 0x0ffffff0) >> 12);
          if (sample > maxReceivedSample) maxReceivedSample = sample;
          sample *= volumeSetting;
          out.write((uint8_t*)&sample, sizeof(sample));
      }
      read_count += get_count;
  }
}

void loop()
{
    //for(int i = 0; i < 2; i++){DEBUGV("Waiting to start rx...\n");delay(1000);}
    pinMode(LED_BUILTIN, OUTPUT);

    set_sys_clock_khz(192000, false); 
    IrReceiver.begin(IR_RECEIVE_PIN, false);

    long last_update = millis();

    spdif_rx_config_t config = {
        .data_pin = PIN_PICO_SPDIF_RX_DATA,
        .pio_sm = 3,
        .dma_channel0 = 2,
        .dma_channel1 = 3,
        .alarm_pool = alarm_pool_get_default(),
        .flags = SPDIF_RX_FLAGS_ALL
    };


    spdif_rx_start(&config);
    spdif_rx_set_callback_on_stable(on_stable_func);
    spdif_rx_set_callback_on_lost_stable(on_lost_stable_func);


    AudioInfo info(48000, 2, 16);
    

    auto config_ = out.defaultConfig();
    config_.copyFrom(info); 
    config_.pin_data = 21;
    config_.buffer_size = 384;
    config_.buffer_count = 8;

    bool res = out.begin(config_);

    pixels.begin();

    while (true) {
        if (stable_flg) {
            stable_flg = false;
            Serial.printf("detected stable sync\n");
        }
        if (lost_stable_flg) {
            lost_stable_flg = false;
            Serial.printf("lost stable sync. waiting for signal\n");
            if (!ever_connected){ // If there is no signal present when we boot, for some reason we will immediately disconnect if the signal does appear.
              delay(1000);        // As a workaround, simply reboot if we've never seen a signal and we get a disconnect. 
              rp2040.reboot();    // Then we know the signal will be present after reboot and everything works fine.
            }
        }
        if (spdif_rx_get_state() == SPDIF_RX_STATE_STABLE){
          if (millis() - last_update > 100) {
            last_update = millis();
            digitalWrite(LED_BUILTIN, (millis()/100)%2); //Activity indicator when sync is stable
            //spdif_rx_samp_freq_t samp_freq = spdif_rx_get_samp_freq();
            //float samp_freq_actual = spdif_rx_get_samp_freq_actual();
            //Serial.printf("Samp Freq = %d Hz (%7.4f KHz)\n", (int) samp_freq, samp_freq_actual / 1e3);            
          }
          writeAudioData();
        }
        if (IrReceiver.decode()) {
    
          IrReceiver.resume();
          Serial.printf("%x %x\n", IrReceiver.decodedIRData.address, IrReceiver.decodedIRData.command);

          //Decode IR data
          if (millis() - lastVolumeChange > 250 && (IrReceiver.decodedIRData.address == 0xA6 || IrReceiver.decodedIRData.address == 0x04)) {
            lastVolumeChange = millis();
            switch(IrReceiver.decodedIRData.command){
              case 0x0B:
              case 0x03:
                changeVolume(false);
                break;
              case 0x0A:
              case 0x02:
                changeVolume(true);
                break;
              case 0x1E:
              case 0x09:
                toggleMute();
              default:  
                break;
            }
          }
        } 
    }
}

void setup1(){}

void loop1(){
  delay(100);
  showVolume2(); //Needs to happen on second core
                 // otherwise audio drops out occasionally
}

