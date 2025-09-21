# What it is

Simple console utility to get the name of the codec that Windows negotiated with the device. It uses ETW (Event Tracing for Windows) to listen to events from Microsoft.Windows.Bluetooth.BthA2dp provider.

The events contain a lot of data that I cannot yet decode and it is useful.

The process was taken from https://helgeklein.com/blog/how-to-check-which-bluetooth-a2dp-audio-codec-is-used-on-windows/

Huge thanks.

The app creates a new ETW trace session named BT_CODEC. If for some reason it is
stuck, it can be stopped with

```
logman stop BT_CODEC -ets
```
If you want to check if the session is running, use:

```
logman query -ets
```

And look for BT_CODEC there.

The app runs in the console and waits for CTRL-C, upon which it closes the
trace session.

Example output:

```
Listening for events... Press Ctrl+C to stop.
Now please connect the bluetooth device to the computer
# Selected codec: MPEG-2,4 (aka AAC)
Ctrl+C received, stopping trace...
```
# How to compile

I use make, nut the command line is very simple and uses cl.exe with a library

# How to trace

Use --trace or -t to print all events

# Gemini

Used to build first version with main, session creation and even handling loop
