/* DHT library


*/


#ifndef DHT_H
#define DHT_H

#define DHT_OK 0
#define DHT_CHECKSUM_ERROR -1
#define DHT_TIMEOUT_ERROR -2
#define MAXdhtData 5	// to complete 40 = 5*8 Bits

class DHT {

	public:

		explicit DHT(gpio_num_t gpioPin);

		static void ErrorHandler(int response);
		int ReadDHT();

		auto GetHumidity() const -> const float &{
            return humidity_;
        };
		auto GetTemperature() const -> const float &{
            return temperature_;
        };

	private:

		gpio_num_t DHTgpio_;
		float humidity_ = 0.;
		float temperature_ = 0.;

		int GetSignalLevel( int usTimeOut, bool state );
        static int VerifyCrc(std::array<std::uint8_t, MAXdhtData> verifyData);

};

#endif
