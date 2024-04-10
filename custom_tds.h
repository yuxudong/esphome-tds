#include "esphome.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

static const char *const TAG = "MyTDSSensor";
static const uint8_t MyTDSSensor_REQUEST_LENGTH = 6;
static const uint8_t MyTDSSensor_RESPONSE_LENGTH = 6;
static const uint8_t MyTDSSensor_COMMAND_GET[] = {0xA0, 0x00, 0x00, 0x00, 0x00, 0xA0};

class MyTDSSensor:public PollingComponent,public UARTDevice,public Sensor {
  private: 
    uint8_t response[MyTDSSensor_RESPONSE_LENGTH];
  public:
    MyTDSSensor(UARTComponent *parent) : UARTDevice(parent), PollingComponent(1000) {}
    Sensor *input_tds_sensor =  new Sensor();
    Sensor *input_temp_sensor = new Sensor();
    Sensor *output_tds_sensor = new Sensor();
    Sensor *output_temp_sensor = new Sensor(); 

    void setup() override {
      // nothing to do here
    }

    float get_setup_priority() const override { return setup_priority::DATA; }

    void loop() override {
    }

    void receivce_data() {
      for(int i=0;i< MyTDSSensor_REQUEST_LENGTH;i++) memset(response,0,sizeof(response));
      int pos=0;
      while(this->available()) {
        response[pos]= this->read();
        pos++;
        if(pos > (MyTDSSensor_RESPONSE_LENGTH - 1)) break;
      }
    }

    void process_data() {
      if(response[0]!=0xaa and response[0]!=0xab) {
        ESP_LOGW(TAG,"Packet header error!");
        return;
      }
      if(!calc_checksum()){
        ESP_LOGW(TAG,"CheckSum error!");
        return;
      }
      if(response[0]==0xaa) {
        this->input_tds_sensor->publish_state(float((response[1] << 8) | response[2]));
        this->output_tds_sensor->publish_state(float((response[3] << 8) | response[4]));
      }
      if(response[0]==0xab) {

        this->input_temp_sensor->publish_state(float((response[1] << 8) | response[2])/100);
        this->output_temp_sensor->publish_state(float((response[3] << 8) | response[4])/100);
      }
    }

    void update() override{
      this->write_array(MyTDSSensor_COMMAND_GET,MyTDSSensor_REQUEST_LENGTH);
      this->receivce_data();
      this->process_data();
      this->receivce_data();
      this->process_data();
    }

    bool calc_checksum() {
      uint8_t crc = 0;
      for (int i = 0; i < MyTDSSensor_RESPONSE_LENGTH - 1; i++)  crc += response[i];
      if(crc==response[MyTDSSensor_RESPONSE_LENGTH - 1]) return true;
      ESP_LOGW(TAG, "CheckSum want 0x%X but current is 0x%X",response[MyTDSSensor_RESPONSE_LENGTH - 1],crc);
      return false;
    }
};