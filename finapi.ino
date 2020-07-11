/*
    Author: Damilare Adedoyin
*/
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

const char* ssid = "YourWifiSSID";
const char* password = "***********";

class Stock {
  public: float list[6];
  private: int lossIndicator;
  private: int gainIndicator;
  private: unsigned long baudRate;
  public: HTTPClient http;
  public: int responseCode;
  public: JSONVar payloadObject;
  public: JSONVar keys;

  public: Stock(int _gainIndicator, int _lossIndicator, long _baudRate) {
      this->lossIndicator = _lossIndicator;
      this->gainIndicator = _gainIndicator;
      this->baudRate = _baudRate;
    }

    /*!
       @function init
       @abstract Initialize the PinModes, initial value of the Pins and the baudrate for serial communication
       @discussion This function initializes the output pins that would be used as indicators. It also initializes the serial communication port.
    */

    void init() {
      setOutputPin(this->lossIndicator);
      setOutputPin(this->gainIndicator);
      setPinLow(this->lossIndicator);
      setPinLow(this->gainIndicator);

      Serial.begin(this->baudRate);
    }

    /*!
      @function setOutputPin
      @abstract Set Pin to OutPut pin mode
      @discussion This function sets the mode of a pin to output.
      @param pin The desired pin
    */

    void setOutputPin (int pin) {
      pinMode(pin, OUTPUT);
    }

    /*!
      @function setPinLow
      @abstract Set Pin digital value to Low
      @discussion This function sets the pin digital value to give a low output.
      @param pin The desired pin
    */

    void setPinLow(int pin) {
      digitalWrite(pin, LOW);
    }

    /*!
      @function setPinHigh
      @abstract Set Pin digital value to Low
      @discussion This function sets the pin digital value to give a high output.
      @param pin The desired pin
    */
    void setPinHigh(int pin) {
      digitalWrite(pin, HIGH);
    }

    /*!
      @function gain
      @abstract Set the gain indicator and unset loss indicator
      @discussion This function displays a gain in price by setting the gain indicator and unsetting the loss indicator
    */

    void gain() {
      setPinHigh(this->gainIndicator);
      setPinLow(this->lossIndicator);
    }

    /*!
      @function loss
      @abstract Set the loss indicator and unset gain indicator
      @discussion This function displays a loss in price by setting the loss indicator and unsetting the gain indicator
    */

    void loss() {
      setPinHigh(this->lossIndicator);
      setPinLow(this->gainIndicator);
    }

    /*!
      @function comparePrice
      @abstract This function determines the status of a trade day
      @discussion This function determines whether the status of the current trade by comparing with the open price for the day
      @param openPrice The price at market open
      @param currentPrice The current market price
    */


    void comparePrice(int openPrice, int currentPrice) {
      if ((currentPrice - openPrice) > 0) {
        gain();
      } else {
        loss();
      }
    }

    /*!
      @function getStockValue
      @abstract Retrieve the stock information
      @discussion This function retrieves the stock information from the API
      @param apiUrl API URL
    */


    void getStockValue(String apiUrl) {
      this->http.begin(apiUrl.c_str());
      this->responseCode = getHttpResponseCode();
      parseData(this->responseCode);
    }

    /*!
      @function getHttpResponseCode
      @abstract This function gets the status code of the request
      @discussion This function gets the status code of the request (possible status codes are 200 OK, 404 Not found, etc)
    */


    int getHttpResponseCode() {
      int httpResponseCode = this->http.GET();
      return httpResponseCode;
    }

    /*!
      @function getKeys
      @abstract This function returns the keys of the payload Object
      @discussion This function returns the keys of the payload object.
    */

    JSONVar getKeys() {
      return this->payloadObject.keys();
    }

    /*!
      @function parseData
      @abstract This function tears down the response object
      @discussion This function parses the response object and compares the value.
      @param code The HTTP response code
    */

    void parseData(int code) {
      if (code > 0) {
        this->payloadObject = JSON.parse(http.getString());

        if (JSON.typeof(this->payloadObject) == "undefined") {
          Serial.println("Parsing input failed!");
          return;
        }

        this->keys = getKeys();

        int currentPrice = this->payloadObject["c"];
        int openPrice = this->payloadObject["o"];

        comparePrice(openPrice, currentPrice);

        for (int i = 0; i < keys.length(); i++) {
          this->list[i] = double(this->payloadObject[keys[i]]);
        }

      }
      else {
        Serial.print("Error code: ");
        Serial.println(code);
      }
    }
};

/*!
    @var apiUrl
    @abstract URL of the Stock Market Price API (Using https://finnhub.io/)
*/
String apiUrl = "https://finnhub.io/api/v1/quote?symbol=TSLA&token=";

/*!
    @var token
    @abstract API token from finnhub (https://finnhub.io/)
*/
String token = "*************************";
/*!
    @var lastTime
    @abstract Measure of time used
    @discussion This variable will help in setting the rate limits. Unsigned long because the value will get bigger as time passes.
*/

unsigned long lastTime = 0;

/*!
    @var timerDelay
    @abstract Delay in microseconds to space out API requests
*/
unsigned long timerDelay = 10000;

/*!
    @var OUTPUT26
    @abstract Pin Number for Gain Indication
*/
const int OUTPUT26 = 26;

/*!
    @var OUTPUT26
    @abstract Pin Number for Loss Indication
*/
const int OUTPUT27 = 27;

/*!
    @var baud
    @abstract Set Baud rate
*/
const long baud = 115200;

Stock TSLA(OUTPUT26, OUTPUT27, baud);

void setup() {
  //Call init function to initialise the pins in order to allow time for the hardware to settle to a stable state before being used.
  TSLA.init();
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      String serverPath = apiUrl + token;
      TSLA.getStockValue(serverPath);

      if (TSLA.responseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(TSLA.responseCode);

        Serial.print("JSON object = ");
        Serial.println(TSLA.payloadObject);

        for (int i = 0; i < TSLA.keys.length(); i++) {
          Serial.print(TSLA.keys[i]);
          Serial.print(" = ");
          Serial.println(TSLA.list[i]);
        }

      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
