# comcat - A netcat-like tool for serial line communication

A simple command-line tool for communication over RS232/UART serial line.
Kept simple and stupid. It is easier to use as pure bash or minicom.

It is designed to work well with login shells, so you can use it interactively to 
connect to linux devices with a serial-line login shell.

## Examples

    # interactive, i.e. type with your keyboard and see the outputs
    comcat /dev/ttyUSB0 19200n81
    
    # save output to file
    comcat /dev/ttyUSB0 19200n81 > log.txt
    
    # send a file over uart
    comcat /dev/ttyUSB0 19200n81 < prerecorded.txt

## Usage

```
Usage: comcat [-ce] <tty> <config>
 <tty> is the console to use (e.g. /dev/ttyS0).
 <config> is a configuration specifier setting baud rate, parity bits, bit count, stop bit count and flow control.
  eg. 9600n81h (9600 baud, no parity, 8 data bits, 1 stop bit, hw flow control)
         ||||`-flow control (h - hardware cts/rts, s - software xon/xoff, omit for no flow control)
         |||`--stop bits    (1 or 2)
         ||`---data bits    (5, 6, 7 or 8)
         |`----parity bit   (n - no parity, e - even parity, o - odd parity)
         `-----baud rate    (50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800,
                             2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400)
 -e     echo all input from stdin to stdout
 -c     set stdin to canonical mode (output text lines rather than single characters)
```

## Tips

* Use ctrl-D to hang up in interactive mode.
* Unlike netcat and more like ssh, comcat sends every character as soon as you type it. Use -c if you need comcat to buffer your lines until you hit enter ('\n'). This gives you the ability to edit what you type before you send it.
