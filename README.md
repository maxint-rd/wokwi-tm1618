# wokwi-tm1618
Wokwi custom chip TM1618. The TM1618 is a LED-driver chip with button interface, that allows easy connection of multi-segment LED displays to a microcontroller. The TM16xx Arduino library makes it very easy to use these and similar chips in on a variety of Arduino platforms. Wokwi allows users to play with virtual hardware within their browser.

### NOTE: this project is still in development
The basic code is working, but it's not complete. You can have a look but don't expect things to be perfect!
Currently supported:
- setting segments
- clearing display
- reading key-state.

### Live examples
- [Example demonstrating TM1618 custom component](https://wokwi.com/projects/452129958367862785) [Working]
- [Example demonstrating external TM1618 custom component](https://wokwi.com/projects/452234964175454209) [Working]

### References
- [Wokwi simulator](https://wokwi.com/) - Simulate various Arduino platforms in your browser (eg. atMega, ESP, STM, rPI)
- [Wokwi custom chips API](https://docs.wokwi.com/chips-api/getting-started) - how to add your own custom components
- [TM16xx Arduino library](https://github.com/maxint-rd/TM16xx) - support for the TM1618 and many other TM16xx chips
- [TM1618 datasheet](https://github.com/maxint-rd/TM16xx/blob/master/documents/LED%20driver%20TM1618_V2.1_EN.pdf) - All the info about this LED driver chip [mirror of v2.1 English]

### Disclaimer
All code on this GitHub account, including this software, is provided to you on an as-is basis without guarantees and with all liability dismissed. It may be used at your own risk. Unfortunately I have no means to provide support.
