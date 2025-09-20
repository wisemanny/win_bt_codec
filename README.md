The process was taken from https://helgeklein.com/blog/how-to-check-which-bluetooth-a2dp-audio-codec-is-used-on-windows/

Huge thank.

The app creates a new ETW trace session named BT_CODEC. If for some reason it is
stuck, it can be stopped with

logman stop BT_CODEC -ets

If you want to check if the session is running, use:

logman query -ets

And look for BT_CODEC there.

The app runs in the console and waits for CTRL-C, upon which it closes the
session
