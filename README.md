# ESP8266-stock-sticker

![Picture of the stock ticker](https://github.com/nirpechuk/ESP8266-stock-sticker/blob/main/stock-ticker-pic.jpg?raw=true)

This project is a stock ticker that displays the price, symbol, and percent change of a stock, refreshing it every minute. See components below. 

Board https://a.co/d/9CmCsjR
Switch https://a.co/d/hpiXl6u
Screen https://a.co/d/dgNNiDY

Connect the screen:
- GND -> gnd
- VCC -> 3v
- SDA -> d2
- SCL -> d1

![Picture of the screen connections](https://github.com/nirpechuk/ESP8266-stock-sticker/blob/main/screen-connections.jpeg?raw=true)

Connect the switch to these pins:
- gnd
- d5

Get your API key by going to https://www.finnhub.io/register
Then, find the following line in the code to insert it: 

#define API_TOKEN "[insert your code here]"


Compile the code, install all the libraries, and upload the code to the board. 

Start with the switch on--the board will enter into setup mode, displaying the wifi access point name and password, as well as the address. Connect to this wifi, enter the name of the wifi that the stock ticker should be using, the password, and the stock ticker symbol you want to track. Submit the changes and turn off the switch. Restart the ticker by removing the power and reconnecting it. 

Here's the model to 3d print a shell for the stock ticker: [STL](https://github.com/nirpechuk/ESP8266-stock-sticker/blob/main/stock-ticker-box.stl)

Enjoy!
