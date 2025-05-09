#include "hal_i2c.hh"

/// I2C BUS

hal::I2CBus::I2CBus(int sda, int scl) {
    i2c_master_bus_config_t bus_config{};
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.i2c_port = I2C_NUM_1;
    bus_config.sda_io_num = static_cast<gpio_num_t>(sda);
    bus_config.scl_io_num = static_cast<gpio_num_t>(scl);
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));
    ESP_LOGI("I2CBus", "Initialized I2C bus SDA:%d SCL:%d\n", sda, scl);
}

hal::I2CBus::~I2CBus() {
    i2c_del_master_bus(bus);
}

hal::I2CDevice hal::I2CBus::NewDevice(const uint16_t address) const {
    return I2CDevice(bus, address);
}

/// I2C DEVICE

hal::I2CDevice::I2CDevice(const i2c_master_bus_handle_t &bus, const uint16_t address) {
    i2c_device_config_t dev_config{};
    dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_config.device_address = address;
    dev_config.scl_speed_hz = 1000000;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_config, &device));
    ESP_LOGI("I2C", "I2C device initialized address:%04x", address);
}

hal::I2CDevice::~I2CDevice() {
    i2c_master_bus_rm_device(device);
}

esp_err_t hal::I2CDevice::Transmit(const uint8_t *tx, size_t txSize, uint8_t *rx, size_t rxSize) const {
    return i2c_master_transmit_receive(device, tx, txSize, rx, rxSize, 1000);
}

esp_err_t hal::I2CDevice::Send(const uint8_t *tx, size_t txSize) const {
    return i2c_master_transmit(device, tx, txSize, 1000);
}
