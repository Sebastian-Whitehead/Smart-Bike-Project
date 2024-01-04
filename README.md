# Smart bike media controller
The program was made in conjunction with a miniproject hand in for the mobile and wearable computing course in Aalborg University's 7th semester Medialogy course. The goal of this project was to rework and rethink the work of others who strove to reduce the cognitive load of controlling mobile devices while cycling, hereby comparatively increasing the user's situational awareness while cycling. Intern increasing rider safety.

## Authors

 - Arlonsompoon P. Lind
 - Jonas B. Lind
 -  Mads W. Sørensen
 -  Sebastian Whitehead
 
## Circuit Diagram
![N|Solid](https://i.imgur.com/LRyxQRj.png)

The digram illustrates the circuit for the current prototype the GitHub is built for. The program utilizes the BLE keyboard library to function as a Bluetooth media controller for smartphones *(Assistant summoning only works on android devices)* It emulates keyboard presses too:

|Input  | Function |
|--|--|
| Single Short Squeeze Either Hand | Pause / Play |
| Squeeze and Hold Right| Volume Up |
| Squeeze and Hold Left| Volume Down |
| Double Short Squeeze Right| Next Song |
| Double Short Squeeze Left| Previous Song |
| Squeeze and Hold Right| Summon Google Assistant (Android Only) |

The prototype consists of two force sensitive resistors, one for each side, mounted under each grip in a bike handle assembly together with a battery powered ESP32 centrally mounted by squeezing on either grip the system registers binary inputs after thresholding the analogue read values. 
