# DHT22 / AM2302 cpp library for ESP32 (ESP-IDF)
**Forked from [DHT-source](https://github.com/gosouth/DHT22-cpp).**  
This is an ESP32 cpp (esp-idf) library for the DHT22 low cost temperature/humidity sensors.

Jun 2017: Ricardo Timmermann, new for DHT22. 

This lib is the cpp version of one in the DHT22 repo.

*Running DHT22*

Create folder components in main project directory. In this folder run:

```
$ git clone https://github.com/gosouth/DHT22-cpp.git
```

**USE**

See main.cpp

```C++
#define DHT_PIN GPIO_NUM_23
extern "C" {
void app_main();
}
void DHT_task(void *pvParameter)
{

    DHT(DHT_PIN);
    printf( "Starting DHT Task\n\n");

    while(true) {

        printf("=== Reading DHT ===\n" );
        int ret = dht.ReadDHT();

        dht.ErrorHandler(ret);

        printf( "Hum %.1f\n", dht.GetHumidity() );
        printf( "Tmp %.1f\n", dht.GetTemperature() );

        // -- wait at least 2 sec before reading again ------------
        // The interval of whole process must be beyond 2 seconds !!
        vTaskDelay( 3000 / portTICK_RATE_MS );
    }
}

void app_main()
{
    nvs_flash_init();
    vTaskDelay( 1000 / portTICK_RATE_MS );
    xTaskCreate( &DHT_task, "DHT_task", 2048, NULL, 5, NULL );
}
```


