I am writing a cpp console app that would start a new ETW trace session
for a single provider with GUID 8776ad1e-5022-4451-a566-f47e708b9075 and will
record all events to the console. Later I want to add code to decode the 
payload and analyze it

The source code is in the /win_bt_codec.cpp and we use msvc

we should handle CTRL-C and close the session when user asks to stop the
program

MUST: add comments to the code, I want the code to be easy to understand
