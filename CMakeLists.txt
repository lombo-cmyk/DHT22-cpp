set(CMAKE_CXX_STANDARD 17)

idf_component_register(SRCS "DHT.cpp"
        INCLUDE_DIRS "include"
        REQUIRES EspEthW5500Modbus
        PRIV_REQUIRES freemodbus)